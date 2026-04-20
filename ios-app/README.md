# LibCimbarReceiver iOS App

This directory contains the XcodeGen-managed iOS receiver app for this fork.

The app is intended to scan animated cimbar frames from another device, reconstruct transferred files, persist them locally, and expose them through a simple SwiftUI UI.

## Features in this scaffold

- live camera preview via `AVFoundation`
- Objective-C++ bridge into the C++ decoder session
- scan progress/status UI
- completed transfer success sheet
- persistent received-files list
- system share sheet for exporting completed files

## Project structure

- `project.yml` — XcodeGen source of truth
- `LibCimbarReceiver/App/` — app entry and state wiring
- `LibCimbarReceiver/Camera/` — camera capture and preview code
- `LibCimbarReceiver/Bridge/` — Swift / Objective-C++ decoder bridge
- `LibCimbarReceiver/Features/` — scan and files UI
- `LibCimbarReceiver/Storage/` — received file persistence

## Generate the project on macOS

```bash
brew install xcodegen
cd ios-app
xcodegen generate
open LibCimbarReceiver.xcodeproj
```

## Before first run in Xcode

Update the app identity/signing settings:

1. open the generated `LibCimbarReceiver.xcodeproj`
2. select the `LibCimbarReceiver` target
3. change `PRODUCT_BUNDLE_IDENTIFIER` to your own identifier
4. choose your Apple Development team under **Signing & Capabilities**
5. run on a physical iPhone with camera access

Current default values in `project.yml`:

- bundle prefix: `org.example`
- bundle identifier: `org.example.LibCimbarReceiver`
- iOS deployment target: `16.0`
- Swift version: `5.10`

## Important notes

- The generated `.xcodeproj` is intentionally not committed.
- Edit `project.yml` and source files, then regenerate the project.
- A real device is strongly recommended because the app depends on camera input.
- The flashlight button in the UI is currently a placeholder toggle; torch control is not yet wired up.
- The receive pipeline depends on **OpenCV**. After the recent XcodeGen fixes, the next likely blocker on macOS is adding an iOS-compatible OpenCV SDK/framework to the Xcode project.

## Full setup guide

For a fuller macOS/Xcode walkthrough, see:

- [`../docs/ios/ios-app-macos-xcode-setup.md`](../docs/ios/ios-app-macos-xcode-setup.md)
