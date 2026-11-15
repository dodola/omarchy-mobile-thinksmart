<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart View CD-18781Y notes

## 2026-11-15: correct Adreno firmware placement

The first GPU trial registered a render node and kept serial control.
Kernel logs showed both A530 microcode files loading but the A506 ZAP
shader failing with `-ENOENT`. The live device tree specifies
`qcom/msm8953/lenovo/cd-18781y/a506_zap.mdt`. The first RAM staging placed
the ZAP files at `qcom/`. The next trial uses the board-specific path for
the signed image and split segments and keeps the microcode in `qcom/`.
Use EGL renderer identity and an actual render/readback before declaring
hardware acceleration. The first GPU trial ended in a clean reboot, with
all 21 checkpoint checks passing.

The corrected trial loaded the chipset microcode and the board-specific
ZAP shader. An EGL/GBM test on the DRM render node reported `FD506`, cleared
an OpenGL ES framebuffer and read back the expected RGBA pixel. The
kernel log check found no GPU initialization or firmware errors. This
test exercised GPU rendering independently of the desktop compositor.

With the FT8201 display enabled, an attended FT8201 touch trial confirmed
physical taps and swipes reach Linux. The evdev capture included 10
tracking-ID events, 111 X-coordinate updates, 86 Y-coordinate updates and
132 synchronization reports. The associated interrupt counter reached 155.
An attended test then used the installed SDDM → UWSM → Omarchy session.
The selected input device was classified as a touchscreen, but Hyprland
did not attach it after only the event node had been triggered. Retriggering
the input parent and event with a ThinkSmart-only runtime `seat` and
`uaccess` rule exposed the FT8201 in Hyprland. The DSI output kept its
`800x1280` mode.

The first login's normal `lock-on-start` reached the secure lock state
within seconds. The owner reported a black screen and could not unlock it.
The test temporarily held that startup toggle for one SDDM autologin.
The owner then hit the two intended targets in order and a finger swipe
scrolled the Wayland window by 59 pixels. The original toggle was
restored, the test window stopped, and the active Omarchy session was left
running. The black screen on the locked startup path remains open.

The first desktop trial exposed interleaved `/dev/kmsg` lines in the
host's lease-status reply. The device automatically returned to the
checkpoint and ext4 mounted without replay. After separating serial
status records, compositor logs showed that held udev left the DRM render
node root-only and the initial XDG runtime path exceeded the socket length
limit. A shorter private XDG directory, current Lua configuration and a
temporary per-user render-node ACL let Hyprland open the built-in DSI
output at `800x1280` and map a native Wayland terminal window.

The Chromium test with GPU enabled failed to map a window and reported a
GPU subprocess exit. A separate run with a temporary D-Bus session and
software rendering opened the local test page as a sandboxed native
Wayland window. These were temporary sessions. The installed Omarchy
profile and normal display-manager login still require an end-to-end test.

## 2026-11-14: persistent mount handoff survives systemd shutdown

The owner's cold start after the stalled exitrd returned to the held
checkpoint and passed 20 checks. The next trial showed systemd programming
the supported 20-second watchdog timeout, then exitrd found no visible
data mount and stopped. A second cold start passed the same checks.
An explicit bind under `/run/initramfs/data` brought the verified ext4
mount into exitrd. That test stopped on a busy old devtmpfs mount and
automatically recovered to the checkpoint. The 20 checks passed without
ext4 journal replay. The busy-devtmpfs fix passed an isolated mount/PID
namespace test with normal ext4 unmount and a clean superblock.

The following live lease expiry reached the RAM shutdown environment,
detached only the duplicate old devtmpfs, unmounted the root overlay and
the persistent filesystem, and verified read-only storage before reboot.
The USB console returned at a new held boot. All 21 checks passed,
including protected-region hashes, installed module hashes, retained
markers and a fresh ext4 mount without journal replay.

In the first guarded input trial, the selected FT8201 bound to
`edt_ft5x06`. Linux created an evdev node with absolute-axis capabilities
and retained serial control. No events arrived in the capture windows,
and the registered GPIO interrupt count remained zero. Physical contact
inside the observation window still needs confirmation. The controlled
reboot after this trial repeated the clean exitrd result and all 21
post-boot checks.

Separate guarded trials bound `qcom_labibb_regulator` and registered the
Qualcomm WLED backlight device. The LAB/IBB trial matched eight protected
regions against the original backup before and after loading the module.
Both trials returned cleanly to the held checkpoint and passed all 21
post-boot checks. A506 firmware files were identified and extracted from
the owner's original vendor backup for the GPU trial and remain private.

With Adreno registration held, MSM MDSS, DPU and DSI bind successfully.
After adding the selected FT8201 panel and the verified suppliers, DRM
reports an enabled DSI connector and an `800x1280` mode with `msmdrmfb`
active. Physical panel appearance and accelerated desktop remain to be
confirmed.

## 2026-11-14: host lease passes; shutdown recovery still needs correction

On `6.19.5-thinksmart1-g770e10fa15a0`, the RAM lease controller passed 16
host renewals over about 69 seconds and survived 65 seconds after explicit
disarm. With renewal withheld, two tests returned to a new held checkpoint;
all 19 storage, retained-file and module checks passed. Both boots reported
ext4 journal recovery, so these were not proof of clean persistent teardown.
The original feeder delayed shutdown; SIGTERM now closes its descriptor
without magic close and allows systemd to continue.

A RAM-only exitrd was then tested to finish the overlay/data teardown. It
entered its environment but stopped before unmounting because systemd had
changed the mount layout. Automatic recovery did not follow: systemd's
unsupported default shutdown timeout caused it to disarm the watchdog.
The owner has been asked to cold-start the unchanged boot image. No display,
touch or desktop driver has been tested. W3 remains open; see
[WATCHDOG-PLAN.md](WATCHDOG-PLAN.md) for the corrected manager-timeout gate,
bounded relocated-mount handling, evidence and rollback.

The revised exitrd passes real OverlayFS/ext4 teardown in isolated host
mount/PID namespaces for original, relocated and relocated/read-only data
mounts, followed by clean offline fsck. It has not yet been tested on the
device. Source review also found `CONFIG_REGULATOR_QCOM_LABIBB` absent from
the installed kernel configuration. The selected panel requires LAB/IBB
supplies. An unchanged pinned-source module builds with `W=1` against the
exact installed kernel output; it remains an offline candidate, not an
installed or working regulator driver. Keep panel/GPU probes held.

## 2026-11-14: watchdog reset returns to the guarded checkpoint

On `6.19.5-thinksmart1-g770e10fa15a0`, the new candidate passed all 19
checkpoint checks. Data and base filesystems were synchronized and
unmounted, the root loop detached and writable marker revoked. Every MMC,
loop and mapper node then read read-only before explicit watchdog loading.
The host captured kernel output; a fresh serial command confirmed control
before removing the bootstrap software reset timer.

The watchdog helper fed for 25 seconds with a configured 20-second timeout,
sent magic close, and survived another 25 seconds. A fresh guard/control
check passed. The next test withheld feeding: the host recorded arming and
USB disconnection, then the device automatically returned to the held
checkpoint. A changed boot ID and retained pending marker prove a new
boot. All 19 post-reset checks passed, including protected-region hashes,
original probes, the installed test package and matching modules. The new
ext4 log does not report journal recovery. The completed marker was archived
and synchronized. Exact hardware expiry time was not measured; USB detection
latency is included in the host observation.

W1/W2 of [WATCHDOG-PLAN.md](WATCHDOG-PLAN.md) therefore pass for this unit.
The host-lease controller for later driver probes is still pending. Display,
touch, GPU and desktop remain unverified; no such driver was probed here.

## 2026-11-14: watchdog bootstrap candidate installed and read back

The owner returned to EDL. Live identity and fresh primary GPT, backup GPT,
calibration, lk2nd and full previous boot-slice checks passed. Only the
reviewed nested boot slice was updated; its full readback matches the
candidate, and the protected hashes still match afterward. No reset was
sent. The previous slice is retained as the direct rollback. The next
owner cold start and actual watchdog tests remain pending; no display,
touch or desktop probe has run.


## 2026-11-14: watchdog bootstrap candidate verified offline

The live system still passes serial, guard, retained-probe and kernel-error
checks. Driver probes remain held. The recovery gate now has an isolated
candidate built from the installed `6.19.5-thinksmart1-g770e10fa15a0` boot
slice. Its only DT change is a watchdog node: all existing properties are
byte-identical after decoding. The pinned watchdog schema passes; a
negative control without its clock fails as expected. Both DT fragments
pass checkpatch. The generic KPSS binding remains an experimental choice,
with source evidence and remaining hardware assumptions recorded in
[WATCHDOG-PLAN.md](WATCHDOG-PLAN.md).

The kernel is unchanged. All previous initramfs files retain their hashes;
the additions are the matching manual-load watchdog module and a static
ARM64 test helper. The helper compiles with warnings as errors and refuses
the wrong host under qemu. It requires an unmounted, read-only checkpoint
before its feed/disarm and timeout-reset tests. Neither addition starts
automatically. The actual lk2nd parser, extracted boot-file hashes and
ext2 read-only fsck pass; the kernel/initramfs memory margin is positive.
The previous verified boot slice remains the direct rollback.

This is offline preparation, not a verified watchdog or a driver-test
result. The candidate still needs the owner's physical EDL/cold-start
steps, bounded write/readback, then attended watchdog bootstrap with host
logs and storage unmounted. Display, touch and desktop remain held until
actual timeout recovery returns to the checkpoint.

## 2026-11-14: embedded console, firstboot and cold-boot application retention pass

The owner's power cycle booted `6.19.5-thinksmart1-g770e10fa15a0` into the
guarded persistent checkpoint. Both existing probes retain their hashes
in the merged root and upper. The installed `usage` package database and
binary remain intact, and all 1,518 matching module files pass their saved
hash list. The data filesystem reports completed journal recovery without
an observed filesystem or I/O error; final unmount on the previous shutdown
was not observed, so this is not evidence of a verified clean shutdown.

The image's embedded v2 console scripts then handed off to systemd without
a RAM script correction. A fresh serial roundtrip confirmed systemd and
the guard before disarming the fallback timer. Udev coldplug, modules-load
and SDDM remain runtime-masked. Releasing only the userspace gate completed
`omamobile-firstboot`: service result success, exit status zero, completion
marker present and the private firstboot configuration removed. Native
`iptables v1.8.13 (nf_tables)` succeeds with the new kernel.

As the ordinary user, `usage` still executes and sandboxed headless Chromium
opens the existing loopback test page and profile. Its exact DOM reports
the retained localStorage value after this power cycle, and the test unit
exits successfully. The test did not recreate the profile or page. This
proves application and test-profile retention; desktop rendering, touch,
OTG and general graphical applications remain unverified. Chromium logged
D-Bus errors during the headless test, which did not prevent that result.

Fresh primary GPT, backup GPT, calibration and lk2nd hashes remain
unchanged. Physical MMC and loop nodes stay read-only; only the exact
bounded data mapper is writable. About 2.1 GiB remains available. The
successful handoff marker was archived as complete and synchronized.

Read-only inventory finds simplefb at 800 x 1280, 24 bpp, no DRM card and
only gpio-keys input. The selected FT8201 node exists but has no bound
driver; the display and GPU platform nodes are also unbound. Neither a
watchdog device nor a watchdog node appears in the running device tree.
The next driver gate therefore needs a reviewed recovery/watchdog path
before any module probe. No display, touch or USB host driver was loaded
in this stage. The installed image still stops at its host-controlled
checkpoint on every boot and is not an unattended desktop release.

## 2026-11-13: nftables kernel candidate installed; boot still pending

After the owner returned to EDL, the pinned device passed fresh checks of
both GPTs, calibration, lk2nd and the full previous nested boot slice.
Only that boot slice was replaced with the offline-verified nftables
kernel/console candidate. Its complete readback matches, and fresh GPT,
calibration and lk2nd hashes remain unchanged. The previous slice is kept
for rollback. No reset was sent. The new kernel's hardware boot, firewall
retry and application retention across this power cycle remain unverified.


## 2026-11-13: native package/browser tests pass; firewall blocks firstboot

The replacement kernel `6.19.5-thinksmart1-g770e10fa15a0` has since completed
its separate LLVM 18 `W=1` build. The private boot candidate passes ext2
read-only fsck, extracted-file hashes, the actual lk2nd configuration parser,
ARM64 script/staging checks and matching early-module vermagic checks. Its
board DTB is byte-identical to the installed one, and the checked kernel
memory gap remains positive. It incorporates the tested v2 console
preludes and explicit runtime gates; it has not yet been written or booted.
Its independent module directory has been added to the guarded persistent
root. All 1,518 staged files match their host hashes; dependency lookup for
nftables succeeds without loading a module into the old kernel. Fresh
protected-region hashes still match. The previous kernel/module tree and
the currently installed boot slice remain available for rollback.

With the v2 serial console and guard active, runtime masks kept udev
coldplug, module loading and SDDM inactive while normal software startup
was released. Generic `systemd-firstboot` had been waiting on a local TTY;
it was stopped and runtime-masked, leaving setup to the prepared
`omamobile-firstboot` configuration. The device package now stages that
mask for future builds. The old probe-created configuration directory's
mode was corrected to 0755, keeping the private configuration at 0600.

Omamobile firstboot completed its keyring, account, locale, SSH, login and
user-profile steps. Its services/firewall step failed: `iptables --version`
reports `Failed to initialize nft: Protocol not supported`. Package-file
verification reports no altered iptables files. The running kernel has
`CONFIG_NF_TABLES` unset. The ThinkSmart fragment now requests the nftables
and UFW match/target support already used by the main project config, with
a distinct kernel local version to avoid mixing module directories.
Firstboot remains incomplete and retains its configuration for retry;
the new kernel has not yet been tested on hardware.

The device's initialized keyring verified an offline package signature,
and native pacman installed `usage` 7.3.0 successfully. It executes as the
ordinary user from the persistent root. Chromium 156.0.8078.79 also passed
a native headless loopback-page test with its sandbox enabled: a dedicated
profile stored a localStorage value, then a second browser process read
it back. An earlier attempt hit the test unit's 128-task ceiling; the
retry used 512. This proves software execution and profile reopening,
not cold-boot application retention, GPU rendering, desktop display or
input. Those remain separate tests.

## 2026-11-13: RAM console correction passes the systemd handoff

The owner's next cold boot returned to the persistent checkpoint. The
data filesystem completed journal recovery without a captured filesystem
or I/O error, and both existing probes retained their hashes in the merged
root and upper. The v2 correction was transferred and checked in RAM.
On `6.19.5-g770e10fa15a0`, systemd then started the corrected USB console
and guard services. A new command roundtrip confirmed PID 1 and the guard
before stopping the fallback timer; an independent serial connection
also succeeded. Physical MMC and loop nodes remain read-only, with only
the exact bounded data mapper writable.

Udev coldplug, the module-load service and graphical login are held by
verified runtime masks while software firstboot is prepared. Generic
modules have appeared during systemd startup, so this does not establish
that no module was loaded. The installed boot image still contains the
old strict terminal prelude: this success requires the RAM override and
has not yet been incorporated into a new boot image. Normal software
firstboot, application retention, desktop, touch and OTG remain separate
unverified steps.

## 2026-11-13: cold-boot retention passes; systemd console prelude fails

After the owner's full power cycle, both existing probe files retain their
expected hashes in the merged root and persistent upper. They were read,
not recreated. The physical nodes remain read-only. Fresh mapper, data
identity and protected-region checks pass before the next handoff.

RAM-only units were staged to hold normal userspace, udev coldplug and
module loading while starting an early serial console and a storage guard
unit. A timer requests a fallback console if the host cannot confirm a new
session. The initramfs then handed off; the private serial capture shows
systemd executing both console services. Neither reaches an interactive
shell: the shared prelude has `set -e` and treats `stty` returning failure
as fatal. The fallback reproduces the same failure. No panic/oops appears
in the captured stream, but clean unmount or shutdown cannot be confirmed
through this failed interactive channel.

The pinned kernel's
[`gs_tty_ops`](https://github.com/msm8953-mainline/linux/blob/770e10fa15a00051eaef862e4cb2724f2f8fa568/drivers/usb/gadget/function/u_serial.c)
has no `set_termios` callback; the TTY core's
[`tty_set_termios`](https://github.com/msm8953-mainline/linux/blob/770e10fa15a00051eaef862e4cb2724f2f8fa568/drivers/tty/tty_ioctl.c)
then preserves hardware settings. This supports removing the unnecessary
baud-rate request. The prepared RAM-only correction tolerates failure of
optional terminal tuning and gives the fallback its own prelude without
`stty`. Guard failure remains fatal. Six actual ARM64 prelude regressions,
unit parsing and the checkpoint wait/release test pass. Runtime recovery
is still pending: the owner must return to the image's default checkpoint,
then the host can check recovery logs and apply the correction in RAM.
No new boot-image write is required for that retry.

Cold-boot retention is proved for the test files only. Interactive systemd
control, application-state retention and the desktop remain unverified.

## 2026-11-13: bounded persistence and remount retention verified

The owner's cold boot reached the persistent checkpoint with working USB
serial control on `6.19.5-g770e10fa15a0`. The live device-mapper table maps
exactly the planned data extent within `system`; its filesystem UUID
matches the build. The base root reports `ro,norecovery`, all physical MMC
and loop nodes are read-only, and only the bounded data mapper is writable.
The full base-root SHA-256 matches the installed image. Available data
space before the probe is approximately 2.26 GiB.

Two new small probe files, one under `/var/lib/omamobile` and one under
`/etc/omamobile`, were written through the merged root and synchronized.
After unmounting and remounting the overlay and data filesystem, both
hashes match in the merged root and persistent upper. Fresh primary GPT,
backup GPT, calibration and lk2nd hashes remain unchanged. The captured
kernel log contains no panic/oops or filesystem I/O error. This verifies
runtime writes and remount retention, not cold-boot or application-state
retention.

The test filesystems were then cleanly unmounted, the loop released and
the RAM writable marker revoked. The guard returned all block nodes,
including the mapper, to read-only. The device now awaits the owner's cold
boot; the next check must read the existing probes without recreating them.
Systemd handoff, desktop, Chromium execution and input remain unverified.

## 2026-11-13: USB console and read-only root verified on hardware

After the owner fully disconnected power and USB for at least 30 seconds,
the screen returned to boot logs. Windows subsequently exposed the CDC ACM
serial interface on the same physical USB port as the preceding EDL
session. Host commands now complete over that console. The precise cause
of the preceding black screen and descriptor failure remains unproven.

Live reads report `6.19.5-g770e10fa15a0`, the expected board compatible,
USB role `device`, UDC state `configured`, and the installed v2 helper's
expected hash. The guard process is running and all 30 MMC block nodes
report read-only. System and calibration partition geometry matches the
reviewed layout. A read-only loop scan found the expected bounded root
partition; mounting it with `ro,noload` succeeded. The resulting mount
reports `ro,norecovery`, systemd is executable, and the matching kernel
module directory is present. All MMC and loop nodes remained read-only.

Read-only package queries find Chromium `156.0.8078.79-1`, Hyprland
`0.57.0-3.2`, and systemd `262.1-1`. Pacman also reports a missing public
keyring / GPGME engine warning in this minimal chroot; package installation
has not been tested. The image deliberately omits the generated keyring;
the existing firstboot program initializes it. Neither browser execution nor desktop readiness is
implied by the package inventory. No persistent filesystem was written,
and no root handoff was attempted.

An overlong host command exceeded the interactive BusyBox line buffer and
left an unfinished compound command. A deliberate syntax error discarded
it before execution; subsequent bounded commands and sentinels completed.
Private host captures preserve the evidence. Persistent handoff, OTG host
input, touch and reboot persistence remain unverified.

Fresh reads through the running kernel reproduce the recorded primary
GPT, backup GPT, calibration and lk2nd hashes. The test mount and loop were
then released. No kernel oops, panic or I/O error appeared in the captured
log. A private persistent candidate now retains the proven ACM binding,
kernel, DTB, display, guard and bounded mapper. It pauses after the overlay
mount until a host-created RAM marker explicitly requests handoff. The
subsequent target is `multi-user.target`; a runtime serial unit is staged
to preserve control after systemd starts.

Offline validation covers extracted payload hashes, the actual lk2nd
parser, ext2 read-only fsck, ARM64 shell syntax and kernel memory bounds.
Admission regressions reject a writable physical node, incorrect board,
missing opt-in and incorrect mapper identity/length. The actual checkpoint
code waits without an automatic timeout, and systemd's parser accepts the
serial unit. These checks do not prove persistent writes or userspace
handoff on hardware. The verified v2 diagnostic slice and full stock eMMC
backup remain the recovery paths.

The owner then returned the unit to EDL. The same pinned WinUSB device
passed fresh reads of both GPTs, calibration, lk2nd and the entire v2
diagnostic boot slice. Only the nested boot slice was replaced with the
persistent USB checkpoint candidate; its complete readback matches the
candidate hash. Both GPTs, calibration and lk2nd were reread and unchanged.
The previous boot slice was retained. No reset was sent; at that point
the device remained in EDL awaiting the owner's cold boot. The subsequent
persistent-mount result is recorded above. The new runtime console handoff
and systemd execution remain unverified.

## 2026-11-13: USB controller ready; ACM association path corrected

The owner's transcription reports 30 read-only MMC nodes, USB role
`device`, a bound USB controller driver and UDC `7000000.usb` in
`not attached` state. The helper fails while associating the ACM function
with its configuration, before UDC binding. Some transcription details
appear inaccurate; the owner separately confirmed that keyboard input
still does not work and that the apparent `help` command was not an actual
interaction.

The installed helper contains a concrete error: its symlink target is
`../../functions/acm.GS0`. Unlike ordinary symlink creation, configfs
immediately resolves the target from the caller's current directory via
[`get_target()`](https://github.com/msm8953-mainline/linux/blob/770e10fa15a00051eaef862e4cb2724f2f8fa568/fs/configfs/symlink.c).
The helper runs outside the gadget directory, so this lookup fails.
The earlier image/syntax validation did not exercise this behavior.

The v2 candidate uses the absolute function path, checks association
failure explicitly and logs the stage before UDC binding. Extracted-file
comparison against the actual v1 readback confirms that only `usb-console`
changed inside the initramfs. Regression checks reproduce the old target's
ENOENT from root and an unrelated working directory; the new target
resolves and the actual ARM64 BusyBox link command succeeds in both.
This host lacks the USB gadget modules, so these path checks are not a
live configfs gadget-binding test. Existing module, filesystem, parser and
memory-layout checks also pass. After the owner returned the unit to EDL,
both GPTs, calibration, compat2 and the complete installed v1 boot slice
matched fresh reads. Only that boot slice was replaced with v2 and fully
read back, with an exact hash match. Both GPTs, calibration and lk2nd were
read again and unchanged. The previous slice is retained; no reset was
sent. After the owner's next cold boot, Windows reports an unknown USB
device with a device-descriptor request failure (problem code 43), with
location paths matching the unit's preceding EDL connection. No COM port
appears during the bounded enumeration check. This is a new enumeration
attempt, not successful serial communication. The device's final association,
binding and USB-state logs are awaited before selecting the next change.

## 2026-11-13: OTG keyboard unresponsive; USB serial diagnostic prepared

The owner connected an OTG keyboard to the working diagnostic shell.
Enter had no effect, and no keyboard indicator/backlight illumination was
observed. This does not establish a hardware limitation or identify the
failure. The pinned board inherits `role-switch-default-mode = "peripheral"`;
the actual USB role, controller binding, enumeration and VBUS have not
been read on the unit. The kernel includes DWC3 dual-role, QUSB2 PHY, HID
and role-switch support. The reference's [USB status](https://github.com/rickx/ubuntu-thinksmart/blob/main/README.md)
also leaves OTG untested.

Prepared a private diagnostic boot slice with one opt-in CDC ACM console,
using the existing device role. It adds the matching `libcomposite`,
`u_serial` and `usb_f_acm` modules and a guarded helper, following the
[kernel configfs interface](https://docs.kernel.org/usb/gadget_configfs.html).
It validates the board, diagnostic flags, active storage protection and
MMC read-only states before binding the one expected UDC. It keeps the
display shell and starts the serial shell with a controlling terminal.
It requests no host-mode transition and exposes no network/storage USB
function. Its CDC descriptors follow the [Windows inbox serial-driver
matching guidance](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-driver-installation-based-on-compatible-ids).

The kernel, DTB, existing simplefb module, guard, persistence code, layout
and firstboot configuration remain unchanged. ARM64 syntax, required
applets, module architecture/vermagic/dependencies, actual lk2nd parsing,
read-only ext2 checks, extracted-file comparisons and memory placement
checks pass. An execution without diagnostic opt-in rejects before device
actions. After the owner returned the unit to EDL, fresh complete reads of
both GPTs, calibration, compat2 and the old diagnostic boot slice matched
the verified state. Only the nested boot slice was replaced; its full
readback matched the candidate, and both GPTs, calibration and lk2nd were
read again and unchanged. The complete previous slice was retained. No
reset was sent. Actual USB enumeration and serial interaction remain
unverified. The owner subsequently confirmed a cold boot with the PC USB
cable connected. Windows found no diagnostic COM port after a bounded
30-second enumeration wait and no corresponding unknown USB device.
The final on-screen USB-helper result is awaited before choosing a fix.
The actual payload DTB separately confirms enabled USB/PHY nodes and the
peripheral default; this does not prove runtime driver binding.
The persistent candidate is deferred
until a usable diagnostic/control path is established. The working
diagnostic slice remains the exact-range rollback.

## 2026-11-13: Linux diagnostic console observed after compat2

The owner's next photo shows Linux running `/init`, the runtime release
`6.19.5-g770e10fa15a0`, and simplefb registering an 800 x 1280, RGB888
framebuffer at `0x90001000`. The console then prints
`THINKSMART DIAGNOSTIC READY: system and persist read-only` and
`diagnostic shell; storage guard active`, followed by a BusyBox shell
prompt. This confirms Linux/initramfs execution and visible framebuffer
output, and the diagnostic's geometry/read-only checks for those two
partitions. It does not establish keyboard input, an independent audit of
every block node, root handoff, persistent storage or the desktop.

The shell reports that it lacks a controlling tty; input/job-control
behavior has not yet been tested. Deferred-probe and sync-state messages
remain visible after the shell starts. The standalone simplefb module is
marked out-of-tree by the kernel and successfully registers afterward.
The original persistent boot slice lacks this working display setup, so
a replacement must preserve the proven kernel/DTB and display parameters
before any attempt to switch entries. No device command was sent while
recording this observation.

A private persistent boot-slice candidate is now prepared from the verified
diagnostic slice. It keeps the kernel, DTB, simplefb module, guard, mapper,
persistence helper, layout and firstboot configuration unchanged. The init
script initializes the display after guard activation (and after the guard
on stopped paths), so persistent boot gets the same console, and reports
data-mount and systemd-handoff stages. All entries retain the working
display parameters; DEFAULT selects persistent. ARM64 shell syntax, the
installed lk2nd parser, read-only ext2 checking, extracted-file hashes and
kernel/initramfs/DTB placement checks pass. This candidate is not installed.
The working diagnostic slice is retained for exact-range rollback. The
owner has a keyboard/OTG adapter; actual input is still being checked.

## 2026-11-13: visible handoff and missing framebuffer-node reproduction

The owner supplied a photo after the clock-preservation diagnostic update
and confirmed that the screen remains on that image. It shows lk2nd
compat1 selecting `diagnostic`, decompressing the kernel, completing DT
updates, then printing `Jumping to kernel via monitor`. The image's
1,461,002-byte initramfs is loaded. No Linux console or diagnostic marker
has been observed. The earlier SD slot2 errors concern the extra SD-card
probe; the subsequent nested eMMC boot filesystem was successfully read.

The Linux board DT's existing framebuffer reservation collides with the
node name the old lk2nd updater tries to create at the same address. A host
test of the actual updater and shipped DTB reproduced the silently missing
`simple-framebuffer` node. A bounded reuse fix passes the regression cases
in [lk2nd/README.md](lk2nd/README.md). The compat2 candidate is built and
its signature checked. The owner then returned the unit to EDL. Its USB
path matched the latest record, and fresh full reads of both GPTs,
calibration, compat1 and the diagnostic boot slice matched the verified
state. Only lk2nd was updated to compat2; its complete readback matched.
Both GPTs, calibration and the full diagnostic boot slice were read again
and unchanged. No reset was sent. The owner must now perform the physical
boot; whether the live framebuffer uses this address and whether Linux is
running remain unknown.

## 2026-11-13: lk2nd version mismatch reproduced and replacement verified

The owner observed the lk2nd menu identifying Lenovo ThinkSmart View and
`20.0-next-b9c007bb5492-20250123`. Selecting Continue returned to the menu.
This proves boot to lk2nd, not boot to Linux. Fastboot later enumerated on
Windows and its log was retrieved, but the original error had already
rolled out of the buffer. A computer-issued Continue returned to the same
menu, as confirmed by the owner, and its USB interface did not reappear.
There is no captured runtime error establishing which loader check failed
first.

The exact old source's parser reproduced a concrete incompatibility with
the installed configuration: uppercase directives leave all boot files
unset. It also lacks DEFAULT selection. Independently, its 32 MiB kernel
limit is smaller than the current decompressed image. Earlier offline
checks incorrectly relied on newer lk2nd behavior and missed this version
mismatch.

Built `20.0-thinksmart-compat1` using the same base source, with upstream
extlinux fixes and visible boot logs. Source references, reproduction and
validation are in [lk2nd/README.md](lk2nd/README.md). After the owner selected
EDL, the same device, both GPTs, calibration and the original lk2nd were
checked, along with the full stock backup. Only the lk2nd payload was
replaced; its complete readback matched. Both GPTs and calibration were
then checked again and were unchanged. No software reset was issued.

The owner then reported a black screen. A further startup while holding
Volume Down showed logs followed by a black screen. The last log line is
unknown, so Linux entry is not established. A diagnostic boot slice has
been prepared offline with `clk_ignore_unused`, `pd_ignore_unused` and
`regulator_ignore_unused`: this initramfs does not load DRM, while its
clock, power-domain and regulator providers are built in. Resource cleanup
is a possible display-loss cause, not a confirmed diagnosis. The modified
slice retains identical kernel, initramfs and DTB bytes and passes ext2
and actual backported-parser checks. The owner returned the unit to EDL;
Windows assigned a different topology-derived USB instance. Before any
write, both raw GPTs (including disk GUID), calibration, compat1 lk2nd and
the entire old boot slice matched the private verified records. The new
diagnostic slice was then written and fully read back. Both GPTs,
calibration and lk2nd were read again and unchanged. No reset was sent.
The later screen observation and next candidate are recorded in the newer
entry above.

The next attended boot must establish whether Linux reaches its diagnostic
marker. Kernel build release remains `6.19.5-g770e10fa15a0`; hardware
`uname -r`, root handoff, desktop and persistence remain unverified.

## 2026-11-12: owner-authorized installation; first boot still pending

The owner's unit was connected through Windows WinUSB in Qualcomm EDL mode.
A complete 7,818,182,656-byte eMMC backup was retained outside Git and its
SHA-256 checked. The owner authorized the migration and installation in
separate stages, followed by computer-side progression under the reviewed
local procedure. Physical power and cable operations remain with the owner.
This records that one installation; it does not authorize other devices or
change the public installer policy.

Completed and checked through exact-length EDL readbacks:

- Copied the unit's original calibration partition to the planned location
  and compared SHA-256 before changing the partition tables.
- Wrote the backup GPT, then the primary GPT. Both headers and entry-array
  CRCs pass, the tables agree, and the retained partitions are unchanged.
- Wrote lk2nd and the 7,311,699,968-byte diagnostic system image. Their full
  readbacks match the intended payloads. Both GPTs and the relocated
  calibration partition also match their pre-image-write hashes.
- Independently checked the saved system readback's nested boot, root and
  data regions against the offline payload hashes. The persistent boot
  slice is 261,095,424 bytes and still selects `DEFAULT persistent`; it has
  not been activated on the unit.

The locally staged diagnostic image adds a `simplefb` module built from the
pinned kernel source and matching build output, plus `lk2nd.pass-simplefb`.
It attempts a visible console without loading the DRM panel stack. After
the storage guard runs, it checks the expected `system` and `persist`
geometry and read-only flags before printing
`THINKSMART DIAGNOSTIC READY: system and persist read-only`.
This diagnostic adaptation is private scratch work, not yet part of the
repository builder. Neither the display nor this marker has been observed
on hardware. The marker alone would not prove interactive console access,
root handoff, or successful persistent boot.

Build release: `6.19.5-g770e10fa15a0`; there is still no hardware `uname -r`.
The image includes Chromium and an empty persistent overlay. Graphical
startup, touch, Chromium rendering, app/settings retention across a real
restart, connectivity and other peripherals all remain unverified. The
latest host check still found the same unit in EDL; no reboot was issued.

All images, per-device hashes, identifiers, credentials, EDL logs and the
installation checkpoint are retained in the private local installation
package or capture directories. Returning to stock now requires the
reviewed full-eMMC restore; the old calibration-copy-only rollback no
longer applies after GPT and system writes.

## 2026-11-11: offline port started; no hardware results

The supplied source identifies the candidate as Lenovo ThinkSmart View
CD-18781Y with APQ8053/MSM8953, 2 GB RAM and 8 GB eMMC. This identifies the
source's target only; the owner's model, bootloader, partition layout and
backup have not been examined.

Research pins the kernel source to Linux `v6.19.5-r0`, commit
`770e10fa15a00051eaef862e4cb2724f2f8fa568`. The expected board DTB is
`qcom/apq8053-lenovo-cd-18781y.dtb`. The reference integration repository
was inspected at `d2e32393b30dfcc8397192747fced0fb7115fc33`.

The first implementation is an offline image path for lk2nd/extlinux with
a read-only persistent root and a RAM overlay. It does not provide an EDL,
GPT or unsuffixed-partition writer. Existing boot-image writers must refuse
the experimental boot method.

No firmware, calibration files or prebuilt bootloader/module binaries were
imported into Git. No device was contacted, flashed, rebooted or modified.
There is no hardware-tested `uname -r` for this port.

Offline checks on 2026-11-11:

- The pinned source's `defconfig` + `msm8953.config` + this port's fragment
  passed `olddefconfig` and the builder's exact config checks with LLVM 18.
- The board DTB built with `W=1`. A targeted `CHECK_DTBS=y` run against
  `arm/qcom.yaml` completed but is **not a clean schema pass**: the source
  reports `graph_child_address` for the USB ports, a duplicate `qcom,fg.yaml`
  schema id, and the board's compatible tuple is absent from `arm/qcom.yaml`.
  These are findings in the unmodified pinned source. They must be resolved
  or explicitly reviewed before hardware use; no kernel patch is claimed.
- `packages/omamobile-device-thinksmart/test/all`, `tests/thinksmart-rootfs`,
  `tests/thinksmart-image`, `tests/build-rootfs` and `tests/boot-method`
  passed. Image tests use synthetic kernel/ELF/root payloads with real
  ext2/ext4 filesystems: geometry, exact root bytes, cpio format, checksums,
  capacity and rejected inputs are checked, **not a successful boot**.
- A locally downloaded static aarch64 BusyBox passed ELF preflight. Its
  applets were not executed on the target.
- `tools/pr-check` passed against a temporary Git snapshot of the changes,
  including identifier/blob checks and script parsing. No branch was moved
  or pushed. `git diff --check` passed.
- The full package gate ran serially: **24 of 32 suites passed**. The eight
  failing, unmodified suites were audio, shell, fingerprint, mmd, sensors,
  settings, speaker-tuning and telephony-tegu. Failures include older Qt
  imports, missing Python GI/qmake, systemd option incompatibility, an audio
  UCM check, a fingerprint timing check, and Python test-module resolution.
  The new ThinkSmart suite passed; this is not a clean full-project gate.
- `tests/all` passed its earlier suites, including the existing rootfs,
  storage-guard and boot-method checks, then stopped in the unmodified
  `tests/release-upload` suite at its published-file-permissions assertion.
  The complete integration gate is therefore also not green on this host.
- This host has no usable aarch64 binfmt handler. The full Mesa build and
  real Omarchy root filesystem build have not run. Their ABI, installed
  size, runtime graphics and the 512 MiB overlay's capacity remain unverified.

The complete `W=1` kernel build succeeded with LLVM 18. The release is
`6.19.5-g770e10fa15a0` (build metadata, **not a hardware `uname -r`**).
Long command sessions were terminated twice; the same output tree was
resumed without changing source/configuration, and the final compile and
stripped module installation returned zero. The unmodified source emits
many `W=1` warnings, including syscall initializer overrides, unused values
and pointer-to-enum casts; this is not a warning-clean build certification.

Local artifacts are in the ignored `out/thinksmart/kernel-6.19.5-first/`:

| Artifact | Size / result |
| --- | --- |
| `arch/arm64/boot/Image.gz` | 15,105,685 bytes; ARM64 header and embedded release checked |
| `arch/arm64/boot/dts/qcom/apq8053-lenovo-cd-18781y.dtb` | 65,100 bytes; schema limitations above |
| `modules-6.19.5-g770e10fa15a0.tar.zst` | 19,496,620 bytes; 1,475 modules, archive paths and root ownership checked |
| `modules/lib/modules/6.19.5-g770e10fa15a0/` | Stripped modules and depmod output; watchdog vermagic matches the kernel |
| `SHA256SUMS` | Kernel, DTB, modules archive, config and metadata all verified |

Logs are kept locally under ignored `logs/thinksmart/`. No full Omarchy
root image or flashable system release was produced, and no hardware
support is inferred from the successful kernel compile.

Follow [BRINGUP-PLAN.md](BRINGUP-PLAN.md) G0–G3 for offline preparation. G4
remains blocked on a reviewed safety-model entry and attended recovery
procedure; source documentation of a working installation is not evidence
that this port boots.

## 2026-11-11: bounded root mapping with BusyBox

BusyBox 1.36.1 rejects `losetup --sizelimit`, which stopped the Omarchy
entry before mounting the root. The initramfs now asks the kernel to scan
the nested MBR on a read-only loop, checks the second partition's start and
length in sysfs, and checks both block nodes for read-only status before
mounting ext4 with `ro,noload`.

On WSL, the static ARM64 BusyBox under QEMU reproduced the old argument
error and mounted the expected read-only second partition with the new
arguments. The backing device was a read-only loop over a synthetic nested
image. The ThinkSmart image, ThinkSmart rootfs and shared rootfs suites
passed. These checks used host block devices backed by scratch files.

## 2026-11-12 offline Omarchy image

- Built the pinned kernel with LLVM 18 and `W=1` and installed its matching
  `6.19.5-g770e10fa15a0` modules. Built Mesa `1:26.2.4-1.5` with Freedreno
  and the ThinkSmart device package `0.1.1-1`.
- The first reduced desktop package selection still occupied 7,102 MiB.
  The final device profile occupied 6,019 MiB and retained Hyprland,
  Chromium, CJK fonts and the Omarchy Mobile packages. The build used a
  current Arch Linux ARM mirror with the Hyprland dependency ABI expected
  by the pinned Omarchy Mobile packages.
- Recreated the root filesystem in 6,656 MiB. Read-only mounts and an rsync
  dry run compared file checksums, ownership, permissions, hard links, ACLs
  and extended attributes against the original ext4. The compact root
  passed `e2fsck -fn` and `tests/rootfs-smoke`.
- The nested `system.img` is 7,241,465,856 bytes. SHA-256, MBR offsets and
  lengths, board DTB identity, packaged initramfs, both filesystems and the
  default diagnostic entry passed offline checks. The same ARM64 BusyBox
  under QEMU mounted the packaged second partition with `ro,noload`.
  Hyprland and the Freedreno MSM DRI module resolved their shared libraries
  inside that mounted root.

The image contains no firstboot account configuration. The device was not
connected, booted or flashed during this build.

## 2026-11-12: persistent overlay offline

Reused the verified Lite root and kernel. The persistent build has
6,035,416 sectors of ext4 data starting at sector 8,245,248 within
`system`. It retains the default diagnostic and RAM-overlay entries.
The additional entry maps only the data extent. It requires the planned
outer layout and the data filesystem's build-time UUID.

On WSL, the packaged ARM64 mapper and BusyBox ran under QEMU user mode
against a scratch-backed block device and simulated board sysfs. The
guard left the mapper writable while raw writes were rejected. The same
initramfs functions rejected incorrect outer geometry and a mismatched
UUID. All three filesystems passed read-only checks.

Installed the signed ARM64 `usage` package into the overlay with pacman.
After unmounting, detaching, remapping and mounting again, the package
binary, pacman database, user and system settings and an OverlayFS
whiteout were still present. The hash of the fixture outside the mapped
data extent was unchanged. This did not exercise lk2nd, a full reboot on
the board, or the actual eMMC driver.

## 2026-11-15: login lock screen

The desktop idle settings leave the lock screen's own blank timer active.
On the ThinkSmart View, keep the lock screen lit so it can be unlocked by
touch without a power key. The board package supplies the display policy
file. The lock service checks it before blanking and leaves the login lock
and PAM flow in place.

Applied the Omarchy 4.0.6 patch on the persistent overlay and restarted
the SDDM session. Hyprland reported a secure lock. After the blank timer
fired, DSI-1 still reported DPMS on and the backlight read 2048. The owner
could see the lock screen, swipe up and unlock with the original
credential. The shell logged `unlocked` and Hyprland reported an unlocked
session. The test used an SDDM relogin within the running Linux boot.
