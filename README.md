# libcimbar-ios

> **中文** | [English](#english)

一款基于 iPhone 的彩色矩阵条码（cimbar）接收器，用于**完全离线的单向文件传输**——发送端屏幕播放动态条码动画，接收端 iPhone 扫描即可接收文件。无需 WiFi、蓝牙、数据线。

基于上游 [`sz3/libcimbar`](https://github.com/sz3/libcimbar) 的 iOS 移植，增加了 SwiftUI 界面、AVFoundation 相机管线、文件持久化和分享流程。

---

## 目录

- [cimbar 是什么](#cimbar-是什么)
- [系统组成与硬件需求](#系统组成与硬件需求)
- [快速上手](#快速上手)
- [扫描技巧](#扫描技巧)
- [理解进度界面](#理解进度界面)
- [常见问题排查](#常见问题排查)
- [显示兼容性说明](#显示兼容性说明)
- [传输速度参考](#传输速度参考)
- [技术支持](#技术支持)

---

## cimbar 是什么

cimbar 是一种**高密度彩色二维矩阵条码**。传统二维码每帧只能承载约 3KB，而 cimbar 使用 4 色或 8 色编码，单帧可达 **~7 KB** 有效载荷。通过连续播放动画帧（fountain 编码），可以传输任意大小的文件。

```
发送端 (电脑)                     接收端 (iPhone)
┌──────────────┐                ┌──────────────┐
│ 浏览器打开     │   动画条码流    │ 打开本 App    │
│ cimbar.org    │ ── 屏幕播放 ──→ │ 对准屏幕扫码  │
│ 拖入文件即可   │                │ 自动重组文件   │
└──────────────┘                └──────────────┘
      ↑                                ↓
   纯离线                            纯离线
   不经过网络                        不经过网络
```

**适用场景：**

- 从内网/隔离网向手机传文件（无需网络）
- 从无法插 U 盘的机器上提取文件
- 快速把电脑上的 PDF / 图片 / 压缩包传到手机
- 演示环境、安全审计、现场取证等

---

## 系统组成与硬件需求

### 发送端（播放条码的机器）

| 要求 | 说明 |
|------|------|
| 浏览器 | Chrome / Edge / Firefox 最新版 |
| 显示器 | **强烈推荐 Mac Retina 显示屏**（像素密度高，颜色准确） |
| 编码网站 | https://cimbar.org （或自建） |

> ⚠️ **Windows 显示器兼容性说明：** Windows 的 ClearType / 子像素渲染可能引入色偏和摩尔纹，部分 Windows 显示器扫码成功率明显低于 Mac 显示器。当前版本已有针对 Windows 显示器的多项优化（通道重对齐、重采样回退、居中裁切等），但成功率仍可能低于 Mac。详见 [显示兼容性说明](#显示兼容性说明)。

### 接收端（iPhone）

| 要求 | 说明 |
|------|------|
| 系统 | iOS 16.0+ |
| 设备 | iPhone XS 及以上（需要较好的摄像头） |
| 权限 | 相机权限（首次打开时授予） |

---

## 快速上手

### 第一步：编译安装 App

```bash
# 在 Mac 上
git clone git@github.com:goldenerr/libcimbar-ios.git
cd libcimbar-ios/ios-app

# 安装 xcodegen（如果没有）
brew install xcodegen

# 生成 Xcode 工程
xcodegen generate
open LibCimbarReceiver.xcodeproj
```

在 Xcode 中：
1. 选择 `LibCimbarReceiver` target
2. 设置唯一的 Bundle Identifier
3. 选择你的 Apple Developer 团队用于签名
4. 连接 iPhone，选择设备后点击 Run

### 第二步：发送端准备

1. 在电脑浏览器打开 https://cimbar.org
2. 拖入或选择要传输的文件
3. 浏览器开始播放彩色条码动画流
4. 把浏览器**全屏**（F11），调高屏幕亮度

### 第三步：iPhone 扫码接收

1. 打开 App，授予相机权限
2. 将 iPhone 对准电脑屏幕上播放的条码
3. 把条码放进屏幕中间的**绿色取景框**内
4. 调整距离使条码填满约 **55%–70%** 的取景框宽度
5. **保持稳定**——不要频繁移动手机

### 第四步：取走文件

扫描完成后：
- 弹出成功页面，显示文件名和大小
- 点击「Share」通过系统分享菜单发送（AirDrop / 微信 / 保存到文件等）
- 或点击「Keep Scanning」继续接收下一个文件
- 已接收的文件在「Files」标签页中可查看和再次分享

---

## 扫描技巧

### 基本技巧

| 技巧 | 说明 |
|------|------|
| 亮度 | 显示器调至最亮，关闭自动亮度 |
| 反光 | 避免屏幕上方有强光源直射反光 |
| 距离 | 条码在取景框中宽约 55%–70%，不要太近或太远 |
| 稳定 | 扫码时双手持机或放在桌面上，尽量不动 |
| 全屏 | 浏览器全屏播放条码，减少干扰 |

### 针对不同显示器

| 显示器类型 | 建议 |
|-----------|------|
| Mac Retina | 直接扫即可，成功率最高。条码填满约 70% 取景框 |
| Mac 外接显示器 | 同上，注意关闭 Night Shift / True Tone |
| Windows 显示器 | 条码填满约 55%–65% 取景框。如果持续「searching」，尝试调低显示器缩放比例（100%），或切换到更暗的显示模式 |
| 投影仪 | 一般不推荐，亮度/对比度不足 |

### 特定状态下的操作

- 进度条显示「Native: searching」→ 没有识别到条码，调整手机位置和距离
- 进度条显示「已锁定码框，但这一帧还没读出有效数据」→ 条码已识别但数据解不出，尝试微调距离或清洁屏幕
- 进度条显示「Try moving closer or improving lighting」→ 条码太小或太暗，靠近一些或调高亮度

---

## 理解进度界面

cimbar 使用 **fountain 编码**将文件拆分成多个 chunk，每个 cimbar 帧包含 **约 12 个 chunk**。

### 进度条显示的是帧数，不是 chunk 数

```
┌─────────────────────────────────────────┐
│  Decoding frame data…                   │
│  已扫描 30/73 帧（42%），累计 437 KB。    │
│                                         │
│  进度 42%                    还差 43 帧  │
│  ████████░░░░░░░░░░░░░░                │
│  已扫 30 帧              约需 73 帧      │
│  Native: decoded frame chunks           │
└─────────────────────────────────────────┘
```

### 文件大小与帧数对照表（Conf8x8 模式）

| 文件大小 | 约需帧数 | 预计耗时（理想条件） |
|---------|---------|-------------------|
| 1 KB | ~1 帧 | <1 秒 |
| 10 KB | ~3 帧 | 1–3 秒 |
| 100 KB | ~15 帧 | 5–15 秒 |
| 500 KB | ~65 帧 | 20–40 秒 |
| 1 MB | ~130 帧 | 40–80 秒 |
| 5 MB | ~650 帧 | 3–6 分钟 |

> **注意：** "约需帧数"包含了 fountain 冗余（约 2×），实际发送端生成的帧数可能是这个数字的两倍左右。进度条显示的是接收端的已收帧数 / 预计总帧数。

### 为什么进度条有时不动

少量帧不推进进度是正常的——fountain 编码有冗余，有些帧携带的是已收到的重复 chunk，不增加进度。如果进度条连续 **10 秒以上** 不推进，参考 [常见问题排查](#常见问题排查)。

---

## 常见问题排查

### Q: 一直显示 "Native: searching"，扫不到码

**可能原因与解决：**

1. 显示器亮度不够 → 调到最亮
2. 距离不对 → 调整使条码占取景框 55%–70%
3. 显示器类型不兼容 → Windows 显示器尝试调缩放比到 100%
4. 相机焦点偏移 → 用手指点击屏幕上条码位置让相机重新对焦
5. 屏幕反光 → 改变角度或遮挡环境光源

### Q: 进度条卡住不动

**正常情况：** fountain 冗余帧不增加进度，偶尔几秒不推进是正常的。

**非正常情况：** 连续 10 秒以上进度不变。

排查步骤：
1. 微调手机位置（前后移动 2–3 厘米）
2. 检查条码是否还在取景框内（对方可能滚出了画面）
3. 确认发送端还在播放（动画没有暂停）
4. 如果进度长时间完全不动，关闭 App 重新打开，让发送端也重新开始（刷新浏览器页面）

### Q: 扫出来的文件大小不对 / 文件损坏

- 发送端不要最小化浏览器窗口
- 接收全程保持手机稳定
- 如果反复失败，尝试降低发送端浏览器窗口的缩放比
- 某些显示器可能导致文件不完整，考虑换一台显示器重试

### Q: App 闪退或相机黑屏

- 确认 iOS 版本 ≥ 16.0
- 确认在「设置 → 隐私 → 相机」中已授权
- 尝试重启 App

### Q: 为什么有些 Windows 电脑完全扫不到

这是一个已知的兼容性问题。不同 Windows 显示器 / 显卡 / 缩放设置下，子像素渲染和色准差异很大。当前版本已包含多项针对 Windows 的优化，但仍有部分显示器组合无法正常工作。最可靠的方案是使用 Mac 显示器作为发送端。

---

## 显示兼容性说明

cimbar 依赖精确的颜色识别，显示器渲染差异会影响扫码成功率。

### 兼容性等级

| 等级 | 显示器类型 | 成功率 | 备注 |
|------|-----------|--------|------|
| ★★★ | MacBook 内置 Retina 屏 | 95%+ | 最佳选择 |
| ★★★ | Mac 外接 Studio Display / Pro Display XDR | 95%+ | |
| ★★☆ | 大部分 Mac 外接 4K 显示器（LG、Dell 等） | 80–90% | |
| ★★☆ | 高端 Windows 笔记本内置屏（Surface、XPS 等） | 70–85% | 调缩放比到 100% |
| ★☆☆ | 普通 Windows 外接显示器 | 40–70% | 依具体型号差异很大 |
| ☆☆☆ | 投影仪 | 不建议 | 亮度和对比度不够 |
| ☆☆☆ | TN 面板低端显示器 | 不建议 | 颜色漂移严重 |

### Windows 显示器优化建议

如果 Windows 显示器扫码困难：

1. **显示缩放比设为 100%**（不要用 125% / 150%）
2. 关闭 ClearType（设置 → 调整 ClearType 文本）
3. 关闭 HDR（如果开启）
4. 使用 sRGB 色彩模式
5. 浏览器缩放 100%，全屏播放
6. 降低显示器色温（偏暖可减少蓝色子像素溢出）

---

## 传输速度参考

在 **Conf8x8 模式**下（当前默认 mode=68）：

| 指标 | 数值 |
|------|------|
| 单帧有效载荷 | ~7.4 KB（12 chunks × ~632 bytes/chunk） |
| 理想帧率 | ~3–5 fps（受 iPhone 处理速度限制） |
| 理想吞吐量 | ~22–37 KB/s |
| 1MB 文件预计耗时 | ~30–50 秒 |

实际速度受显示器质量、环境光线、手机稳定性等因素影响。

### 模式说明

| 模式 | 颜色数 | 单帧载荷 | 特点 |
|------|--------|---------|------|
| Conf4x4 (mode=64) | 4 色 | ~1.8 KB | 更快但载荷小，适合小文件 |
| Conf8x8 (mode=68, 默认) | 8 色 | ~7.4 KB | 平衡速度和可靠性 |
| Conf12x12 (mode=72) | 16 色 | ~16 KB | 载荷大但对显示器和相机要求更高 |

通常不建议修改默认模式，Conf8x8 在大部分场景下表现最佳。

---

## 技术支持

- **上游项目：** https://github.com/sz3/libcimbar
- **Android 接收端：** https://github.com/sz3/cfc
- **在线编码器：** https://cimbar.org
- **本仓库 Issues：** https://github.com/goldenerr/libcimbar-ios/issues

### 报告问题时请提供

1. iPhone 型号和 iOS 版本
2. 发送端操作系统和显示器型号
3. App 显示的具体状态文字（如 "Native: searching" / "Native: decoded frame chunks"）
4. 文件大小和扫描持续了多久
5. 如果有进度条截图或屏幕录像更好

---

# English {#english}

## What is cimbar

cimbar is a **high-density color 2D matrix barcode** for **completely offline, one-way file transfer** — the sender plays an animated barcode stream on screen, and the receiving iPhone scans it to reconstruct the file. No WiFi, Bluetooth, or cable needed.

Traditional QR codes hold ~3KB per frame. cimbar uses 4-color or 8-color encoding, achieving **~7 KB per frame**. By streaming consecutive frames (fountain encoding), files of arbitrary size can be transferred.

## Requirements

### Sender (the machine playing the codes)

- Browser: recent Chrome / Edge / Firefox
- Display: **Mac Retina strongly recommended** (best color accuracy and pixel density)
- Encoder: https://cimbar.org

> ⚠️ **Windows display note:** Windows ClearType / subpixel rendering can introduce color fringing and moiré, reducing scan success rates compared to Mac. Multiple Windows-specific optimizations are included (channel realignment, resample fallbacks, center crops), but some monitor configurations may still underperform.

### Receiver (iPhone)

- iOS 16.0+
- iPhone XS or newer
- Camera permission granted

## Quick Start

1. **Build the app** on macOS:
   ```bash
   brew install xcodegen
   cd ios-app && xcodegen generate && open LibCimbarReceiver.xcodeproj
   ```
   Select target, set Bundle ID and signing team, run on physical iPhone.

2. **Prepare the sender**: open https://cimbar.org in a desktop browser, drag in a file, full-screen the browser (F11), maximize brightness.

3. **Scan with iPhone**: point the camera at the animated code on screen. Center the code inside the green guide rectangle. Adjust distance so the code fills ~55%–70% of the guide width. **Hold steady.**

4. **Retrieve the file**: a success sheet appears when done. Tap "Share" to AirDrop / send, or "Keep Scanning" for the next file.

## Scanning Tips

- Max out display brightness, disable auto-brightness
- Avoid direct light sources reflecting off the screen
- Keep the code at ~55%–70% of the guide width
- Hold the phone steady — brace it on a surface if possible
- Browser should be full-screen

### For Windows Displays

- Set display scaling to 100%
- Disable ClearType and HDR
- Use sRGB color mode
- Keep the code slightly smaller in frame (~55%–65%)

## Understanding the Progress UI

cimbar uses **fountain encoding**, splitting a file into chunks. Each cimbar frame carries **~12 chunks**.

| File Size | ~Frames Needed | Estimated Time (ideal) |
|-----------|---------------|----------------------|
| 1 KB | ~1 | <1 sec |
| 10 KB | ~3 | 1–3 sec |
| 100 KB | ~15 | 5–15 sec |
| 500 KB | ~65 | 20–40 sec |
| 1 MB | ~130 | 40–80 sec |
| 5 MB | ~650 | 3–6 min |

> The "~Frames Needed" includes ~2× fountain redundancy. Actual frames emitted by the sender may be roughly double this number.

The progress bar shows **frames received / estimated frames required**, not raw fountain chunks. A 1MB file needing ~2000 fountain chunks will display as ~130–170 frames.

## Troubleshooting

### Stuck on "Native: searching"

1. Increase display brightness to maximum
2. Adjust distance — code should fill 55%–70% of the guide
3. Tap the screen where the code appears to refocus the camera
4. Eliminate screen glare from overhead lights
5. On Windows, try 100% display scaling

### Progress bar frozen

Brief stalls (a few seconds) are normal — duplicate fountain chunks don't advance progress. If stalled for 10+ seconds:
1. Nudge the phone slightly (move 2–3 cm forward or back)
2. Verify the code is still in frame
3. Check the sender is still playing (animation hasn't paused)
4. Restart both the app and the sender (refresh browser)

### File corrupted or wrong size

- Keep the sender browser visible and un-minimized
- Hold the phone steady throughout the entire scan
- Try a different display if failures persist

## Display Compatibility

| Rating | Display Type | Success Rate |
|--------|-------------|-------------|
| ★★★ | MacBook Retina (built-in) | 95%+ |
| ★★★ | Apple Studio Display / Pro Display XDR | 95%+ |
| ★★☆ | Most Mac external 4K (LG, Dell) | 80–90% |
| ★★☆ | Premium Windows laptop (Surface, XPS) | 70–85% |
| ★☆☆ | Standard Windows external monitors | 40–70% |

## Transfer Speed (Conf8x8 mode)

| Metric | Value |
|--------|-------|
| Payload per frame | ~7.4 KB |
| Effective frame rate | ~3–5 fps |
| Effective throughput | ~22–37 KB/s |
| 1MB file | ~30–50 seconds |

## Reporting Issues

Please include: iPhone model + iOS version, sender OS + display model, the exact status message shown in the app, file size, and how long you waited.

- **Upstream:** https://github.com/sz3/libcimbar
- **Android receiver:** https://github.com/sz3/cfc
- **Online encoder:** https://cimbar.org
- **Issues:** https://github.com/goldenerr/libcimbar-ios/issues
