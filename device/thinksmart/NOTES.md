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
