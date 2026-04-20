# Running LibCimbarReceiver on macOS with Xcode

This guide explains how to generate, open, sign, and run the iOS receiver app from this repository.

## What you are building

The iOS app in this repo is a SwiftUI shell around the `libcimbar` receive pipeline:

- camera capture via `AVFoundation`
- Objective-C++ bridge into the C++ decoder
- receive progress/status UI
- persistence of completed files
- sharing/export of reconstructed files

## Prerequisites

You need:

- macOS with Xcode installed
- Xcode Command Line Tools
- Homebrew
- `xcodegen`
- an Apple Development signing identity
- ideally a physical iPhone running iOS 16+

Install XcodeGen:

```bash
brew install xcodegen
```

## 1. Clone the repo

```bash
git clone https://github.com/goldenerr/libcimbar-ios.git
cd libcimbar-ios
```

If you are already inside the repo, continue to the next step.

## 2. Generate the Xcode project

The committed source of truth is `ios-app/project.yml`, not a checked-in `.xcodeproj`.

```bash
cd ios-app
xcodegen generate
```

Expected result:

- `LibCimbarReceiver.xcodeproj` is created in `ios-app/`

If you later change `project.yml`, regenerate the project again with the same command.

## 3. Open the project

```bash
open LibCimbarReceiver.xcodeproj
```

## 4. Configure signing and bundle identifier

Inside Xcode:

1. select the **LibCimbarReceiver** project in the navigator
2. select the **LibCimbarReceiver** target
3. open the **Signing & Capabilities** tab
4. choose your Apple Development team
5. replace the default bundle identifier (`org.example.LibCimbarReceiver`) with your own unique identifier, for example:
   - `com.yourname.LibCimbarReceiver`
6. let Xcode manage provisioning automatically if available

If signing fails, it is usually because:

- the bundle identifier is still the placeholder
- no Apple Development team is selected
- the selected device does not trust the signing identity yet

## 5. Choose a run target

Recommended:

- a physical iPhone

Possible but less useful:

- iOS Simulator

Why physical device is recommended:

- the app depends on live camera input
- transfer testing is much easier when you can point the phone at another screen playing animated cimbar frames

## 6. Build and run

In Xcode:

1. choose the `LibCimbarReceiver` scheme
2. choose your iPhone as the run destination
3. press **Run**
4. accept the camera permission prompt on first launch

The app declares camera usage text in `Info.plist`, so iOS should prompt automatically.

## 7. Test the receive flow

Suggested test setup:

1. on another device, open an animated cimbar sender
   - for example, the upstream web encoder at https://cimbar.org
2. display the animated transfer on that device's screen
3. on your iPhone, open the Scan screen in `LibCimbarReceiver`
4. point the camera at the sending display
5. watch progress/status updates
6. once reconstruction completes, verify that:
   - a success sheet appears
   - the file is listed in the Files tab
   - share/export works

## 8. Where files are stored

Completed files are persisted under the app's Application Support directory.

Logical layout:

- `Application Support/ReceivedFiles/`
- `Application Support/ReceivedFiles/index.json`

Each completed payload is saved as its own file, and `index.json` tracks metadata for the file list UI.

## 9. Useful files to know

- `ios-app/project.yml` — XcodeGen project definition
- `ios-app/LibCimbarReceiver/App/AppState.swift` — app wiring and persistence integration
- `ios-app/LibCimbarReceiver/Camera/CameraCaptureService.swift` — camera session / sample-buffer delivery
- `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.mm` — Objective-C++ bridge into the C++ decoder
- `src/lib/ios_recv/` — reusable receive-session core used by the iOS integration

## 10. Known limitations

At the time of writing:

- this iOS app path has been implemented but not fully validated end-to-end on a physical iPhone in this environment
- the flashlight UI control is currently a placeholder and does not yet toggle the device torch
- if you edit `project.yml`, you must rerun `xcodegen generate`
- desktop Linux build issues involving `GLES3/gl3.h` are unrelated to iOS Xcode generation on macOS
- the decoder path depends on **OpenCV**; on macOS/iOS you will need an iOS-compatible OpenCV SDK/framework, not just a Linux package install

## 11. Troubleshooting

### Xcode now fails on `opencv2/opencv.hpp` or OpenCV linker errors

That means the project has advanced past the earlier bridge/link issues and is now compiling the real decoder stack.

You need an **iOS-compatible OpenCV SDK** (for example an `opencv2.xcframework` or equivalent framework package) added to the Xcode project. A Homebrew/macOS OpenCV install is not sufficient for an iPhone target.

Typical fix direction:

1. download/build an iOS OpenCV SDK
2. add the framework/xcframework to the Xcode project
3. link it to the `LibCimbarReceiver` target
4. clean and rebuild


### `xcodegen: command not found`

Install it:

```bash
brew install xcodegen
```

### Xcode shows signing/provisioning errors

Check:

- your Apple Development team is selected
- your bundle identifier is unique
- automatic signing is enabled, if appropriate

### Project changes are not reflected in Xcode

Regenerate the project:

```bash
cd ios-app
xcodegen generate
```

Then reopen or refresh the Xcode project.

### Camera view is blank on simulator

Use a physical iPhone. The simulator is not the intended test environment for this app.

## 12. Recommended next validation step

The highest-value next step is to run the generated app on a real iPhone and verify a complete transfer from a known-good animated cimbar sender.
