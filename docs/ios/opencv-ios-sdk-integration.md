# OpenCV iOS SDK 获取与接入

这份说明面向当前仓库 `libcimbar-ios`，目标是解决这类错误：

- `opencv2/opencv.hpp file not found`
- OpenCV 相关 linker error

## 核心结论

这个项目的接收链路会直接编译 `src/lib/extractor`、`src/lib/cimb_translator`、`src/lib/ios_recv` 等 C++ 源码，而这些源码依赖：

```cpp
#include <opencv2/opencv.hpp>
```

所以你需要的是：

- **iOS 可用的 OpenCV SDK**
- 最好是 `opencv2.xcframework`

注意：

- `brew install opencv` 得到的是 macOS 用的库，不适合 iPhone target
- 你需要的是 **iOS / iPhoneOS / iPhoneSimulator** 对应的 framework 或 xcframework

---

## 推荐目录结构

建议把 OpenCV 放到仓库里这个位置：

```text
ios-app/Vendor/OpenCV/opencv2.xcframework
```

最终类似：

```text
libcimbar-ios/
  ios-app/
    Vendor/
      OpenCV/
        opencv2.xcframework/
```

这样后续：

- 路径稳定
- XcodeGen / Xcode 都容易配置
- 团队协作时也好说明

---

## 获取方式

## 方案 A：下载现成 iOS OpenCV 包

优先找包含以下平台 slice 的 OpenCV 包：

- `iphoneos`
- `iphonesimulator`

最好形式：

- `opencv2.xcframework`

如果下载到的是：

- `opencv2.framework`

也能尝试接入，但 `xcframework` 更适合现在的 Xcode / 真机 + 模拟器场景。

> 你要确认下载的是 **iOS** 版本，不是 macOS 版本。

---

## 方案 B：你自己已有 OpenCV iOS SDK

如果你手头已经有：

- `opencv2.xcframework`
- 或 `opencv2.framework`

直接复制到：

```bash
mkdir -p ios-app/Vendor/OpenCV
cp -R /path/to/opencv2.xcframework ios-app/Vendor/OpenCV/
```

或者：

```bash
cp -R /path/to/opencv2.framework ios-app/Vendor/OpenCV/
```

---

## 接入方式（先用 Xcode 手动接，最快）

在当前项目里，**最快的办法是先手动在 Xcode 里接入**，不要先纠结自动化。

### 1. 生成并打开工程

```bash
cd ios-app
xcodegen generate
open LibCimbarReceiver.xcodeproj
```

### 2. 把 OpenCV 拖进项目

把下面之一拖进 Xcode 左侧 Project Navigator：

- `ios-app/Vendor/OpenCV/opencv2.xcframework`
- 或 `ios-app/Vendor/OpenCV/opencv2.framework`

拖入时：

- 勾选 **Copy items if needed**（如果文件已经在仓库里，通常可不勾）
- 添加到 target：**LibCimbarReceiver**

### 3. 在 Target -> General 里确认

打开：

- `LibCimbarReceiver` target
- **General**

检查：

#### Frameworks, Libraries, and Embedded Content

确保 OpenCV 已出现在这里。

如果是 `.xcframework`：
- 一般会自动处理 slice
- Embed 通常按 Xcode 默认

如果是 `.framework`：
- 通常选择 `Embed & Sign` 或按 Xcode 推荐处理
- 具体取决于包的分发方式

### 4. 在 Build Settings 里确认 Header Search Paths（如果仍找不到头）

如果拖入后仍报：

```text
opencv2/opencv.hpp file not found
```

到：

- **Build Settings**
- 搜索 `Header Search Paths`

补充一条类似路径（按你实际 OpenCV 包结构调整）：

```text
$(SRCROOT)/Vendor/OpenCV/opencv2.xcframework/**
```

或者如果是 framework 内 Headers：

```text
$(SRCROOT)/Vendor/OpenCV/opencv2.framework/Headers
```

> 不同 OpenCV 分发包内部目录可能不同，所以这里以“你的实际包结构”为准。

---

## 建议验证顺序

接入后按这个顺序验证：

### 1. 清理旧缓存

```bash
rm -rf ~/Library/Developer/Xcode/DerivedData/LibCimbarReceiver-*
```

### 2. 重新生成工程

```bash
cd ios-app
xcodegen generate
```

### 3. 重新打开工程并 Build

如果此时：

- `opencv2/opencv.hpp` 消失了

说明 OpenCV 头文件路径已经接通。

下一步如果报的是某些 OpenCV 符号链接问题，再继续补 framework/link 配置。

---

## 常见误区

### 误区 1：用了 Homebrew 的 OpenCV

```bash
brew install opencv
```

这个只能解决 macOS 本地库问题，**不能直接给 iPhone target 用**。

### 误区 2：只加头文件不加 framework

只让 `#include <opencv2/opencv.hpp>` 不报错还不够，后面链接时还会需要对应二进制实现。

### 误区 3：只加了 iPhoneOS slice，没有 simulator slice

如果 OpenCV 包不包含 `iphonesimulator`，模拟器编译仍可能失败。

---

## 对当前项目的建议

### 最短路径

1. 先拿到 `opencv2.xcframework`
2. 放到：
   - `ios-app/Vendor/OpenCV/opencv2.xcframework`
3. 先在 Xcode 手动拖进去
4. 编译一次
5. 看新的第一条报错

### 为什么先手动接，不先改 XcodeGen

因为目前我们还不知道你拿到的 OpenCV 包具体是：

- `xcframework`
- `framework`
- 里面的 header 路径长什么样
- 是否需要额外系统 framework

先用 Xcode 手动接入，最快能验证路径和链接是否正确。
等跑通后，再把这一步固化回 `project.yml` 自动化。

---

## 接入完成后怎么继续

一旦你把 OpenCV 包放好并手动接入成功，下一步把新的报错贴出来。

最理想的是直接告诉我这两件事：

1. 你放进去的具体路径
   - 例如：`ios-app/Vendor/OpenCV/opencv2.xcframework`
2. 新的第一条报错

这样我就可以继续帮你把：

- Xcode 手动接入
- 再固化回 XcodeGen 配置
- 继续清下一批编译/链接错误

一直往下推进到能跑。
