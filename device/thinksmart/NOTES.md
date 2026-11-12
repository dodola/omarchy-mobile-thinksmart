<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart View CD-18781Y notes

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
