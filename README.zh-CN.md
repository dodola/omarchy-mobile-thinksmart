<!-- SPDX-License-Identifier: MIT -->
# ThinkSmart View 的 Omarchy Mobile 移植

[English](README.md)

本分支把 Arch Linux ARM、Hyprland 和 Omarchy Mobile 桌面移植到
**Lenovo ThinkSmart View CD-18781Y**。硬件为 Qualcomm APQ8053/MSM8953、
2 GB 内存、8 GB eMMC 和 800 × 1280 屏幕。目前在一台 FT8201 面板的设备上做了
实验性验证。开发镜像冷启动后仍需电脑通过串口放行启动检查点。

## 项目进度 · 2026-11-15

| 项目 | 当前结果 |
| --- | --- |
| 启动和首启 | lk2nd compat2 能按 extlinux 配置启动 Linux 6.19.5。受保护的持久化根文件系统可进入 systemd 并完成 Omarchy 首次配置。测试镜像冷启动后会等待主机放行。 |
| 持久化和应用 | ext4 覆盖层总容量约 **2.88 GiB**。已验证安装的软件包、设置和用户文件在冷启动后仍保留，一次实测约有 **2.1 GiB** 可用。离线镜像包含 Omarchy 桌面、移动端 shell 和 Chromium，并已在真机上验证 pacman 能安装签名软件包。 |
| 显示与 GPU | FT8201 面板以 800 × 1280 运行，使用 DSI 和 MSM DRM。使用从本机原厂备份提取的固件后，Mesa 识别 Adreno A506 为 `FD506`，EGL/GBM 绘制和像素回读通过。 |
| 桌面和触摸 | 正式 SDDM → UWSM → Omarchy 会话在临时 udev 配置下运行。点按命中屏幕上的目标，手指滑动可滚动 Wayland 窗口。 |
| 锁屏 | 保留登录后自动锁定和 PAM 认证。ThinkSmart 没有电源键，锁屏超过原熄屏时间后仍亮着，可触屏输入原有凭据解锁。已通过 SDDM 重登验证，整机冷启动复测待做。 |
| Chromium | 带沙箱的无界面测试确认浏览器 localStorage 能跨冷启动保留。带沙箱的原生 Wayland 窗口已用软件渲染打开。测试中 GPU 子进程退出，Chromium 硬件加速仍待验证。 |
| USB 和恢复 | 测试镜像的 USB 设备模式可在 Windows 上提供 CDC ACM 串口。诊断环境中的 OTG 键盘无响应，USB-C 主机模式待验证。看门狗和受控关机测试已恢复到检查点，存储校验通过。 |

接下来要把显示和触摸的临时配置纳入正常启动路径，验证冷启动直接进入桌面，再测试
USB 主机模式、休眠唤醒、Wi-Fi、蓝牙、音频、摄像头和传感器。
[真机验证记录](device/thinksmart/NOTES.md) · [看门狗恢复方案](device/thinksmart/WATCHDOG-PLAN.md)

## 构建与持久化

在 Linux 或 WSL2 上准备内核构建工具、ARM64 构建环境和本地签名仓库。依次构建固定在
`v6.19.5-r0` 的内核及匹配模块、设备软件包和精简的 Arch Linux ARM 根文件系统，
再用 `tools/build-thinksmart-image` 打包并核对 `MANIFEST.json` 和 `SHA256SUMS`。
完整构建命令、精简清单和离线测试见 [设备构建指南](device/thinksmart/README.zh-CN.md)。

| extlinux 启动项 | 用途 |
| --- | --- |
| `diagnostic` | 默认启动项，用于只读诊断。 |
| `omamobile` | 使用 RAM 覆盖层，断电后清除会话改动。 |
| `persistent` | 构建时加入映射工具后可用。把电脑上的镜像副本中的 extlinux `DEFAULT` 设为 `persistent`，可把软件包、配置和用户数据保存在 `system` 末尾的 ext4 数据区。 |

`system.img` 是写入 eMMC 的 `system` 分区的嵌套镜像。整份替换该镜像会覆盖其
数据区，更新前需要另行备份要保留的数据。
[启动与存储布局](device/thinksmart/BRINGUP-PLAN.md#boot-and-storage-boundary)

## 安装与恢复

真机迁移使用 Windows 下的 Qualcomm EDL / WinUSB，已对这台设备的整块原厂 eMMC
做了只读备份并核对 SHA-256。迁移前单独核对校准分区和主备 GPT，写入后回读验证。
更新启动区失败时，上一个完整回读过的启动区可以用于回退。迁移 GPT 后若需回到原厂，
使用该设备自己的整盘备份，按设备专用恢复流程操作。

当前仓库提供离线构建工具。ThinkSmart 的安装仍需按实机核对分区与恢复路径，
通用 Pixel 安装工具会拒绝该设备。整盘备份、本机 GPT 迁移文件、首启凭据和提取的
原厂固件应保存在本地。
[安装与恢复边界](device/thinksmart/BRINGUP-PLAN.md#g4-hardware-authorization-boundary-high-risk-local-run-authorized)
