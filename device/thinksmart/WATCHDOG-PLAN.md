<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart watchdog recovery gate

Status (2026-11-14): W1 and W2 passed on the owner's unit. The host lease
passed renewal and explicit disarm. The attended RAM exitrd also passed
clean teardown and automatic reboot after host lease expiry.
Current hardware runs `6.19.5-thinksmart1-g770e10fa15a0`
from the pinned `msm8953-mainline/linux` commit
`770e10fa15a00051eaef862e4cb2724f2f8fa568`. It retains the USB ACM checkpoint,
guarded persistent mapper and systemd console. The tested boot slice stays
available for rollback. No display/touch/OTG probe belongs to this gate.

The driver was explicitly loaded after sync, unmount and confirmation that
all physical, loop and mapper nodes were read-only. Feeding for 25 seconds
and surviving 25 seconds after magic close passed. With feeding withheld,
USB disconnected and a subsequent boot returned automatically to the
checkpoint with a different boot ID. The pending marker survived; all 19
post-reset checks passed, including protected-region and retained-file
hashes. The 20-second timeout is the configured value: the host's 42-second
observation includes USB/disconnect detection and is not a precise hardware
timing measurement. No experimental driver auto-loaded on the new boot.

## Source and scope

The owner's offline stock DT candidates agree on the watchdog's extent
and SPI numbers. The private extraction is retained outside Git. The
[Qualcomm watchdog driver](https://android.googlesource.com/kernel/msm.git/+/03c2d42aa4bc362578b3824a81583638e2e23151/drivers/soc/qcom/watchdog_v2.c)
uses the KPSS register offsets, requests a rising-edge bark interrupt and
uses a nominal 32 kHz counter. The pinned mainline
[`qcom-wdt.c`](https://github.com/msm8953-mainline/linux/blob/770e10fa15a00051eaef862e4cb2724f2f8fa568/drivers/watchdog/qcom-wdt.c)
has the same register layout, supports magic close and obtains the counter
clock through DT. Using the board's existing 32768 Hz sleep clock is an
inference to validate with elapsed-time tests; the downstream driver's
default is 32765. Mainline compatibility is not established by these source
comparisons alone.

`overlays/watchdog.dtsi` keeps the node disabled. The separately included
`watchdog-bootstrap.dtsi` enables it only in the local attended candidate.
The generic `qcom,kpss-wdt` binding is supported but deprecated in favour
of SoC-specific compatibles; this is a local experiment, not an upstream
binding claim. The candidate does not modify the kernel or existing DT
properties. It adds a matching, manually loaded `qcom-wdt.ko` and a static
`watchdog-test` to initramfs. Nothing invokes either automatically. Udev,
modules-load and graphical login remain held. Firmware watchdog behaviour
before Linux claims it is still unknown.

## W0: offline validation (no hardware access)

Compile the exact pinned board DTS plus the two include fragments in an
isolated directory. Compare decoded DT properties: the watchdog must be
the only added node; every previous node and property must remain identical.
Validate against the pinned watchdog schema. Compile the helper for ARM64
with warnings as errors and exercise its wrong-host refusal under qemu.
Preserve all previous initramfs files byte-for-byte, validate module ABI,
kernel/initramfs memory gap, actual lk2nd parser, ext2 read-only fsck and
extracted boot-file hashes. Keep the previous full boot slice and its hash.

Pass: all checks recorded privately. Stop: any changed existing DT property,
guard/script difference, automatic driver load, schema or extraction failure.
Rollback: discard the regular candidate file; the device is unchanged.

## W1: attended bootstrap (one new hardware driver; high risk)

This is the initial establishment of the watchdog itself: it cannot be
protected by an already verified hardware watchdog. Keep the owner-recovery
path ready, capture serial logs first and use a bounded software timeout.
That timeout is not a guarantee against a kernel hang. Do not interpret
the bootstrap exception as permission to probe other drivers unprotected.

After the owner's approved boot update and cold start, verify the physical
USB location, kernel, checkpoint, previous application/probe hashes, guard
and protected-region hashes. Record a pending marker through the existing
bounded data mapper, then sync and cleanly unmount merged root, data and
base root. Detach the read-only root loop, revoke the writable marker and
run the guard. Every MMC, loop and mapper must now read `ro=1` and no ext4
or overlay mount may remain. Keep helper and module in initramfs/RAM.

Start host capture before explicitly inserting only `/qcom-wdt.ko`. Verify
the new watchdog device belongs to `qcom_wdt`. Do not open it merely to
inventory it: opening arms the watchdog. The insertion itself can adopt a
firmware-running watchdog; treat it as the attended hardware boundary.

Run `/watchdog-test exercise`. It checks the board/kernel, read-only state,
driver identity and API, sets a 20-second timeout with zero pretimeout,
feeds for 25 seconds, sends magic close, then survives another 25 seconds.
It must emit `WATCHDOG_FEED_AND_DISARM_PASS` and exit zero. Capture the
whole interval and verify a fresh serial roundtrip and block protection.
Stop on a hang, unexpected reboot, API mismatch or any kernel I/O/oops error.
If control is lost, stop issuing commands and request the owner's recovery.

## W2: prove timeout recovery (storage unmounted and read-only)

Keep W1's protection and capture. Run `/watchdog-test bite`: it arms the
same timeout and holds the descriptor open without feeding. A reset should
occur near 20 seconds. If no reset occurs by 45 seconds, it attempts magic
close and reports failure; an interrupted test also fails. Never treat
USB disappearance alone as proof of reset.

Pass requires a subsequent checkpoint, a different boot ID, matching
protected hashes and retained pending marker. The new boot must not load
any experimental driver automatically. Read the marker before changing it
to a completed result. If reboot enters a firmware menu or loses USB, the
owner may need to cold-start or return to EDL. A failed test is not a usable
recovery mechanism. Keep the tested previous slice as the bounded EDL
rollback; never change GPT, calibration or the system root for this gate.

## W3: protect later single-driver trials

Only after W2 passes, a feeder may protect display or touch trials. Its
lease must be renewed by successful host command roundtrips, not by an
unconditional local loop. Arm it and a host log stream before one detached
driver operation. A missed lease must stop feeding and request orderly
recovery; the hardware timeout covers a hung kernel. Keep an overall trial
deadline and a persistent pending marker, and leave the next boot at the
checkpoint. Disarm only after a fresh host connection confirms the guard,
control path and probe result. Never auto-repeat a failed driver load.

These later trials also need a bounded per-driver undo plan. Do not unload
a display driver blindly if it owns the framebuffer or shares dependencies
with USB. Kernel reset into the held checkpoint is the final recovery, with
the owner's physical cold-start/EDL path as backup. No watchdog result alone
proves display, touchscreen, GPU or desktop operation.

### W3 results and exitrd gate

The RAM-only lease controller now passes 16 host renewals over about 69
seconds and survives 65 seconds after explicit magic close. With renewal
withheld, it requests reboot and the device returns to the checkpoint with
a different boot ID and retained marker. All 19 protected-region/module/
retention checks pass. The first implementation delayed shutdown by holding
the watchdog descriptor after SIGTERM. It now closes without magic close
on the shutdown signal: the pinned watchdog core performs one final ping
and leaves the watchdog active. Kernel timestamped reports confirm this
handoff. Neither recovery attempt established clean data unmount: ext4
reports journal recovery on the next boot, without I/O or filesystem errors.

Before driver trials, test `shutdown-exitrd` in RAM using systemd's
[exitrd interface](https://github.com/systemd/systemd/blob/v262/docs/INITRD_INTERFACE.md).
Its static BusyBox environment verifies PID 1, board/kernel and the exact
mapper. Before shutdown, bind the verified ext4 mount under
`/run/initramfs/data` so a reference reaches exitrd. Normally unmount the
old root, then sync/unmount `/data` and protect the mapper. A busy old `/dev`
may be detached only after confirming that it is a duplicate of the active
devtmpfs and has no descendants. Storage mounts require normal unmounts.
On a failed reboot teardown, give the watchdog time to act, then attempt a
bounded sync and recovery reboot if PID 1 can still run. A recovery reboot
after a STOP does not pass the clean teardown gate. The pending marker is created through the merged root and only read
after that overlay has been detached. Test copies have passed actual
OverlayFS/ext4 teardown in an isolated host mount/PID namespace and clean
offline fsck; this is not yet a real-device exitrd result.

Pass requires a captured teardown result, a new held checkpoint, no ext4
journal recovery on that boot, preserved marker and all existing checks.
Stop on missing mounts, a wrong mapper, any unmount error or lost recovery.
Rollback is removal of the RAM-only exitrd before a trial, or the proven
checkpoint/boot-slice recovery after a failure. No persistent boot image is
changed for this test. Display, touch and desktop remain held.

The first live exitrd reached its RAM environment but stopped because the
data mount no longer had its original path. No subsequent unmount was
attempted. The controller had yielded its descriptor to systemd, whose
default 10-minute shutdown timeout exceeded this driver's supported range.
The pinned systemd watchdog code disarms on that setup failure. Thus a
successful earlier reboot did not establish continuous shutdown watchdog
coverage. Passive observation showed no automatic reset, and owner cold
recovery was requested. Do not reuse this first exitrd trial.

The revised controller refuses to arm unless the manager reports
`RebootWatchdogUSec=20000000` and `RuntimeWatchdogUSec=0`; set only the
former through the writable manager D-Bus property before testing. This
does not enable an unconditional runtime feeder. On hardware, systemd
reported a 20-second timeout, but the original data mount disappeared
before exitrd and the system did not reset within the observation window.
After a cold start, all 20 recovery checks passed. Binding the data mount
under `/run/initramfs/data` preserved it at `/data`. The next live test
stopped on a busy old `/dev` and returned automatically to the held
checkpoint, with all 20 checks passing and no ext4 journal replay. The
latest namespace test reproduces the busy duplicate devtmpfs, verifies
normal ext4 unmount and confirms a clean ext4 superblock. Sources: systemd v262
[`watchdog.c`](https://github.com/systemd/systemd/blob/v262/src/shared/watchdog.c),
[`umount.c`](https://github.com/systemd/systemd/blob/v262/src/shutdown/umount.c),
and the [manager D-Bus interface](https://www.freedesktop.org/software/systemd/man/262/org.freedesktop.systemd1.html).
