# libcimbar-ios

An iOS-focused fork of [`sz3/libcimbar`](https://github.com/sz3/libcimbar) for receiving animated cimbar transfers on iPhone.

This repository keeps the upstream C++ barcode core and adds an iOS receiver app scaffold with a native SwiftUI interface, AVFoundation camera pipeline, Objective-C++ bridge, file persistence, and sharing flow.

## What this fork adds

Compared with upstream `libcimbar`, this fork adds:

- a reusable `src/lib/ios_recv/` C++ receive-session module
- a narrow C API for calling the receiver from Objective-C++ / Swift
- an XcodeGen-managed iOS app in `ios-app/`
- SwiftUI scanning UI with live camera preview
- completed-file persistence under Application Support
- in-app file list and share sheet flow

## Current status

Implemented in this repo:

- core frame processing and chunk decode pipeline
- fountain reconstruction and decompression
- C API wrapper for iOS integration
- SwiftUI app shell for scan / files / share UX
- AVFoundation camera capture pipeline
- local file persistence and received-file index

Known limitations:

- the iOS app has been scaffolded and wired up, but has **not yet been fully validated on a physical iPhone** in this environment
- desktop Linux builds for some viewer / GL targets may require system OpenGL ES headers (`GLES3/gl3.h`)
- the generated `.xcodeproj` is not committed; it must be generated on macOS from `ios-app/project.yml`

## Repository layout

- `src/lib/ios_recv/` — iOS-oriented receive-session core and C API
- `src/exe/cimbar_recv2/` — CLI receiver refactored to reuse the session abstraction
- `ios-app/` — XcodeGen-based iOS app project
- `docs/plans/` — implementation plan documents
- `docs/superpowers/specs/` — design/spec documents

## Quick start

### Build the C++ core on Linux

Install dependencies:

```bash
sudo apt install libopencv-dev libglfw3-dev libgles2-mesa-dev
```

Build:

```bash
cmake .
make -j7
make install
```

Run the iOS receiver unit target if available in your build tree.

### Run the iOS app on macOS with Xcode

See:

- [`ios-app/README.md`](ios-app/README.md)
- [`docs/ios/ios-app-macos-xcode-setup.md`](docs/ios/ios-app-macos-xcode-setup.md)

Short version:

```bash
brew install xcodegen
cd ios-app
xcodegen generate
open LibCimbarReceiver.xcodeproj
```

Then in Xcode:

1. select the `LibCimbarReceiver` target
2. set a unique bundle identifier
3. choose your Apple Development team for signing
4. run on a physical iPhone
5. grant camera access when prompted

## iOS app workflow

The intended receive flow is:

1. open the iOS app on an iPhone
2. point the camera at another device playing animated cimbar frames
3. let the decoder accumulate frames and reconstruct the payload
4. the completed file is saved locally and surfaced in the UI
5. share/export the file from the success sheet or Files tab

Stored files are written under the app's Application Support directory in a `ReceivedFiles/` folder, with metadata tracked in `index.json`.

## Upstream project background

`libcimbar` is an experimental high-density color barcode format for air-gapped transfer. The upstream project also includes:

- encoder web app: https://cimbar.org
- Android receiver app: https://github.com/sz3/cfc
- upstream library repo: https://github.com/sz3/libcimbar

## Original upstream build notes

The upstream C++ project depends on:

- OpenCV — `libopencv-dev`
- GLFW — `libglfw3-dev`
- OpenGL ES headers — `libgles2-mesa-dev`
- bundled third-party dependencies already vendored in the tree

For WebAssembly / cimbar.js, see [`WASM.md`](WASM.md).

## Additional references


- [`DETAILS.md`](DETAILS.md)
- [`PERFORMANCE.md`](PERFORMANCE.md)
- [`TODO.md`](TODO.md)
- [`ABOUT.md`](https://github.com/sz3/cimbar/blob/master/ABOUT.md)
