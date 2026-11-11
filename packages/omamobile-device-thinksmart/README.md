<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart View bring-up policy

This package stages a conservative profile for the Lenovo ThinkSmart View
CD-18781Y. It is an offline port scaffold, not a qualified installation.

All eMMC block devices remain read-only. The initramfs must protect them
before systemd and use a RAM overlay for writable root state. This package
contains no boot writer, slot-success helper, firmware or audio service.
Suspend is masked; the first-light session disables idle blanking and the
phone power-key DPMS binding. Scale 1 is seeded once for the 800×1280 panel.

Build the new Mesa and device package in a contributor repository with
`tools/build-packages --local thinksmart packages/mesa packages/omamobile-device-thinksmart`.
After the remaining shared packages and matching kernel modules are ready,
`KERNEL_OUT=<build> tools/build-rootfs --local thinksmart` creates the bare
ext4 root image with its normal `omamobile-root` label.

The offline packer accepts regular files only:

```sh
tools/build-thinksmart-image --kernel <Image.gz> --dtb <board.dtb> \
  --config <kernel.config> --rootfs <uncompressed-rootfs.img> \
  --busybox <static-aarch64-busybox> --out <new-output-directory>
```

The optional `--firstboot-conf <private-file>` provisions the RAM overlay.
The resulting image keeps all block storage read-only; user changes vanish
at power loss. The generic installer deliberately rejects this device.
