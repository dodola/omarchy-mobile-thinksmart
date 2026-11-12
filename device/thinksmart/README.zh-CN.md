<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart View 离线移植

目标仅为 **Lenovo ThinkSmart View CD-18781Y / APQ8053 / 2 GB / 8 GB eMMC**。
当前是实验性离线实现，尚无真机启动结果。源码依据、风险、停止条件及恢复要求见
[BRINGUP-PLAN.md](BRINGUP-PLAN.md)，实际验证结果见 [NOTES.md](NOTES.md)。

系统路线为 Arch Linux ARM → Omarchy / Hyprland，保留设备现有的
lk2nd → extlinux 启动方式。它不复用 Pixel 的刷写路径，也不转换已有系统。
默认镜像的根目录使用最大 512 MiB 的内存覆盖层。另有可选的持久化镜像，
其持久写入范围仅限于 `system` 内的 ext4 数据区。

## 构建顺序

以下命令均在电脑运行。依赖包括 Git、LLVM（至少 15）、Linux 内核构建依赖、
Docker、可用的 aarch64 binfmt/QEMU、GnuPG、zstd、dtc 和 e2fsprogs。
需要提前下载依赖、软件包和源码；“离线”在这里指不连接或操作目标设备，
首次准备并不保证断网可完成。

1. 在仓库外准备固定版本的 GPL 内核源码，单独输出构建结果：

   ```sh
   git clone --depth 1 --branch v6.19.5-r0 \
     https://github.com/msm8953-mainline/linux.git \
     "$HOME/omamobile-kernel/wt-thinksmart"
   tools/build-thinksmart-kernel \
     --source "$HOME/omamobile-kernel/wt-thinksmart" \
     --out "$HOME/omamobile-kernel/out-thinksmart-first" --jobs 4
   ```

   构建器校验固定提交、配置和输出目录，执行 `W=1` 编译并生成匹配的模块包。
   如使用带后缀的工具链，可在命令前加 `LLVM=-18`；仅检查配置时加
   `--config-only`，并另选一个空输出目录。

2. 建立本地签名仓库。`local-key-init` 只运行一次；已有密钥时跳过它。
   密钥仅保存在忽略的 `keys/thinksmart/`，不要上传。导入项目已发布的签名包，
   再构建包含 Freedreno 的 Mesa 和本设备包：

   ```sh
   tools/local-key-init thinksmart
   tools/fetch-published --channel development out/thinksmart/published
   tools/build-packages --local thinksmart --import out/thinksmart/published \
     packages/mesa packages/omamobile-device-thinksmart
   ```

   旧的项目 Mesa 包没有 Freedreno，不能作为替代。新设备包声明了最低版本要求。
   参考系统的音频策略、固件和私有数据不会自动复制进来。

3. 用相同内核模块构建根文件系统，保留未压缩的 ext4 文件：

   ```sh
   KERNEL_OUT="$HOME/omamobile-kernel/out-thinksmart-first" \
     tools/build-rootfs --local thinksmart --keep-raw --version firstlight
   ```

   ThinkSmart 仍安装 Omarchy 桌面和移动端软件包，并从桌面默认清单中省去
   LibreOffice、Obsidian、Kdenlive、OBS Studio、Docker、Clang/LLVM 工具链、
   Mise、Aether、Cliamp 和 Pinta/.NET。
   具体名单见本目录的 `device.conf`。通用构建器为根镜像预留了空闲空间，
   ThinkSmart 可以将同一套文件重新装入 6,656 MiB 的 ext4 镜像。以下命令
   在 Linux 电脑上只读挂载构建产物，并在新文件中生成紧凑镜像：

   ```sh
   source_img=out/thinksmart/omamobile-rootfs-thinksmart-firstlight-aarch64.img
   compact=$(mktemp -d out/thinksmart/compact.XXXXXX)
   mkdir "$compact/source"
   truncate -s 6656M "$compact/rootfs.img"
   sudo unshare -m --propagation private bash -c '
     set -eu
     mount -t ext4 -o loop,ro,noload "$1" "$3"
     mkfs.ext4 -q -F -b 4096 -L omamobile-root -m 1 -d "$3" "$2"
   ' _ "$source_img" "$compact/rootfs.img" "$compact/source"
   e2fsck -fn "$compact/rootfs.img"
   ```

4. 在仓库外准备具有 `mount`、`losetup`、`blockdev`、`switch_root` 等 applet
   的静态 aarch64 BusyBox。其 `losetup` 必须支持 `-P`，用于扫描只读 loop
   设备中的分区。然后生成一个新的输出目录：

   ```sh
   tools/build-thinksmart-image \
     --kernel "$HOME/omamobile-kernel/out-thinksmart-first/arch/arm64/boot/Image.gz" \
     --dtb "$HOME/omamobile-kernel/out-thinksmart-first/arch/arm64/boot/dts/qcom/apq8053-lenovo-cd-18781y.dtb" \
     --config "$HOME/omamobile-kernel/out-thinksmart-first/.config" \
     --rootfs "$compact/rootfs.img" \
     --busybox /absolute/path/to/busybox-static \
     --out out/thinksmart/firstlight-image
   ```

   输出包括 `system.img`、`boot/`、`MANIFEST.json` 和 `SHA256SUMS`。
   `system.img` 是 `system` 分区内部的嵌套镜像，**不是整盘镜像**。
   默认启动项是只读诊断环境，`omamobile` 条目使用内存覆盖层。
   诊断环境没有 USB 网络服务；UART、屏幕及输入均有待真机验证。

   裸根镜像没有用户。后续要验证桌面时，可在打包命令中显式加入
   `--firstboot-conf /absolute/private/firstboot.conf`；格式及校验器见
   `packages/omamobile-firstboot/bin/omamobile-firstboot`。
   文件会包含在 initramfs 内，整个输出应视为私人文件，不能发布。
   首次设置和账户资料写入所选的覆盖层。

## 持久化镜像

用带 ARM64 Linux sysroot 的静态工具链编译小型设备映射程序：

```sh
aarch64-linux-gnu-gcc -O2 -Wall -Wextra -static \
  device/thinksmart/initramfs/map.c -o /absolute/path/to/thinksmart-map
```

给上面的打包命令加入 `--persistence-mapper /absolute/path/to/thinksmart-map`，
并指定新输出目录。内核必须内置 `CONFIG_BLK_DEV_DM=y`。
打包器在只读根之后创建第三个嵌套分区，并填入一个空的 ext4 数据文件系统。
默认仍进入只读诊断，普通 Omarchy 仍用 RAM。lk2nd 按 extlinux 的
`DEFAULT` 启动。要使用持久层，在电脑上的镜像副本中将嵌套启动区的
`/extlinux/extlinux.conf` 设为 `DEFAULT persistent`。运行时验证整盘大小、
`system` 起点与长度、嵌套数据区边界和 ext4 UUID。检查失败会停止启动。

持久层保存 `/home`、`/etc` 及软件包数据库等根目录更改。
写入一份新打包的完整 `system.img` 会覆盖这个数据区。

## 验证与硬件边界

相关离线测试：

```sh
packages/omamobile-device-thinksmart/test/all
tests/thinksmart-rootfs
tests/thinksmart-image
tests/boot-method
```

这些检查覆盖设备包、构建参数、真实镜像文件系统结构、容量与 ABI 检查，以及
现有刷写工具的拒绝行为。它们不证明设备能启动，也不验证触摸、Adreno 加速、
Wi-Fi、USB、音频、蓝牙、传感器或摄像头。

当前项目安全规则没有授权此设备的 `boot`、`system` 或 GPT 写入。因此这里不提供
刷写命令；拿到设备后先做只读信息和备份核对，再审查它专用的安装与恢复流程。
