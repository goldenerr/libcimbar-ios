# LibCimbarReceiver iOS App

This directory contains an XcodeGen-managed iOS app scaffold for the LibCimbar receiver.

## Generate the project on macOS

```bash
brew install xcodegen
cd ios-app
xcodegen generate
open LibCimbarReceiver.xcodeproj
```

## Notes

- The generated `.xcodeproj` is not committed.
- Edit `project.yml` and source files, then regenerate the project on macOS.
