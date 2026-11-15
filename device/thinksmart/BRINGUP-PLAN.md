<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart View CD-18781Y: first-light plan

Status (2026-11-15): experimental bring-up. One owner's separately authorized
EDL installation and bounded boot updates have completed with full readback.
The lk2nd compatibility and framebuffer handoff fixes reach Linux.
`6.19.5-thinksmart1-g770e10fa15a0` now boots the guarded persistent root and
hands off to systemd using its embedded USB console scripts. Software
firstboot, signed package execution and sandboxed headless Chromium pass.
Existing probes, the installed test package, matching modules and browser
test-profile state survive a cold boot. The image still requires host
release of its checkpoint. DSI modesetting and an Adreno A506 render/readback
pass. A temporary Hyprland session maps a native Wayland terminal and a
Chromium window using software rendering for Chromium. With targeted
runtime udev setup, the installed SDDM/UWSM Omarchy session accepts physical
taps on the intended targets and a finger swipe scrolls a Wayland window.
The board's lock display policy fixes the black screen. SDDM relogin
verified that the secure lock stays visible and accepts the original
credential by touch. Normal cold boot to the desktop and USB host mode
remain to be verified. See
[lk2nd compatibility](lk2nd/README.md) and the dated
entry in [NOTES.md](NOTES.md); this does not establish a public installer or
extend authorization to another device. Kernel source: `msm8953-mainline/linux`
`v6.19.5-r0`, commit `770e10fa15a00051eaef862e4cb2724f2f8fa568`.
Use a dedicated source checkout and a separate output directory; there is no
shared kernel output or release-kernel branch for this port.

## Scope and evidence

The target inferred from the supplied source is **Lenovo ThinkSmart View
CD-18781Y**, APQ8053/MSM8953, ARM64, 2 GB RAM and 8 GB eMMC. This is a
source identification, not verification of the owner's unit. Other ThinkSmart
models, including View Plus, are outside this port.

The intended system is Arch Linux ARM with Omarchy, Hyprland and the
ThinkSmart device package. The initial build preserves the existing lk2nd
boot model, uses a dedicated read-only eMMC initramfs, and mounts the root
filesystem through a RAM overlay. It does not convert an existing system in
place. Changes in the RAM overlay disappear when power is removed.

| Claim | Evidence | Status on this port |
| --- | --- | --- |
| Board identity and device tree | [Linux board DTS at the pinned revision](https://github.com/msm8953-mainline/linux/blob/770e10fa15a00051eaef862e4cb2724f2f8fa568/arch/arm64/boot/dts/qcom/apq8053-lenovo-cd-18781y.dts), `compatible = "lenovo,cd-18781y", "qcom,apq8053", "qcom,msm8953"` | Source and runtime compatible checked |
| Kernel and build toolchain | [Reference kernel build notes](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/kernel/README.md) and [build script](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/kernel/build.sh): 6.19.5-r0, ARM64, LLVM; reference container uses clang 21.1.8 | Built offline with LLVM 18; source warnings and validation limits in NOTES.md |
| Boot layout | [Reference layout description](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/README.en.md#partitions-and-boot): stock boot chain → lk2nd → extlinux on an ext2 boot filesystem inside `system` | Migrated layout and boot update readbacks verified; Linux and systemd observed |
| Nested image geometry | [Reference image builder](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/image/build-image.sh): MBR boot start 2048, boot size 509952 sectors, root start 512000 | Bounded root/data mapping and persistent retention verified on hardware |
| Display and touch | Linux DTS contains FT8201 and HX83100A variants, 800 × 1280 touch geometry; panel selection preserves lk2nd's board fixups | FT8201 DSI connected and enabled at 800 × 1280, with MSM DRM framebuffer active. Physical touches generated evdev tracking and X/Y updates |
| GPU | Linux DRM MSM and A506 firmware path; Linux reads the signed ZAP path from the live device tree | With the owner's firmware staged in RAM, Mesa EGL reported `FD506` and read back the expected GPU-rendered pixel |
| Display power-off | [Reference session](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/kiosk/thinksmart-kiosk-session) disables idle blanking because DRM Off does not recover | Avoid DPMS/suspend during first light |
| USB networking | [Reference USB note](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/README.en.md#usb-network-after-flashing) reports unavailable USB device mode after handoff | No working recovery network assumed |
| Audio, Wi-Fi, Bluetooth, sensors, camera | Reference has device-specific drivers/configuration, but these are separate hardware gates | Not tested or claimed working |

The MIT reference supplies build and integration information. The Linux
kernel remains GPL-2.0-only; its board DTS retains BSD-3-Clause. Firmware,
calibration files, prebuilt modules, bootloaders and binary partition tables
are not copied into this repository. Firmware must come from the owner's
own legally obtained image/backup, outside Git, after its mapping and
licence have been reviewed.

## Boot and storage boundary

`BOOT_METHOD=lk2nd` is an explicit experimental backend. Existing Android
boot-image, flash, push, install and boot-update paths must refuse it. The
existing slot-B writer must never be repurposed to write unsuffixed `boot`
or `system`.

The offline image contains an MBR, `pmOS_boot` (ext2) and
`omamobile-root` (ext4). The initramfs protects every eMMC/SCSI block node,
including whole disks, boot partitions and loop devices, read-only. It
mounts the nested filesystems read-only and uses tmpfs for the writable
overlay.

An optional offline image uses a third nested partition after the root.
It preserves the planned outer `system` start (923648) and size
(14280671 sectors) on the 15269888-sector eMMC. No additional outer GPT
entry is needed. Only the `persistent` boot entry opens a linear
device-mapper target for that tail extent. lk2nd follows the extlinux
`DEFAULT` directive, so activate this entry in a local image copy after
the diagnostic stage is reviewed. The initramfs checks the board, non-removable
MMC, geometry, nested MBR entry and the generated ext4 UUID. The target must
take its backing reference before software read-only flags are set, so the
initramfs runs no shell prompt, services or filesystem mounts in that
interval. It applies the guard before mounting either filesystem. The
guard protects physical nodes and loop maps and leaves only the specific
data mapper writable. The Linux block layer may report a write to the
read-only backing device when the already opened mapper submits it.

The data filesystem contains the OverlayFS upper and work directories.
An image rebuilt with a new data filesystem starts with an empty upper.
Offline validation must cover repeated mounts, package and settings
retention, failed geometry/UUID checks, direct write rejection and a
byte comparison outside the mapped extent. The default diagnostic and
RAM-overlay entries still keep all block devices read-only.

If the guard cannot protect a newly visible device, it sets the panic
timeout to zero and triggers a kernel panic to halt all userspace. It
does not restart automatically; the human recovery path must be ready.
The default diagnostic entry has no USB gadget service. Display output
and UART access are unverified, so a diagnostic prompt is not a promised
working recovery console.

The kernel embeds the storage, loop, filesystem and console support needed
before modules are available. No new automatic firmware/driver bring-up is
part of this initramfs. The root must carry modules for the exact resulting
`kernel.release`. `panic=0` and no automatic recovery reboot prevent
single-slot reboot loops. `omamobile.initramfs` stops before root handoff for
console diagnosis.
Hardware watchdog behavior still needs its own attended gate; `panic=0`
controls software panic recovery and cannot establish how boot firmware's
watchdog behaves.

The project safety model has **no ThinkSmart entry** authorizing unsuffixed
`boot`, `system`, EDL or GPT writes. This plan adds no exception. An image
file produced on the laptop is not an approved installation procedure.
The current owner's separate local approval and checked backup/recovery
procedure are recorded under G4. A reusable public installer still needs
the policy gap resolved and its exact recovery commands reviewed. A dedicated
initramfs cannot protect storage while an external download-mode writer
runs.

## Gated runbook

Run each gate separately. Stop when any stop condition is met. All paths
below are examples outside the repository or in its ignored `out/` tree;
never put captures or proprietary files in a commit.

### G0: confirm identity and recovery (read-only; low risk)

Have the owner read the physical model label and describe the current OS,
whether lk2nd is installed, and whether the partition table was changed.
Do not probe an unknown address with `tools/phone-ssh`.

If the owner already has a Linux shell, ask them to save these read-only
facts locally, review them for identifiers, and share only redacted results:

```sh
uname -r
cat /proc/device-tree/compatible | tr '\000' '\n'
lsblk -o NAME,SIZE,RO,TYPE,FSTYPE,LABEL
findmnt -rn -o SOURCE,TARGET,FSTYPE,OPTIONS
```

Expect/Pass: CD-18781Y, matching board compatible, an understood storage
layout, and a documented way to recover using the buttons and the owner's
original images. Stop: another model, unknown layout, unavailable backup or
unverified recovery route. Rollback: none; these commands do not change
storage. Do not publish filesystem paths containing account names.

### G1: review backups and current boot files (read-only; low risk)

Review the owner's existing full eMMC backup and checksum manifest outside
Git. Verify that the backup belongs to this unit and includes the original
boot chain, GPT and device calibration. Inspect a copy of the owner's
`extlinux.conf`, kernel release and boot files; retain lk2nd's panel/touch
selection and record the current kernel command line privately.

Expect/Pass: checksums match, recovery is understood, board fixes and boot
arguments are known. Stop: incomplete backup, mismatched hashes, no current
boot configuration, or unclear lk2nd source/licensing. Rollback: none. No
backup-taking or restoration command is supplied yet: its exact read paths
depend on G0 and require separate review.

### G2: build a kernel on the laptop (offline; low risk)

Obtain the GPL kernel source and LLVM prerequisites on the laptop. Keep
them separate from the project's release-kernel worktrees. Pin the checkout
to `770e10fa15a00051eaef862e4cb2724f2f8fa568` and leave it clean. The
following command neither downloads code nor contacts a device:

```sh
tools/build-thinksmart-kernel \
  --source "$HOME/omamobile-kernel/wt-thinksmart" \
  --out "$HOME/omamobile-kernel/out-thinksmart-first" --jobs 1
```

`LLVM=-18` selects a suffixed host toolchain when needed. For a configuration
check alone, use `--config-only` with another empty output directory.

Expect/Pass: `Image.gz`, `qcom/apq8053-lenovo-cd-18781y.dtb`, a matching
`modules/` tree, `modules-<release>.tar.zst` and `kernel.release`; all requested early-boot config values
survive `olddefconfig`. The builder merges the pinned tree's own arm64
`defconfig`, `msm8953.config`, and this device's small config fragment. Its
`W=1` build log must be reviewed before use.

Stop: unexpected source revision, nonempty output directory, config
mismatch, compiler error or new relevant warnings. Rollback: keep the failed
output for diagnosis and choose another empty output directory. Do not
replace another agent's build. Before hardware use, run the kernel's
`dtbs_check`/`dt-validate` with the matching schema toolchain and inspect all
board warnings; kernel edits also need `checkpatch --strict`.

### G3: build and inspect the offline system image (low risk)

Build the ThinkSmart device package and an Arch Linux ARM root with the
matching kernel modules. Set `KERNEL_OUT` to the G2 directory when running
`tools/build-rootfs --local thinksmart --keep-raw`; the rootfs builder consumes
its `modules-<release>.tar.zst`. Supply a static ARM64 BusyBox for the
initramfs. After the local package repository and root are built, assemble
the image using paths to those local artifacts:

```sh
tools/build-thinksmart-image \
  --kernel "$HOME/omamobile-kernel/out-thinksmart-first/arch/arm64/boot/Image.gz" \
  --dtb "$HOME/omamobile-kernel/out-thinksmart-first/arch/arm64/boot/dts/qcom/apq8053-lenovo-cd-18781y.dtb" \
  --config "$HOME/omamobile-kernel/out-thinksmart-first/.config" \
  --rootfs "out/thinksmart/omamobile-rootfs-thinksmart-<version>-aarch64.img" \
  --busybox /OFF_REPO/busybox-static-aarch64 \
  --out out/thinksmart/image-first
```

Replace `<version>` and `/OFF_REPO` with actual local paths before running.
The output directory must not exist. The tool writes a regular file
`system.img`, never a block device. Optional `--firstboot-conf` embeds a
private configuration: keep the resulting image private, and do not add
its contents or the file to Git.

Expect/Pass: the resulting image has the nested geometry above; its
extlinux entry references the supplied kernel, board DTB and initramfs,
uses this initramfs's root lookup, and contains `panic=0`. The default
extlinux entry stops at the diagnostic shell with `omamobile.initramfs`;
the second entry explicitly selects the RAM-overlay handoff. Neither entry
sets up USB networking. Inspect its
manifest and compare all staged files to the supplied inputs. The root
must contain the matching kernel module directory and the ThinkSmart
package, with no automatic DPMS/suspend or unreviewed radio services.

Stop: missing inputs, mismatched module release, too-large image, a writable
physical node at handoff, data writes outside the tested mapper, automatic
driver experiments, or identifiers in the image.
Rollback: retain or discard the regular output file; no device changes.
Record checks and their actual results in NOTES.md, without claiming that
an image-format test proves a boot.

### G4: hardware authorization boundary (high risk; local run authorized)

For the current owner's unit only, the owner separately approved the full
backup, calibration relocation, both GPT writes and installation, then
authorized computer-side progression while retaining control of physical
hardware actions. Those writes and their readbacks are recorded in NOTES.md.
The compat2 display-handoff fix was installed and fully read back;
protected regions were unchanged. The owner's next boot reached the
Linux diagnostic shell, but the owner's OTG keyboard produced no input.
The USB serial diagnostic boot slice has been installed after live
identity/readback checks and fully read back; GPTs, calibration and lk2nd
were unchanged. The owner's boot log reaches the UDC but reports an ACM
association error. The v2 correction to the helper's relative-path bug
passed offline checks and has been installed after live verification, with
the entire boot slice read back and protected regions unchanged. The
owner's next boot initially produced a Windows device-descriptor request
failure. After full power removal, the display recovered and CDC ACM
enumerated. Live serial commands now verify the kernel, configured UDC,
running guard, read-only MMC nodes and the bounded read-only root mount.
The working diagnostic slice is retained for rollback. The persistent USB
checkpoint candidate has since passed offline checks, was installed after
fresh identity and protected-region checks, and was completely read back.
Protected regions remained unchanged. It preserves the control path and
stops for inspection of the bounded persistent mapper before handing off
to userspace. The owner's cold boot has now verified the mapper, guarded
mounts and probe-file retention across a remount. All filesystems were
cleanly unmounted and all block nodes made read-only again. The current
boundary has advanced: the owner's cold boot retained both probe files.
A gated systemd handoff initially failed because both USB console
preludes exited on `stty` failure. After the owner's next cold boot,
journal recovery and the existing probes checked out. A RAM-only
correction now reaches systemd with working serial command roundtrips
and an active guard. A separate serial connection confirms control.
The later nftables kernel boot update embeds that console correction and
has passed a new cold boot and systemd handoff without a RAM patch.
Software firstboot and application cold-boot retention now pass. Hardware
coldplug and graphical login remain runtime-masked. The public installer policy below
remains unresolved; the local approval is not a reusable device exception.

There is no generally authorized flash/boot command in this plan. For a
reusable installation workflow,
the maintainer must approve a ThinkSmart safety-model entry, exact allowed
partitions, serial/model pinning, an attended dry run, backup checks,
buttons-only recovery and restoration steps. The human must approve each
reboot, flash, overlay boot and irreversible write. Full eMMC/GPT writes
are not made safe by being described in the reference repository.

Expect/Pass: separately reviewed, device-specific authorization and runbook.
Stop: any missing prerequisite. Rollback: do not write the device.

### G5: first-light validation (high risk; diagnostic output observed)

The attended test has reached `omamobile.initramfs` console mode and its
system/persist read-only marker on `6.19.5-g770e10fa15a0`. USB serial input,
read-only root mounting, matching modules and package inventory have now
been verified. The owner's separately authorized persistent entry has also
passed live mapper-boundary, protected-node and remount-retention checks.
Cold-boot probe retention has passed. The later installed kernel
`6.19.5-thinksmart1-g770e10fa15a0` provides an interactive systemd USB
console through the embedded correction and an active guard. Its nftables
API resolves the firstboot failure: service exit zero, done marker present,
private configuration removed. The signed test package, all matching
module files and the existing sandboxed headless Chromium test profile
survive the owner's cold boot. Desktop/input remain unverified; the last
OTG keyboard observation was unpowered and unresponsive. The image still
requires host-controlled checkpoint and userspace release.
Announce that the screen will change before the human starts it. Record
`uname -r`; verify every persistent disk, partition and loop is read-only;
inspect mounts without write tests. Verify a console/recovery path before
attempting handoff to the root in a subsequent approved boot.

Expect/Pass: console works, guard holds and nested root opens read-only.
The generic diagnostic/RAM entry has no persistent writer. For this owner's
authorized persistent run, only the exact bounded data mapper may back the
writable overlay; raw disks and partitions remain read-only. Stop: an unexpected persistent
read-write node, kernel oops, inaccessible console or missing recovery path.
Rollback: human follows the approved G4 recovery runbook; no automatic
reboot. A 2 GB device may run out of RAM with a large overlay, so diagnostic
and RAM-overlay first light must avoid package installation. The bounded
persistent overlay has separately passed the runtime write and cold-boot
probe gates above; software tests there must retain the guard and control path.

Only after that gate: one subsystem per approved boot, log stream before
driver loads, a verified watchdog/dead-man procedure where appropriate,
and no register pokes. Test display/touch before GPU/Hyprland, then Wi-Fi,
Bluetooth, audio and sensors. Keep the camera and the VCNL4200 kernel driver
disabled until separately reviewed. Do not test DPMS off or suspend until
panel recovery has its own gated plan. No feature is marked working until
the dated result includes its actual kernel release.

### G6: prepare display and touch recovery (offline/read-only first)

The 2026-11-14 inventory on `6.19.5-thinksmart1-g770e10fa15a0` shows a
working simplefb console but no `/dev/dri` card. MDSS, GPU and the selected
FT8201 I2C node are unbound; only gpio-keys has an input handler. Keep the
existing runtime masks until individual driver probes have a recovery
path. No broad udev trigger or graphical-login start is part of this gate.

Offline source inspection maps the selected `focaltech,ft8201` compatible
to the pinned `edt-ft5x06` driver; the matching module is built. This is
not a successful touch probe. The owner's offline stock boot bundle also
contains a downstream watchdog node consistently across its DT candidates.
It uses a different binding from mainline `qcom-wdt`, without an explicit
clock reference. Source comparison and the pinned binding now support an
offline bootstrap candidate; its full validation and attended recovery
sequence are in [WATCHDOG-PLAN.md](WATCHDOG-PLAN.md). Clock timing, reset
and mainline compatibility still require hardware confirmation. No DT
change has been installed. Raw stock DTs remain outside Git.

The subsequent watchdog candidate has now been written and completely read
back on the owner's unit. Its cold boot, feed/magic-close test and actual
timeout reset back to the checkpoint pass, with a changed boot ID and
unchanged protected hashes. Host-lease renewal/disarm and RAM-only recovery
now pass. Lease expiry led to normal overlay and data unmounts, read-only
storage and an automatic reboot without ext4 journal replay. All 21 checks
passed after the reboot. The first touch trial can use the proven recovery
path while keeping display and GPU coldplug held.

Offline display dependency review found a parameter-name trap in the pinned
MSM module: `adreno_device.c` declares `module_param(skip_gpu, ...)`, while
the adjacent description calls it `no_gpu`. The actual parameter for an
initial display-only probe is `skip_gpu=1`; do not use `no_gpu=1`. This
suppresses Adreno platform-driver registration and is only an isolation
step, not a replacement for later GPU and full desktop validation.
The FT8201 panel probe also requests its reset GPIO as output-high before
attaching to DSI. Module insertion can therefore affect the visible screen
even before a modeset; announce it and arm the recovery mechanism first.
These are source findings, not successful device probes. Keep normal
coldplug held and inspect the actual selected panel node before choosing
the matching panel module.

The installed configuration also lacks `CONFIG_REGULATOR_QCOM_LABIBB`.
The selected panel's `vsp`/`vsn` supplies refer to those regulators, so the
existing panel module alone cannot establish a complete display path.
An unchanged pinned-source regulator module has been built in an isolated
external-module directory against the exact installed output, with `W=1`;
no module was installed or loaded. Its later PMIC probe requires refreshed
protected backups/hashes, the completed recovery gate and a separate
single-driver trial. Do not bypass regulator dependencies with dummy supplies.

1. Inspect the pinned driver source, DT dependencies and module dependency
   list offline. Do not infer this unit's touch variant from another unit.
2. Establish the watchdog gate before loading a new hardware driver. The
   project stage tools' `--watchdog` uses a real watchdog device. The current
   attended candidate provides the verified Qualcomm watchdog, loaded
   explicitly before a trial. Use the bounded lease and exitrd recovery
   procedure in [WATCHDOG-PLAN.md](WATCHDOG-PLAN.md).
3. Prepare a host log stream, a bounded single-driver operation and a
   verified dead-man recovery for any step that could cut USB. Preserve
   the current boot slice and host checkpoint as recovery. A userspace
   timer alone does not recover a fully hung kernel.
4. Announce screen changes before an attended display test. Start with
   one display or touch change, inspect logs and prove a fresh USB command
   roundtrip. Only a later gate may start the compositor or test OTG.

Expect/Pass: reviewed driver dependencies and a tested recovery mechanism
for the selected probe. Stop: no watchdog/recovery, missing logs, new I/O
errors, guard failure, loss of USB or an unexpected driver binding.
Rollback: no device change during preparation; later driver trials need
their own explicit undo and the owner's physical recovery path. Avoid
DPMS off and suspend; retain the disabled camera and VCNL4200 policy.
