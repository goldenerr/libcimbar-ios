# libcimbar iOS Receiver App Design

**Project:** `sz3/libcimbar`
**Date:** 2026-04-18
**Goal:** Build an iOS-native receiver app that uses the existing `libcimbar` decoding pipeline to continuously scan animated cimbar codes from another device’s screen, reconstruct files, and provide a polished app experience with progress, file history, and sharing/export.

---

## 1. Problem Statement

`libcimbar` already contains the core C++ decoding and file reconstruction logic, but the current receive-side implementations are desktop/CLI oriented:

- `src/exe/cimbar_recv/recv.cpp`
- `src/exe/cimbar_recv2/recv2.cpp`

These entrypoints combine camera handling, frame loop orchestration, extraction, decoding, progress reporting, and file output inside `main()`-style executables. That structure is workable for CLI tools, but not for an iOS app.

The requested outcome is not a thin wrapper or demo shell. It is an iOS app with a user experience comparable to the Android receiver app category:

- live camera scanning of animated cimbar codes
- visible scan/decode progress
- successful file reconstruction into app storage
- received file list/history
- export/share flows through iOS

The app’s primary use case is **real-time scanning of dynamic cimbar sequences displayed on another device**, not static single-image import.

---

## 2. Scope

### In scope for v1

- Native iOS app project
- Real-time camera-based scanning of animated cimbar sequences
- Reuse of `libcimbar` C++ decoding/reconstruction logic
- Extraction of a reusable session-style receive API from existing CLI receive code
- Progress/state display suitable for end users
- Persist completed files to app-managed storage
- List received files in-app
- Share/export completed files via native iOS sharing mechanisms

### Explicitly out of scope for v1

- Background scanning while app is suspended
- Multiple concurrent receive sessions
- iCloud sync / cloud backup / account systems
- Complex analytics/history dashboards
- App Store packaging and release ops
- Advanced editing/preview of every file type
- iPad-specific UX optimization
- Static-image import as a primary path

### Deferred / future work

- Static image or photo-library import
- ROI/manual scan region selection
- Multi-file receive queueing
- Rich file metadata database
- Advanced recovery/error diagnostics UI

---

## 3. Design Principles

1. **Reuse the proven decode pipeline rather than rewriting it.**
   The high-risk/high-value asset is the C++ extraction/decoding/reconstruction logic.

2. **Separate app shell responsibilities from decoding responsibilities.**
   iOS UI, camera lifecycle, storage, and sharing should be native concerns. Decoder logic should be packaged as a reusable engine.

3. **Design for stable real-time scanning first.**
   The success criterion is a usable scanning app, not maximum architectural purity in the first implementation.

4. **YAGNI for product scope.**
   One active receive flow, one polished scan path, one file history path.

5. **Progress must be understandable, not deceptively precise.**
   Fountain-based reconstruction is not cleanly linear. UI should combine coarse progress with status messaging.

---

## 4. Recommended Approach

### Chosen approach

Build a **native iOS app using SwiftUI + AVFoundation**, with an **Objective-C++ bridge** that wraps reusable `libcimbar` receive functionality extracted primarily from `recv2.cpp`.

This is the recommended middle path:

- more product-appropriate than a WebView/PWA shell
- faster and lower-risk than fully redesigning the C++ core up front
- preserves the option to refine the internal API later

### Why base the receive path on `recv2.cpp`

`recv2.cpp` is the most relevant starting point because it already models the flow closest to the desired app behavior:

- consume frames
- extract/decode payload data
- reconstruct file(s)
- save completed output

It is a better foundation for an iOS receive session abstraction than starting from send-side code or trying to wrap a CLI binary wholesale.

---

## 5. Architecture

The app will be split into four layers.

### 5.1 UI Layer (SwiftUI)

Responsibilities:

- scanning screen
- progress/status display
- success modal/screen
- received files list
- file actions (share/export/open where supported)

This layer should know nothing about OpenCV internals or decoder object graphs. It consumes a simplified state model exposed by a view model / service layer.

### 5.2 Camera Capture Layer (AVFoundation)

Responsibilities:

- own camera authorization and session lifecycle
- configure rear camera, frame format, frame rate targets
- capture real-time frames
- hand frames off to decode pipeline with throttling/backpressure control

This layer should prefer responsiveness over exhaustive frame processing. In a real-time scanner, the newest useful frame is more important than processing every frame.

### 5.3 Bridge / Adapter Layer (Objective-C++)

Responsibilities:

- convert captured frames (`CVPixelBuffer` / `CMSampleBuffer`) into the format required by C++ processing
- expose a stable Swift-callable API
- translate C++ decoder state into app-friendly status/progress snapshots

This layer is the contractual boundary between the iOS app and `libcimbar` internals.

### 5.4 Core Decode Layer (`libcimbar` + iOS adapter)

Responsibilities:

- frame extraction
- cimbar decoding
- fountain reconstruction
- decompression
- completed file materialization

This layer should be refactored so the receive flow is represented by a long-lived session object instead of a CLI loop embedded in `main()`.

---

## 6. Core Refactor: Session-Based Receive API

The key internal change is to extract the CLI receive flow into a reusable stateful object.

### Proposed concept: `CimbarReceiveSession`

Responsibilities:

- configure receive mode/options
- accept one frame at a time
- maintain decoder/reconstruction state across frames
- report scan/decode progress
- emit completed files
- reset for a new receive session

### Conceptual API

The exact naming can change, but the abstraction should provide capabilities equivalent to:

- `configure(mode, options)`
- `processFrame(buffer, width, height, channels, stride)`
- `getProgressSnapshot()`
- `takeCompletedFiles()`
- `reset()`

### Why this refactor matters

The iOS app cannot own a CLI-style `while(true)` decoder loop. Instead, the app camera pipeline will deliver frames opportunistically, and the decoder must behave like a stateful service object.

This is the central transformation that makes `libcimbar` app-friendly.

---

## 7. iOS Runtime Flow

### 7.1 High-level flow

1. User opens app
2. App lands on scan screen
3. Camera session starts automatically
4. Capture layer produces frames
5. Frames are throttled and sent to bridge layer
6. Bridge converts frame data and calls decoder session
7. Decoder session updates state and possibly completes a file
8. UI reflects state changes in near real time
9. Completed file is persisted and shown in success flow + file list

### 7.2 Frame processing pipeline

Recommended pipeline:

1. `AVCaptureVideoDataOutput` receives frames
2. Frame callback enqueues newest frame for decode queue
3. Decode queue drops stale frames if already busy
4. Objective-C++ bridge converts frame to `cv::Mat` / required buffer format
5. `CimbarReceiveSession::processFrame(...)` runs extract/decode/reconstruct step
6. Progress snapshot is returned to Swift layer
7. UI updates on main thread

### 7.3 Backpressure strategy

To avoid UI jank and thermal issues:

- camera may run at ~30fps
- decode loop should begin at a lower effective target (e.g. ~10–15fps)
- if decode is behind, stale queued frames should be discarded in favor of the newest frame

This policy is intentionally optimized for real-time scanning.

---

## 8. Threading Model

### Main thread

- UI only
- no heavy decode work

### Camera queue

- receives raw capture callbacks
- minimal frame dispatch work

### Decode queue (serial)

- owns `CimbarReceiveSession`
- all frame processing occurs here
- prevents concurrent mutation of decoder session state

### Rationale

The existing codebase was not designed around iOS multi-queue UI orchestration. For v1, the safest approach is to treat the receive session as a serially accessed object.

---

## 9. State Model for UI

The decoder should not leak low-level implementation details directly into SwiftUI. The app should use a simplified state model such as:

- `idle`
- `searching`
- `detecting`
- `decoding(progress?)`
- `reconstructing`
- `completed(file)`
- `error(message)`

### Display strategy

The scan screen should combine:

- a coarse progress bar
- a user-facing status message
- optional technical hints like “recognized frames” or “signal unstable” only when useful

### Progress philosophy

Because fountain decode progress is inherently non-linear, the app should avoid pretending precision it does not actually have. Better UI:

- “Recognizing signal”
- “Receiving data”
- “Reconstructing file”
- “Almost done”
- “Completed”

If a numeric or bar-based progress estimate is available, it should be treated as approximate.

---

## 10. Product UX

### 10.1 Primary screens

#### Scan screen

Must include:

- live camera preview
- scan status header
- scan guidance/overlay
- progress region
- reset action
- entry to received file list
- optional flashlight toggle

#### Success screen or success modal

Must include:

- file name
- file size
- receive success feedback
- actions: share, export, continue scanning

#### Received files list

Must include:

- file name
- size
- received time
- tap to share/export/open where supported

### 10.2 Interaction rules

- app should be ready to scan immediately after permissions are granted
- temporary scan loss should not trigger noisy fatal UI
- successful completion should provide strong feedback
- user must be able to explicitly reset the current receive session

---

## 11. File Persistence

Completed files should be saved to an app-managed durable location such as:

- `Application Support/ReceivedFiles/`

Not temporary cache directories.

### Metadata/indexing

For v1, a lightweight metadata layer is sufficient. Each received file record should include:

- file name
- local path
- size
- received timestamp
- optional type/mime if derivable
- session identifier if needed

A full database is unnecessary for v1. A simple JSON index or directory scan + sidecar metadata is enough.

---

## 12. Sharing / Export

Use native iOS mechanisms:

- `UIActivityViewController` for share/export
- support common destinations such as Files, AirDrop, and other apps
- optional `QuickLook` preview where file type supports it

Priority is reliable export/share, not sophisticated in-app preview.

---

## 13. Dependency / Build Considerations

### OpenCV

`libcimbar` depends on OpenCV. iOS integration will require either:

- an existing iOS-compatible OpenCV framework/xcframework, or
- custom cross-compilation

This is one of the primary implementation risks.

### Desktop-only GUI dependencies

Desktop-oriented window/display code such as GLFW-based UI must not be part of the iOS runtime path.

The receive engine for iOS should depend only on decode-relevant components, not desktop presentation scaffolding.

### Build orchestration

The current repository is CMake-driven. The iOS app will likely need an Xcode-first integration strategy for v1, selectively compiling the necessary source sets into the app target, rather than trying to run the entire existing desktop build shape unchanged.

---

## 14. Risks and Mitigations

### Risk: OpenCV integration complexity on iOS

**Mitigation:** choose and pin a known-good iOS-compatible OpenCV packaging strategy early.

### Risk: receive code is too entangled with CLI/desktop assumptions

**Mitigation:** refactor from `recv2.cpp` into a session-based adapter, not by wrapping the whole executable.

### Risk: real-time decode performance is insufficient

**Mitigation:**

- serial decode queue
- frame throttling
- stale-frame dropping
- optimize later only after correctness is established

### Risk: image format/stride/channel mismatch causes silent decode failure

**Mitigation:** add explicit bridge-layer tests with known-good sample frames.

### Risk: progress UX feels erratic or misleading

**Mitigation:** expose coarse state + approximate progress instead of fake precision.

---

## 15. Testing Strategy

### 15.1 Core decode regression tests

Add tests around the extracted session abstraction to verify:

- initialization/configuration
- multi-frame ingestion
- progress state changes
- completed file emission
- reset behavior

Where possible, use sample frame/image inputs to verify decode behavior remains stable after refactor.

### 15.2 Bridge-layer tests

Verify that iOS image buffers are converted correctly:

- width/height/stride correctness
- RGB/BGR channel handling
- pixel buffer conversion correctness

This is a likely source of subtle bugs.

### 15.3 Device scanning tests

Validate on real hardware under realistic conditions:

- varying distances
- varying brightness
- multiple iPhone models if available
- sustained scanning duration
- thermal behavior / dropped frame behavior

### 15.4 End-to-end UX tests

Exercise the real user path:

- launch → grant camera → scan → complete → share/export
- temporary occlusion
- wrong target/no signal
- reset and rescan
- repeated receive sessions

---

## 16. Phased Implementation Plan (High-Level)

### Phase 1: iOS project foundation

- create app project
- establish SwiftUI shell
- add Objective-C++ bridge capability
- integrate required `libcimbar` and OpenCV pieces into app build

**Exit criteria:** app builds and Swift can invoke bridge methods.

### Phase 2: Minimal decoder session extraction

- extract reusable receive session from `recv2.cpp`
- support test-driven frame ingestion outside CLI
- surface progress snapshots and completed files

**Exit criteria:** sample frames can drive the session to a completed file path without the camera pipeline.

### Phase 3: Real-time camera integration

- connect AVFoundation video output to decode queue
- implement throttling/backpressure
- run continuous decode loop on device

**Exit criteria:** app can scan animated cimbar sequences in real time on device.

### Phase 4: Product UX layer

- scan screen
- success flow
- file list
- share/export actions

**Exit criteria:** complete user flow works without logs/debug tooling.

### Phase 5: Stability and performance tuning

- improve decode responsiveness
- reduce thermal load
- refine messages and reset behavior
- verify across additional conditions/devices

**Exit criteria:** stable demo-quality v1 receiver app.

---

## 17. Acceptance Criteria for v1

The v1 design is satisfied when all of the following are true:

1. An iOS app launches and starts a real-time scan flow using the device camera.
2. The app continuously processes animated cimbar frames from another device.
3. The app displays meaningful scanning/progress state during reception.
4. The app reconstructs at least one transmitted file successfully through the reused `libcimbar` pipeline.
5. The file is saved locally in app-managed storage.
6. The user can view a file list/history and share/export a completed file.
7. The implementation does not depend on desktop GUI runtime code paths.

---

## 18. Recommendation Summary

Proceed by turning the existing receive-side `libcimbar` logic—primarily from `recv2.cpp`—into a reusable session-based decoder service, then bridge that service into a native SwiftUI iOS app via Objective-C++, with AVFoundation providing real-time frames and native iOS storage/share UX surrounding it.

This approach is the best balance of delivery speed, technical realism, reuse of the existing codebase, and product-quality UX for the requested first version.
