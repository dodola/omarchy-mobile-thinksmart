<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart lk2nd compatibility build

The newest diagnostic build is `20.0-thinksmart-compat2`. It applies
both patches in this directory. Its framebuffer fix has passed the host
checks below and the owner's installed payload has been fully read back.
The subsequent hardware boot reached Linux's simplefb console and the
read-only diagnostic shell; root/desktop startup remains unverified.

The owner's original `20.0-next-b9c007bb5492-20250123` loader reaches its
menu, but cannot boot the generated Omarchy image. Its extlinux parser
matches lowercase commands only and does not select `DEFAULT` labels.
The image uses uppercase commands and multiple labels. A host build of
that exact parser leaves the kernel, DTB and initramfs unset for the real
configuration. Simply changing case would let the last entry override
diagnostic mode and is not a valid fix.

There is also a separate size limit. The original loader reserves 32 MiB
for the kernel. The current kernel file is 40,643,072 bytes, with a
41,222,144-byte runtime span from its ARM64 header, so it cannot fit.

`0001-extlinux-compat.patch` changes only `lk2nd/boot/extlinux.c` on
[the installed version's source](https://github.com/msm8916-mainline/lk2nd/tree/b9c007bb549229da15e9b2b35f19f82fa4d07967).
It backports the corresponding file from
[upstream cf45aefd](https://github.com/msm8916-mainline/lk2nd/blob/cf45aefd22d744b2bf9b39455afae697ce430063/lk2nd/boot/extlinux.c),
including case-insensitive commands, label/default selection, relative
paths and flexible kernel/initramfs placement. The two backport adaptations
are the upstream 50 MiB boot-memory default in this file and an exact
`strncasecmp` comparison including the terminator, since the old libc does
not implement `strcasecmp`. Platform, board and panel sources stay unchanged.
The upstream file retains its BSD-3-Clause copyright and licence.

## Compat1 offline build

Use a dedicated source tree at the exact base revision and an empty output
directory. The build requires `arm-none-eabi-gcc`, matching binutils, dtc,
libfdt headers and Python with pyasn1 modules and pycryptodome. Apply the
patch, then build from the lk2nd source directory:

```sh
patch -p1 < /PATH/TO/0001-extlinux-compat.patch
make -j2 TOOLCHAIN_PREFIX=arm-none-eabi- \
  SIGN_BOOTIMG=1 LK2ND_QCDTBS= \
  LK2ND_VERSION=20.0-thinksmart-compat1 DEBUG_FBCON=1 \
  BOOTLOADER_OUT=/DEDICATED/OUTPUT lk2nd-msm8953
```

AVB1 signing and the empty QCDT setting follow this base revision's
`apq8053-lenovo-cd-18781y.dts` instructions. Do not unlock or replace the
stock bootloader. `DEBUG_FBCON=1` makes subsequent boot failures visible
on the screen. It does not establish a working Linux framebuffer.

On this host, Ubuntu's packaged Python library is named `Cryptodome`,
while the old signing script imports `Crypto`. A separate host-side copy
of `bootsignature.py` with those imports renamed signed the generated
image, using the source tree's bundled AVB1 test certificate/key and
`/boot` target. The normal build's `SEANDROIDENFORCE` trailer was appended.
No target code was changed for this host dependency adjustment.

## Validation and limits

For compat1 on 2026-11-13:

- Compared every original archive member with the build source: only
  `lk2nd/boot/extlinux.c` differed.
- Compiled the original and backported C parsers on the host. The original
  fails to identify any boot files in the real diagnostic configuration;
  the backport selects its diagnostic arguments. The persistent slice
  separately selects `omamobile.persist`.
- Checked the current diagnostic initramfs (1,461,002 bytes) and kernel
  runtime span against the new memory layout. The kernel has 7,647,232
  bytes of space before the reserved DTB region.
- Verified the new AVB1 signature independently with strict RSA PKCS#1
  v1.5 verification, the authenticated `/boot` target and payload length,
  and the source tree's certificate. Boot header addresses match the
  previously booted lk2nd. The padded image is below the 512 KiB lk2nd
  reservation.
- The owner's separately authorized replacement was fully read back and
  matched SHA-256. Both GPTs and calibration readbacks were unchanged.
  A successful Linux boot remains unverified.

Build inputs are public; device images, calibration hashes and actual
write/readback records remain in the private installation package. This
document is an offline build recipe, not a general flashing authorization.
After any kernel/initramfs change, repeat the memory-layout check using
the decompressed file size and ARM64 `image_size`, not the gzip size alone.

## Existing framebuffer reservation (compat2)

After compat1 and the diagnostic clock-preservation parameters, the owner
observed a stable screen ending in `Jumping to kernel via monitor`. The
same screen shows successful diagnostic-entry selection, decompression
and DT updates. This is the last bootloader message, not evidence that
Linux reached its console. The SD slot2 probe errors precede a successful
read of the nested eMMC boot filesystem.

The pinned Linux board DT already contains
`/reserved-memory/cont-splash@90001000`, reserving 800 x 1280 x 3 bytes.
The old lk2nd framebuffer updater unconditionally adds a node with the
framebuffer's address in its name. If that node already exists,
`fdt_add_subnode` fails and the function returns without publishing a
`simple-framebuffer` node. `lk2nd.pass-simplefb` alone cannot prevent this.

`0002-simplefb-reuse-reservation.patch` reuses an existing reservation only
when its start matches and its size covers the active framebuffer. It
preserves the reservation's size and phandle, and logs reuse and successful
publication. Incorrect start/size is rejected with a visible message. The
new-node path remains available when no reservation exists.

Apply this patch after 0001 and use the same build command with
`LK2ND_VERSION=20.0-thinksmart-compat2`. No platform, panel, kernel or
initramfs source changes are part of compat2.

Host tests compiled the actual old and modified updater functions, the
original command-line scanner and FDT helpers against libfdt. With the
actual shipped board DTB and its specified framebuffer geometry, the old
updater produced no framebuffer node; the new one published it. Tests also
covered a missing reservation, a larger existing reservation, too-small or
wrong-address reservations, a missing command-line flag, unchanged phandles
and repeat updates without changes. The device's live framebuffer address
has not yet been captured; the new on-screen log will establish whether
this reproduced case applies on hardware.

The candidate's AVB1 signature was strictly verified against the same
certificate as compat1. Boot header addresses match, and the padded image
still fits in the lk2nd reservation. Only extlinux.c and simplefb.c differ
from the base source archive.

On the owner's return to EDL, both GPTs, calibration, compat1 and the full
diagnostic boot slice matched their verified state. Only the lk2nd payload
was replaced. Its full readback matched compat2, and repeat reads of both
GPTs, calibration and the entire diagnostic boot slice were unchanged.
The complete old lk2nd range is retained privately for rollback. No reset
was sent. The owner's subsequent boot photo confirms the live framebuffer
address `0x90001000`, successful simplefb registration and Linux/initramfs
execution with the read-only diagnostic marker. See the dated hardware
observation in [../NOTES.md](../NOTES.md) for its scope and remaining gates.
