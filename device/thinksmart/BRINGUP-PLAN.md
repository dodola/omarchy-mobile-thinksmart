<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart View CD-18781Y: first-light plan

Status (2026-11-11): experimental offline port. No device has been connected,
booted or flashed by this work. Kernel source: `msm8953-mainline/linux`
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
| Board identity and device tree | [Linux board DTS at the pinned revision](https://github.com/msm8953-mainline/linux/blob/770e10fa15a00051eaef862e4cb2724f2f8fa568/arch/arm64/boot/dts/qcom/apq8053-lenovo-cd-18781y.dts), `compatible = "lenovo,cd-18781y", "qcom,apq8053", "qcom,msm8953"` | Source checked; hardware untested |
| Kernel and build toolchain | [Reference kernel build notes](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/kernel/README.md) and [build script](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/kernel/build.sh): 6.19.5-r0, ARM64, LLVM; reference container uses clang 21.1.8 | Built offline with LLVM 18; source warnings and validation limits in NOTES.md |
| Boot layout | [Reference layout description](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/README.en.md#partitions-and-boot): stock boot chain → lk2nd → extlinux on an ext2 boot filesystem inside `system` | Source checked; owner's layout unknown |
| Nested image geometry | [Reference image builder](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/image/build-image.sh): MBR boot start 2048, boot size 509952 sectors, root start 512000 | Source checked; image format only, no permission to write it |
| Display and touch | Linux DTS contains FT8201 and HX83100A variants, 800 × 1280 touch geometry; panel selection must preserve lk2nd's board fixups | Hardware untested |
| GPU | Linux DRM MSM/MDP5 and A506 firmware path; reference carries freedreno fixes in [Mesa build script](https://github.com/justhalfbit/thinksmart-ubuntu-image/blob/d2e32393b30dfcc8397192747fced0fb7115fc33/mesa/build-mesa.sh) | Hyprland untested |
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
overlay. The initial root does not have a persistence allowlist.

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
The owner and maintainer must resolve that policy gap and review exact
backup/recovery commands before any first boot or flash. A dedicated
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

Stop: missing inputs, mismatched module release, too-large image, persistent
write enablement, automatic driver experiments, or identifiers in the image.
Rollback: retain or discard the regular output file; no device changes.
Record checks and their actual results in NOTES.md, without claiming that
an image-format test proves a boot.

### G4: hardware authorization boundary (high risk; blocked)

There is no authorized flash/boot command in this plan. Before advancing,
the maintainer must approve a ThinkSmart safety-model entry, exact allowed
partitions, serial/model pinning, an attended dry run, backup checks,
buttons-only recovery and restoration steps. The human must approve each
reboot, flash, overlay boot and irreversible write. Full eMMC/GPT writes
are not made safe by being described in the reference repository.

Expect/Pass: separately reviewed, device-specific authorization and runbook.
Stop: any missing prerequisite. Rollback: do not write the device.

### G5: first-light validation (high risk; pending G4)

The future attended test starts in `omamobile.initramfs` console mode.
Announce that the screen will change before the human starts it. Record
`uname -r`; verify every persistent disk, partition and loop is read-only;
inspect mounts without write tests. Verify a console/recovery path before
attempting handoff to the root in a subsequent approved boot.

Expect/Pass: console works, guard holds, nested root opens read-only and
RAM overlay is the sole writable root layer. Stop: an unexpected persistent
read-write node, kernel oops, inaccessible console or missing recovery path.
Rollback: human follows the approved G4 recovery runbook; no automatic
reboot. A 2 GB device may run out of RAM with a large overlay, so first
light must avoid updates and package installation in the running system.

Only after that gate: one subsystem per approved boot, log stream before
driver loads, a verified watchdog/dead-man procedure where appropriate,
and no register pokes. Test display/touch before GPU/Hyprland, then Wi-Fi,
Bluetooth, audio and sensors. Keep the camera and the VCNL4200 kernel driver
disabled until separately reviewed. Do not test DPMS off or suspend until
panel recovery has its own gated plan. No feature is marked working until
the dated result includes its actual kernel release.
