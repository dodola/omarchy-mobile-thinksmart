#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""RAM-only watchdog lease: only host-acknowledged progress renews feeding."""
import argparse
import array
import fcntl
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time


class Lease:
    def __init__(self, now, maximum, heartbeat=15):
        self.end = now + maximum
        self.expires = now + heartbeat
        self.interval = heartbeat
        self.sequence = 0
        self.last_ping = now - 2
        self.terminal = None

    def step(self, now, sequence, disarm=None):
        if self.terminal:
            return self.terminal
        if (now >= self.end or now >= self.expires or
                sequence not in (self.sequence, self.sequence + 1)):
            self.terminal = 'expired'
            return self.terminal
        if sequence > self.sequence:
            self.sequence = sequence
            self.expires = now + self.interval
        if disarm == self.sequence:
            self.terminal = 'disarm'
            return self.terminal
        if now - self.last_ping >= 2:
            self.last_ping = now
            return 'ping'
        return 'wait'


def wait_for_shutdown(fd, stopped, report, sleep=time.sleep, close=os.close):
    """Yield to shutdown without magic close; the watchdog remains armed.

    On this pinned qcom_wdt kernel, release without V sends one final ping
    and leaves the active watchdog running, without a background feeder.
    SIGTERM must not make systemd wait for the hardware timeout before it
    can unmount the persistent filesystem.
    """
    try:
        while not stopped():
            sleep(0.2)
    finally:
        close(fd)
    report('shutdown-released', recovery='descriptor closed without magic close')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage')
    parser.add_argument('--maximum', type=int, default=180)
    args = parser.parse_args()
    assert re.fullmatch(r'[a-z0-9-]{1,48}', args.stage)
    assert 45 <= args.maximum <= 300
    assert os.geteuid() == 0
    assert os.uname().release == '6.19.5-thinksmart1-g770e10fa15a0'
    assert Path('/sys/firmware/devicetree/base/compatible').read_bytes().split(b'\0')[0] == b'lenovo,cd-18781y'
    assert Path('/sys/class/watchdog/watchdog0/device/driver').resolve().name == 'qcom_wdt'
    subprocess.run(['/run/omamobile-thinksmart/busybox', 'sh',
                    '/run/omamobile-thinksmart/guard', 'once'], check=True, timeout=8)
    # An unsupported systemd shutdown timeout makes systemd magic-close the
    # watchdog on setup failure. Require a verified compatible handoff.
    for prop, expected in [('RebootWatchdogUSec', 't 20000000'),
                           ('RuntimeWatchdogUSec', 't 0')]:
        actual = subprocess.check_output([
            '/usr/bin/busctl', 'get-property', 'org.freedesktop.systemd1',
            '/org/freedesktop/systemd1', 'org.freedesktop.systemd1.Manager',
            prop], text=True, timeout=8).strip()
        assert actual == expected, 'Unsafe systemd watchdog setting: ' + prop
    base = Path('/run/omamobile-thinksmart/driver-stage') / args.stage
    assert base.is_dir() and not base.is_symlink()
    assert base.stat().st_uid == 0 and base.stat().st_mode & 0o077 == 0
    assert (base / 'heartbeat').read_text().strip() == '0'
    assert not (base / 'disarm').exists()
    with (base / 'started').open('x') as stream:
        stream.write(str(os.getpid()) + '\n')

    def report(state, **extra):
        data = dict(state=state, **extra)
        temporary = base / 'status.new'
        temporary.write_text(json.dumps(data) + '\n')
        temporary.replace(base / 'status.json')
        print(json.dumps(data), flush=True)
        # Kernel timestamps let the host distinguish feeding, lease expiry,
        # shutdown delay and the later watchdog event without wall-clock drift.
        kmsg = os.open('/dev/kmsg', os.O_WRONLY | os.O_CLOEXEC | os.O_NONBLOCK)
        try:
            message = '<6>THINKSMART_LEASE ' + json.dumps(data) + '\n'
            os.write(kmsg, message.encode())
        finally:
            os.close(kmsg)

    def read_sequence(path):
        text = path.read_text().strip()
        if not re.fullmatch(r'[0-9]{1,6}', text):
            raise ValueError('Invalid sequence')
        return int(text)

    # Signals cannot accidentally magic-close a protection lease.
    stopped = False

    def stop(_signum, _frame):
        nonlocal stopped
        stopped = True

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    fd = os.open('/dev/watchdog0', os.O_WRONLY | os.O_CLOEXEC | os.O_NOFOLLOW)
    armed = False
    disarmed = False
    try:
        info = bytearray(40)
        fcntl.ioctl(fd, 0x80285700, info, True)  # WDIOC_GETSUPPORT
        options = int.from_bytes(info[:4], 'little')
        assert info[8:].split(b'\0')[0] == b'qcom_wdt'
        assert options & 0x100  # WDIOF_MAGICCLOSE
        timeout = array.array('i', [20])
        fcntl.ioctl(fd, 0xC0045706, timeout, True)
        assert timeout[0] == 20
        if options & 0x200:  # WDIOF_PRETIMEOUT
            fcntl.ioctl(fd, 0xC0045708, array.array('i', [0]), True)
        actual = array.array('i', [0])
        fcntl.ioctl(fd, 0x80045707, actual, True)
        assert actual[0] == 20
        policy = Lease(time.monotonic(), args.maximum)
        armed = True
        report('armed', sequence=0, timeout=20)
        while True:
            try:
                sequence = read_sequence(base / 'heartbeat')
                disarm = read_sequence(base / 'disarm') if (base / 'disarm').exists() else None
                action = 'expired' if stopped else policy.step(time.monotonic(), sequence, disarm)
            except (OSError, ValueError):
                action = 'expired'
            if action == 'disarm':
                assert os.write(fd, b'V') == 1
                os.close(fd)
                fd = -1
                disarmed = True
                report('disarmed', sequence=policy.sequence)
                return
            if action == 'expired':
                break
            if action == 'ping':
                fcntl.ioctl(fd, 0x80045705, array.array('i', [0]), True)
                report('feeding', sequence=policy.sequence)
            time.sleep(0.2)
    except BaseException:
        if not armed:
            os.write(fd, b'V')
            os.close(fd)
            raise
        # After arming, an internal failure uses the same recovery path.
    if armed and not disarmed:
        try:
            report('expired', recovery='orderly reboot requested; feeding stopped')
        finally:
            try:
                subprocess.Popen(['/usr/bin/systemctl', 'reboot', '--no-block'],
                                 stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL)
            except OSError:
                pass
            # Hold until shutdown signals us; never disarm during recovery.
            # If shutdown never signals, the held watchdog still expires.
            wait_for_shutdown(fd, lambda: stopped, report)


if __name__ == '__main__':
    main()
