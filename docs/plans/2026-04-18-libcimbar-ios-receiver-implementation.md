# libcimbar iOS Receiver Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Turn `sz3/libcimbar` into an iOS-native receiver app that continuously scans animated cimbar codes, reconstructs files, stores them locally, and supports share/export.

**Architecture:** Extract the receive loop from `src/exe/cimbar_recv2/recv2.cpp` into a reusable session-oriented C++ library, expose a narrow C API for Objective-C++ consumption, then wrap that in a SwiftUI + AVFoundation iOS app shell generated from an XcodeGen manifest. Keep desktop/CLI behavior intact while adding a new iOS-specific path.

**Tech Stack:** C++17, OpenCV, existing `libcimbar` extractor/decoder/fountain/zstd code, CMake for native library/tests, C API wrapper, Objective-C++, Swift, SwiftUI, AVFoundation, UniformTypeIdentifiers, XcodeGen.

---

## Preconditions and assumptions

- Repository root: `/tmp/hermes-work/libcimbar`
- The implementation environment can edit files freely but cannot build iOS targets on Linux.
- Native C++ tests should still run on Linux via the existing CMake build.
- iOS project generation/build verification will be documented as **macOS-only** verification steps.
- The first deliverable is one active receive session at a time.

---

## Task 1: Add a new reusable receive module skeleton

**Objective:** Create a dedicated C++ module for the session-based receive flow so iOS logic does not depend on CLI `main()` files.

**Files:**
- Create: `src/lib/ios_recv/CMakeLists.txt`
- Create: `src/lib/ios_recv/CimbarReceiveSession.h`
- Create: `src/lib/ios_recv/CimbarReceiveSession.cpp`
- Create: `src/lib/ios_recv/CimbarReceiveTypes.h`
- Modify: `CMakeLists.txt`

**Step 1: Write failing test**

Create a skeleton test file reference first (actual test executable is added in Task 2):

```cpp
#include "ios_recv/CimbarReceiveSession.h"

int main() {
    cimbar::ios_recv::CimbarReceiveSession session;
    return session.mode_value() == 68 ? 0 : 1;
}
```

**Step 2: Run test to verify failure**

Run:
```bash
cmake -S . -B build >/tmp/cmake.log && cmake --build build -j4
```
Expected: FAIL — missing `src/lib/ios_recv/*` files and missing subdirectory in root `CMakeLists.txt`.

**Step 3: Write minimal implementation**

Use this structure:

```cpp
// src/lib/ios_recv/CimbarReceiveTypes.h
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cimbar::ios_recv {

enum class SessionPhase {
    Idle,
    Searching,
    Detecting,
    Decoding,
    Reconstructing,
    Completed,
    Error,
};

struct ProgressSnapshot {
    SessionPhase phase = SessionPhase::Idle;
    bool recognized_frame = false;
    bool needs_sharpen = false;
    int extracted_bytes = 0;
    int64_t completed_file_id = 0;
    std::string status_message;
    std::vector<int> fountain_progress;
};

struct CompletedFile {
    uint32_t file_id = 0;
    std::string filename;
    std::vector<unsigned char> compressed_bytes;
};

} // namespace cimbar::ios_recv
```

```cpp
// src/lib/ios_recv/CimbarReceiveSession.h
#pragma once

#include "CimbarReceiveTypes.h"

namespace cimbar::ios_recv {

class CimbarReceiveSession {
public:
    CimbarReceiveSession();

    int configure_mode(int mode_value);
    int mode_value() const;
    void reset();
    const ProgressSnapshot& progress() const;

private:
    int _mode_value = 68;
    ProgressSnapshot _progress;
};

} // namespace cimbar::ios_recv
```

```cpp
// src/lib/ios_recv/CimbarReceiveSession.cpp
#include "CimbarReceiveSession.h"

namespace cimbar::ios_recv {

CimbarReceiveSession::CimbarReceiveSession() = default;

int CimbarReceiveSession::configure_mode(int mode_value) {
    _mode_value = mode_value > 0 ? mode_value : 68;
    return 0;
}

int CimbarReceiveSession::mode_value() const {
    return _mode_value;
}

void CimbarReceiveSession::reset() {
    _progress = {};
}

const ProgressSnapshot& CimbarReceiveSession::progress() const {
    return _progress;
}

} // namespace cimbar::ios_recv
```

```cmake
# src/lib/ios_recv/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

set(SOURCES
    CimbarReceiveTypes.h
    CimbarReceiveSession.h
    CimbarReceiveSession.cpp
)

add_library(ios_recv STATIC ${SOURCES})

target_link_libraries(ios_recv
    cimb_translator
    extractor
    fountain
    zstd
    ${OPENCV_LIBS}
)

if(NOT DEFINED DISABLE_TESTS)
    add_subdirectory(test)
endif()
```

Update root `CMakeLists.txt` by adding `src/lib/ios_recv` to the `PROJECTS` list before executables.

**Step 4: Run test to verify pass**

Run:
```bash
cmake -S . -B build && cmake --build build -j4
```
Expected: PASS for compilation of the new library target.

**Step 5: Commit**

```bash
git add CMakeLists.txt src/lib/ios_recv
git commit -m "feat: add ios receive session module skeleton"
```

---

## Task 2: Add unit tests for the new session module

**Objective:** Ensure the new receive abstraction is test-first and gains a dedicated test target like the other library modules.

**Files:**
- Create: `src/lib/ios_recv/test/CMakeLists.txt`
- Create: `src/lib/ios_recv/test/test.cpp`
- Create: `src/lib/ios_recv/test/CimbarReceiveSessionTest.cpp`
- Modify: `src/lib/ios_recv/CMakeLists.txt`

**Step 1: Write failing test**

```cpp
// src/lib/ios_recv/test/CimbarReceiveSessionTest.cpp
#include "unittest.h"
#include "ios_recv/CimbarReceiveSession.h"

TEST_CASE("CimbarReceiveSession/defaultMode", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    assertEquals(68, session.mode_value());
}

TEST_CASE("CimbarReceiveSession/resetClearsProgress", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    session.configure_mode(4);
    session.reset();
    assertEquals(4, session.mode_value());
    assertEquals(static_cast<int>(cimbar::ios_recv::SessionPhase::Idle),
                 static_cast<int>(session.progress().phase));
}
```

**Step 2: Run test to verify failure**

Run:
```bash
cmake -S . -B build && cmake --build build -j4 --target ios_recv_test
```
Expected: FAIL — no `test` subdirectory and/or missing include wiring.

**Step 3: Write minimal implementation**

```cmake
# src/lib/ios_recv/test/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

set(SOURCES
    test.cpp
    CimbarReceiveSessionTest.cpp
)

include_directories(
    ${libcimbar_SOURCE_DIR}/test
    ${libcimbar_SOURCE_DIR}/test/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(ios_recv_test ${SOURCES})
add_test(ios_recv_test ios_recv_test)

target_link_libraries(ios_recv_test
    ios_recv
    cimb_translator
    extractor
    fountain
    zstd
    ${OPENCV_LIBS}
)
```

```cpp
// src/lib/ios_recv/test/test.cpp
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
```

**Step 4: Run test to verify pass**

Run:
```bash
cmake -S . -B build && cmake --build build -j4 --target ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: PASS — 2 test cases, 0 failures.

**Step 5: Commit**

```bash
git add src/lib/ios_recv/test src/lib/ios_recv/CMakeLists.txt
git commit -m "test: add ios receive session unit target"
```

---

## Task 3: Move frame scan/extract/decode logic into the session object

**Objective:** Refactor the scan loop logic from `recv2.cpp` / `cimbar_recv_js.cpp` into `CimbarReceiveSession::process_frame(...)`.

**Files:**
- Modify: `src/lib/ios_recv/CimbarReceiveSession.h`
- Modify: `src/lib/ios_recv/CimbarReceiveSession.cpp`
- Modify: `src/lib/ios_recv/test/CimbarReceiveSessionTest.cpp`
- Reference: `src/exe/cimbar_recv2/recv2.cpp`
- Reference: `src/lib/cimbar_js/cimbar_recv_js.cpp`

**Step 1: Write failing test**

Add a test that feeds a known sample image and expects bytes to be extracted:

```cpp
#include "TestHelpers.h"

TEST_CASE("CimbarReceiveSession/processFrameExtractsChunks", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    auto result = session.process_frame(img.data, img.cols, img.rows, 3, img.step);

    assertTrue(result.extracted_bytes > 0);
    assertTrue(result.recognized_frame);
}
```

**Step 2: Run test to verify failure**

Run:
```bash
cmake --build build -j4 --target ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: FAIL — `process_frame` does not exist.

**Step 3: Write minimal implementation**

Mirror the logic already proven in `cimbard_scan_extract_decode`, but make it session-owned instead of global/static:

```cpp
// add to header
ProgressSnapshot process_frame(const unsigned char* imgdata,
                               unsigned width,
                               unsigned height,
                               int format,
                               unsigned stride = 0);
```

Implementation requirements:
- maintain `_extractor`, `_decoder`, `_chunk_buffer`, and `_progress` as members
- convert input image to RGB `cv::UMat` similarly to `get_rgb(...)` in `cimbar_recv_js.cpp`
- call `_extractor.extract(...)`
- if extraction succeeds, decode into a per-session chunk buffer using `escrow_buffer_writer`
- set `recognized_frame`, `needs_sharpen`, `phase`, `status_message`, and `extracted_bytes`
- leave fountain reconstruction for the next task

Suggested member additions:

```cpp
Extractor _extractor;
Decoder _decoder;
std::vector<unsigned char> _chunk_buffer;
```

Suggested status mapping:
- no extract -> `Searching`
- extract + no decode -> `Detecting`
- decode bytes > 0 -> `Decoding`

**Step 4: Run test to verify pass**

Run:
```bash
cmake --build build -j4 --target ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: PASS — sample image yields `extracted_bytes > 0`.

**Step 5: Commit**

```bash
git add src/lib/ios_recv
git commit -m "feat: add frame extraction and chunk decode to ios receive session"
```

---

## Task 4: Move fountain reconstruction and decompression into the session object

**Objective:** Make the session self-contained by handling chunk ingestion, completed file IDs, filename resolution, and decompression reads.

**Files:**
- Modify: `src/lib/ios_recv/CimbarReceiveSession.h`
- Modify: `src/lib/ios_recv/CimbarReceiveSession.cpp`
- Modify: `src/lib/ios_recv/test/CimbarReceiveSessionTest.cpp`
- Reference: `src/lib/cimbar_js/cimbar_recv_js.cpp`
- Reference: `src/lib/cimbar_js/test/cimbar_recv_jsTest.cpp`

**Step 1: Write failing test**

```cpp
TEST_CASE("CimbarReceiveSession/processFrameCompletesSampleFile", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    auto snapshot = session.process_frame(img.data, img.cols, img.rows, 3, img.step);
    auto completed = session.take_completed_files();

    assertTrue(snapshot.completed_file_id > 0);
    assertEquals(1, static_cast<int>(completed.size()));
    assertFalse(completed.front().filename.empty());
    assertTrue(completed.front().decompressed_bytes.size() > 0);
}
```

**Step 2: Run test to verify failure**

Run:
```bash
cmake --build build -j4 --target ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: FAIL — no `take_completed_files()` and no reconstruction support.

**Step 3: Write minimal implementation**

Add state modeled on `cimbar_recv_js.cpp`, but per-session instead of namespace globals:

```cpp
std::shared_ptr<fountain_decoder_sink> _sink;
std::vector<unsigned char> _reassembled;
std::unique_ptr<cimbar::zstd_decompressor<std::stringstream>> _decompressor;
std::vector<CompletedFile> _completed_files;
```

Expand `CompletedFile` in `CimbarReceiveTypes.h`:

```cpp
struct CompletedFile {
    uint32_t file_id = 0;
    std::string filename;
    std::vector<unsigned char> compressed_bytes;
    std::vector<unsigned char> decompressed_bytes;
};
```

Add methods:

```cpp
std::vector<CompletedFile> take_completed_files();
```

Implementation behavior:
- if `process_frame(...)` extracts bytes, feed aligned chunks into `_sink->decode_frame(...)`
- if a file completes (`res > 0`), recover compressed bytes
- read filename via zstd header helper logic or fallback naming
- decompress into `CompletedFile::decompressed_bytes`
- append to `_completed_files`
- set `phase = SessionPhase::Completed`
- set `completed_file_id`

**Step 4: Run test to verify pass**

Run:
```bash
cmake --build build -j4 --target ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: PASS — sample frame path completes the sample payload and yields one completed file.

**Step 5: Commit**

```bash
git add src/lib/ios_recv
git commit -m "feat: add fountain reconstruction and completed file extraction"
```

---

## Task 5: Add a narrow C API for iOS bridge consumption

**Objective:** Provide a stable C interface that Objective-C++ can call without directly depending on C++ class internals.

**Files:**
- Create: `src/lib/ios_recv/cimbar_ios_recv_c.h`
- Create: `src/lib/ios_recv/cimbar_ios_recv_c.cpp`
- Modify: `src/lib/ios_recv/CMakeLists.txt`
- Modify: `src/lib/ios_recv/test/CimbarReceiveSessionTest.cpp`

**Step 1: Write failing test**

```cpp
extern "C" {
#include "ios_recv/cimbar_ios_recv_c.h"
}

TEST_CASE("cimbar_ios_recv_c/createResetDestroy", "[unit]") {
    auto* handle = cimbar_ios_recv_create();
    assertFalse(handle == nullptr);
    assertEquals(0, cimbar_ios_recv_reset(handle));
    cimbar_ios_recv_destroy(handle);
}
```

**Step 2: Run test to verify failure**

Run:
```bash
cmake --build build -j4 --target ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: FAIL — missing C API files.

**Step 3: Write minimal implementation**

```cpp
// cimbar_ios_recv_c.h
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cimbar_ios_recv_handle cimbar_ios_recv_handle;

typedef struct {
    int phase;
    int recognized_frame;
    int needs_sharpen;
    int extracted_bytes;
    int64_t completed_file_id;
} cimbar_ios_recv_progress;

cimbar_ios_recv_handle* cimbar_ios_recv_create(void);
void cimbar_ios_recv_destroy(cimbar_ios_recv_handle* handle);
int cimbar_ios_recv_reset(cimbar_ios_recv_handle* handle);
int cimbar_ios_recv_configure_mode(cimbar_ios_recv_handle* handle, int mode_value);
int cimbar_ios_recv_process_frame(cimbar_ios_recv_handle* handle,
                                  const unsigned char* imgdata,
                                  unsigned width,
                                  unsigned height,
                                  int format,
                                  unsigned stride,
                                  cimbar_ios_recv_progress* out_progress);

#ifdef __cplusplus
}
#endif
```

Use an opaque wrapper struct in `.cpp`:

```cpp
struct cimbar_ios_recv_handle {
    cimbar::ios_recv::CimbarReceiveSession session;
};
```

Add the new source to `src/lib/ios_recv/CMakeLists.txt` so it is compiled into `ios_recv`.

**Step 4: Run test to verify pass**

Run:
```bash
cmake --build build -j4 --target ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: PASS — C API handle creation/reset works.

**Step 5: Commit**

```bash
git add src/lib/ios_recv
git commit -m "feat: add C API wrapper for ios receive session"
```

---

## Task 6: Adapt `recv2.cpp` to use the new session abstraction

**Objective:** Prove the abstraction is real by reusing it from the existing CLI receive tool instead of keeping duplicate logic.

**Files:**
- Modify: `src/exe/cimbar_recv2/recv2.cpp`
- Modify: `src/exe/cimbar_recv2/CMakeLists.txt` (only if link target changes are needed)

**Step 1: Write failing test**

Verification target is the CLI build. First make the code stop including low-level decode globals directly.

Expected replacement pattern in `recv2.cpp`:

```cpp
#include "ios_recv/CimbarReceiveSession.h"
```

**Step 2: Run build to verify failure during transition**

Run:
```bash
cmake --build build -j4 --target cimbar_recv2
```
Expected: FAIL during the edit window until the new API is fully wired.

**Step 3: Write minimal implementation**

Refactor `recv2.cpp` so:
- camera/window loop remains in CLI
- all frame decode/reconstruction behavior moves to `CimbarReceiveSession`
- on completed file, CLI still writes output to disk

Sketch:

```cpp
cimbar::ios_recv::CimbarReceiveSession session;
session.configure_mode(config_mode);

...

auto snapshot = session.process_frame(img.data, img.cols, img.rows, 3, img.step);
for (auto& file : session.take_completed_files()) {
    std::ofstream outs(fmt::format("{}/{}", outpath, file.filename), std::ios::binary);
    outs.write(reinterpret_cast<const char*>(file.decompressed_bytes.data()),
               file.decompressed_bytes.size());
}
```

**Step 4: Run build and targeted tests to verify pass**

Run:
```bash
cmake --build build -j4 --target cimbar_recv2 ios_recv_test && ./build/src/lib/ios_recv/test/ios_recv_test
```
Expected: PASS — CLI target compiles and the ios receive unit tests still pass.

**Step 5: Commit**

```bash
git add src/exe/cimbar_recv2 src/lib/ios_recv
git commit -m "refactor: route cimbar_recv2 through ios receive session"
```

---

## Task 7: Create the iOS app source tree and XcodeGen project manifest

**Objective:** Add a repository-managed iOS app scaffold that can be generated on macOS without hand-editing `.pbxproj` files.

**Files:**
- Create: `ios-app/project.yml`
- Create: `ios-app/README.md`
- Create: `ios-app/LibCimbarReceiver/App/LibCimbarReceiverApp.swift`
- Create: `ios-app/LibCimbarReceiver/App/AppState.swift`
- Create: `ios-app/LibCimbarReceiver/Features/Scan/ScanView.swift`
- Create: `ios-app/LibCimbarReceiver/Features/Files/ReceivedFilesView.swift`
- Create: `ios-app/LibCimbarReceiver/Models/ReceivedFile.swift`
- Create: `ios-app/LibCimbarReceiver/Resources/Info.plist`
- Create: `ios-app/LibCimbarReceiver/Resources/Assets.xcassets/Contents.json`

**Step 1: Write failing test**

The “test” here is manifest validation and tree completeness.

Add `ios-app/README.md` instructions that expect this command on macOS:

```bash
cd ios-app
xcodegen generate
```

**Step 2: Verify failure before files exist**

Run:
```bash
test -f ios-app/project.yml
```
Expected: FAIL — file does not exist.

**Step 3: Write minimal implementation**

Use an XcodeGen manifest roughly like this:

```yaml
name: LibCimbarReceiver
options:
  bundleIdPrefix: org.example
settings:
  base:
    SWIFT_VERSION: 5.10
    IPHONEOS_DEPLOYMENT_TARGET: 16.0
targets:
  LibCimbarReceiver:
    type: application
    platform: iOS
    sources:
      - LibCimbarReceiver
    settings:
      base:
        PRODUCT_BUNDLE_IDENTIFIER: org.example.LibCimbarReceiver
        INFOPLIST_FILE: LibCimbarReceiver/Resources/Info.plist
    info:
      path: LibCimbarReceiver/Resources/Info.plist
      properties:
        CFBundleDisplayName: LibCimbarReceiver
        NSCameraUsageDescription: Scan animated cimbar codes to receive files.
```

Minimal app entry:

```swift
import SwiftUI

@main
struct LibCimbarReceiverApp: App {
    @StateObject private var appState = AppState()

    var body: some Scene {
        WindowGroup {
            TabView {
                ScanView()
                    .tabItem { Label("Scan", systemImage: "viewfinder") }
                ReceivedFilesView()
                    .tabItem { Label("Files", systemImage: "folder") }
            }
            .environmentObject(appState)
        }
    }
}
```

**Step 4: Verify pass**

Run on Linux for structural verification:
```bash
test -f ios-app/project.yml && test -f ios-app/LibCimbarReceiver/App/LibCimbarReceiverApp.swift && test -f ios-app/README.md
```
Expected: PASS.

Also document macOS verification in `ios-app/README.md`:
```bash
brew install xcodegen
cd ios-app
xcodegen generate
open LibCimbarReceiver.xcodeproj
```

**Step 5: Commit**

```bash
git add ios-app
git commit -m "feat: add xcodegen-based ios app scaffold"
```

---

## Task 8: Add Objective-C++ bridge files and Swift-facing decoder service

**Objective:** Connect Swift code to the new C API through an Objective-C++ bridge that owns the native decoder handle.

**Files:**
- Create: `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.h`
- Create: `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.mm`
- Create: `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.swift`
- Create: `ios-app/LibCimbarReceiver/Models/ScanState.swift`
- Modify: `ios-app/project.yml`

**Step 1: Write failing test**

Add a Swift wrapper expectation in `CimbarDecoderBridge.swift`:

```swift
final class CimbarDecoderBridgeService {
    func reset() {}
    func configure(mode: Int) {}
}
```

The manifest should include bridge headers/sources so Xcode can compile them.

**Step 2: Verify failure before files exist**

Run:
```bash
test -f ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.mm
```
Expected: FAIL.

**Step 3: Write minimal implementation**

Objective-C++ header:

```objc
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CimbarDecoderBridge : NSObject
- (instancetype)init;
- (void)reset;
- (void)configureMode:(NSInteger)mode;
@end

NS_ASSUME_NONNULL_END
```

Objective-C++ implementation pattern:

```objc
#import "CimbarDecoderBridge.h"
#import "cimbar_ios_recv_c.h"

@interface CimbarDecoderBridge () {
    cimbar_ios_recv_handle *_handle;
}
@end

@implementation CimbarDecoderBridge
- (instancetype)init {
    self = [super init];
    if (self) {
        _handle = cimbar_ios_recv_create();
    }
    return self;
}
- (void)dealloc {
    if (_handle) { cimbar_ios_recv_destroy(_handle); }
}
- (void)reset { cimbar_ios_recv_reset(_handle); }
- (void)configureMode:(NSInteger)mode { cimbar_ios_recv_configure_mode(_handle, (int)mode); }
@end
```

Swift-facing wrapper:

```swift
import Foundation

@MainActor
final class CimbarDecoderBridgeService: ObservableObject {
    private let native = CimbarDecoderBridge()

    func reset() {
        native.reset()
    }

    func configure(mode: Int) {
        native.configureMode(mode)
    }
}
```

Update `ios-app/project.yml` to add the bridge source folder and a bridging header only if needed.

**Step 4: Verify pass**

Run structural verification:
```bash
test -f ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.h && test -f ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.mm && test -f ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.swift
```
Expected: PASS.

Document macOS verification in `ios-app/README.md`:
```bash
xcodegen generate
xcodebuild -project LibCimbarReceiver.xcodeproj -scheme LibCimbarReceiver -sdk iphonesimulator -destination 'platform=iOS Simulator,name=iPhone 16' build
```

**Step 5: Commit**

```bash
git add ios-app
git commit -m "feat: add objective-c++ decoder bridge for ios app"
```

---

## Task 9: Add AVFoundation camera pipeline and frame throttling service

**Objective:** Capture live frames on iOS and feed only the newest useful frames into the decoder bridge.

**Files:**
- Create: `ios-app/LibCimbarReceiver/Camera/CameraCaptureService.swift`
- Create: `ios-app/LibCimbarReceiver/Camera/CameraPreviewView.swift`
- Modify: `ios-app/LibCimbarReceiver/Features/Scan/ScanView.swift`
- Modify: `ios-app/LibCimbarReceiver/App/AppState.swift`

**Step 1: Write failing test**

The test here is API shape. `ScanView` should expect a `CameraCaptureService`:

```swift
@StateObject private var camera = CameraCaptureService()
```

**Step 2: Verify failure before files exist**

Run:
```bash
test -f ios-app/LibCimbarReceiver/Camera/CameraCaptureService.swift
```
Expected: FAIL.

**Step 3: Write minimal implementation**

Use this structure:

```swift
import AVFoundation
import SwiftUI

final class CameraCaptureService: NSObject, ObservableObject {
    let session = AVCaptureSession()
    private let output = AVCaptureVideoDataOutput()
    private let captureQueue = DispatchQueue(label: "camera.capture.queue")
    private let decodeQueue = DispatchQueue(label: "camera.decode.queue")
    private var decodeInFlight = false
    var onSampleBuffer: ((CMSampleBuffer) -> Void)?

    func start() {}
    func stop() {}
}
```

Implement:
- camera permission request
- rear camera configuration
- `AVCaptureVideoDataOutputSampleBufferDelegate`
- stale-frame dropping using `decodeInFlight`
- callback to the bridge service with the latest sample buffer only

Use a preview wrapper like:

```swift
struct CameraPreviewView: UIViewRepresentable {
    let session: AVCaptureSession
    func makeUIView(context: Context) -> PreviewView { ... }
    func updateUIView(_ uiView: PreviewView, context: Context) {}
}
```

**Step 4: Verify pass**

Run structural verification:
```bash
test -f ios-app/LibCimbarReceiver/Camera/CameraCaptureService.swift && test -f ios-app/LibCimbarReceiver/Camera/CameraPreviewView.swift
```
Expected: PASS.

Mac-only verification (documented): build and run in simulator or on device; camera preview appears and session starts.

**Step 5: Commit**

```bash
git add ios-app/LibCimbarReceiver/Camera ios-app/LibCimbarReceiver/Features/Scan ios-app/LibCimbarReceiver/App
git commit -m "feat: add ios camera capture pipeline"
```

---

## Task 10: Extend the bridge to process frames and publish scan progress

**Objective:** Convert pixel buffers to raw bytes, call the C API, and translate progress into SwiftUI state.

**Files:**
- Modify: `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.h`
- Modify: `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.mm`
- Modify: `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.swift`
- Modify: `ios-app/LibCimbarReceiver/Models/ScanState.swift`
- Modify: `ios-app/LibCimbarReceiver/Camera/CameraCaptureService.swift`

**Step 1: Write failing test**

Add a Swift-facing processing API:

```swift
struct ScanSnapshot {
    let phase: ScanPhase
    let recognizedFrame: Bool
    let extractedBytes: Int
    let completedFileID: Int64
}

func process(sampleBuffer: CMSampleBuffer) -> ScanSnapshot?
```

**Step 2: Verify failure before implementation**

Run:
```bash
python - <<'PY'
from pathlib import Path
text = Path('ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.swift').read_text()
print('process(sampleBuffer:' in text)
PY
```
Expected: `False`.

**Step 3: Write minimal implementation**

In `.mm`, lock the `CVPixelBuffer`, get base address / width / height / bytesPerRow, and call:

```objc
cimbar_ios_recv_progress progress = {0};
int res = cimbar_ios_recv_process_frame(_handle, base, width, height, 4, stride, &progress);
```

Map C progress to Objective-C/Swift models.

Swift-side enum suggestion:

```swift
enum ScanPhase {
    case idle, searching, detecting, decoding, reconstructing, completed, error(String)
}
```

Use `@Published` state on the service so `ScanView` can react live.

**Step 4: Verify pass**

Run structural verification:
```bash
python - <<'PY'
from pathlib import Path
text = Path('ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.swift').read_text()
print('process(sampleBuffer:' in text)
PY
```
Expected: `True`.

Mac-only verification: on device, point camera at a cimbar sample and confirm phase changes from searching to decoding/completed.

**Step 5: Commit**

```bash
git add ios-app/LibCimbarReceiver/Bridge ios-app/LibCimbarReceiver/Models ios-app/LibCimbarReceiver/Camera
git commit -m "feat: publish scan progress from decoder bridge"
```

---

## Task 11: Persist completed files and expose them in the UI

**Objective:** Save completed files into durable app storage and surface them in a simple file list.

**Files:**
- Create: `ios-app/LibCimbarReceiver/Storage/ReceivedFilesStore.swift`
- Modify: `ios-app/LibCimbarReceiver/Models/ReceivedFile.swift`
- Modify: `ios-app/LibCimbarReceiver/App/AppState.swift`
- Modify: `ios-app/LibCimbarReceiver/Features/Files/ReceivedFilesView.swift`
- Modify: `ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.swift`

**Step 1: Write failing test**

Add a storage expectation in `ReceivedFilesStore.swift`:

```swift
func saveCompletedFile(filename: String, data: Data) throws -> ReceivedFile
```

`ReceivedFilesView` should expect `@EnvironmentObject var appState: AppState` and render `appState.receivedFiles`.

**Step 2: Verify failure before implementation**

Run:
```bash
test -f ios-app/LibCimbarReceiver/Storage/ReceivedFilesStore.swift
```
Expected: FAIL.

**Step 3: Write minimal implementation**

Model:

```swift
struct ReceivedFile: Identifiable, Codable {
    let id: UUID
    let filename: String
    let relativePath: String
    let size: Int
    let receivedAt: Date
}
```

Store behavior:
- create `Application Support/ReceivedFiles`
- write file bytes there
- update `index.json`
- reload records into memory

App state:

```swift
@MainActor
final class AppState: ObservableObject {
    @Published var receivedFiles: [ReceivedFile] = []
    let store = ReceivedFilesStore()
}
```

When bridge reports a completed file, persist it and append/reload the list.

**Step 4: Verify pass**

Run structural verification:
```bash
test -f ios-app/LibCimbarReceiver/Storage/ReceivedFilesStore.swift && python - <<'PY'
from pathlib import Path
text = Path('ios-app/LibCimbarReceiver/Features/Files/ReceivedFilesView.swift').read_text()
print('receivedFiles' in text)
PY
```
Expected: PASS.

**Step 5: Commit**

```bash
git add ios-app/LibCimbarReceiver/Storage ios-app/LibCimbarReceiver/Models ios-app/LibCimbarReceiver/App ios-app/LibCimbarReceiver/Features/Files ios-app/LibCimbarReceiver/Bridge
git commit -m "feat: persist completed files and list them in ios app"
```

---

## Task 12: Add success flow, export/share actions, and scanning polish

**Objective:** Finish the v1 UX with a success path, share sheet, reset action, and basic status messaging.

**Files:**
- Create: `ios-app/LibCimbarReceiver/Features/Scan/ShareSheet.swift`
- Create: `ios-app/LibCimbarReceiver/Features/Scan/ScanStatusView.swift`
- Modify: `ios-app/LibCimbarReceiver/Features/Scan/ScanView.swift`
- Modify: `ios-app/LibCimbarReceiver/Features/Files/ReceivedFilesView.swift`
- Modify: `ios-app/LibCimbarReceiver/App/AppState.swift`

**Step 1: Write failing test**

Add a UI expectation that `ScanView` exposes reset and share affordances:

```swift
Button("Reset") { ... }
```

and a sheet bound to the last received file.

**Step 2: Verify failure before implementation**

Run:
```bash
python - <<'PY'
from pathlib import Path
text = Path('ios-app/LibCimbarReceiver/Features/Scan/ScanView.swift').read_text()
print('Reset' in text and 'sheet' in text)
PY
```
Expected: `False`.

**Step 3: Write minimal implementation**

Add:
- `ScanStatusView` showing phase/progress text
- `ShareSheet` wrapping `UIActivityViewController`
- a reset button that clears the native session
- success banner/sheet when a new file lands
- file-row actions from the files list

Sketch:

```swift
struct ShareSheet: UIViewControllerRepresentable {
    let items: [Any]
    func makeUIViewController(context: Context) -> UIActivityViewController {
        UIActivityViewController(activityItems: items, applicationActivities: nil)
    }
    func updateUIViewController(_ controller: UIActivityViewController, context: Context) {}
}
```

`ScanView` should show:
- camera preview
- status section
- reset button
- files shortcut
- optional flashlight toggle placeholder if not fully implemented

**Step 4: Verify pass**

Run:
```bash
python - <<'PY'
from pathlib import Path
text = Path('ios-app/LibCimbarReceiver/Features/Scan/ScanView.swift').read_text()
print('Reset' in text and 'sheet' in text)
PY
```
Expected: `True`.

Mac-only verification: scan a real cimbar sequence on device, receive a file, see success UI, and export through the share sheet.

**Step 5: Commit**

```bash
git add ios-app/LibCimbarReceiver/Features/Scan ios-app/LibCimbarReceiver/Features/Files ios-app/LibCimbarReceiver/App
git commit -m "feat: complete ios receiver v1 sharing and scan ux"
```

---

## Final verification checklist

After all tasks are complete, run these verification steps.

### Linux / native verification

Run:
```bash
cmake -S . -B build
cmake --build build -j4 --target ios_recv_test cimbar_recv2
./build/src/lib/ios_recv/test/ios_recv_test
ctest --test-dir build --output-on-failure
```
Expected:
- native build succeeds
- `ios_recv_test` passes
- existing tests remain green
- `cimbar_recv2` still compiles using the new abstraction

### Repository structure verification

Run:
```bash
test -f docs/superpowers/specs/2026-04-18-libcimbar-ios-app-design.md
test -f ios-app/project.yml
test -f ios-app/LibCimbarReceiver/Bridge/CimbarDecoderBridge.mm
test -f src/lib/ios_recv/CimbarReceiveSession.cpp
```
Expected: all commands succeed.

### macOS-only verification

Run:
```bash
cd ios-app
xcodegen generate
xcodebuild -project LibCimbarReceiver.xcodeproj -scheme LibCimbarReceiver -sdk iphonesimulator -destination 'platform=iOS Simulator,name=iPhone 16' build
```
Then on a real iPhone:
- grant camera permission
- point at a device playing animated cimbar frames
- confirm state transitions from searching → decoding → completed
- confirm file appears in Files tab
- confirm share/export works

---

## Notes for the implementer

- Prefer moving proven code from `cimbar_recv_js.cpp` and `recv2.cpp` into `src/lib/ios_recv` instead of inventing new algorithms.
- Do not keep process-global decoder state for the iOS path; keep it inside the session object.
- Keep desktop GLFW/window code out of the iOS path.
- If a macOS-only build blocker appears, isolate it in the XcodeGen/iOS manifest and keep native library work independently testable on Linux.
- If sample-path assumptions fail in tests, fix the sample fixture path before changing decode behavior.

---

## Ready-for-execution handoff

When this plan is approved, execute it with **subagent-driven-development**:
- one fresh subagent per task
- spec compliance review after each task
- code quality review after spec passes
- only then move to the next task
