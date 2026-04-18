import Foundation

enum ScanPhase: Equatable {
    case idle
    case searching
    case detecting
    case decoding
    case completed
    case error(String)

    var statusText: String {
        switch self {
        case .idle:
            return "Ready to scan"
        case .searching:
            return "Searching for cimbar code…"
        case .detecting:
            return "Detected a frame"
        case .decoding:
            return "Decoding frame data…"
        case .completed:
            return "Scan completed"
        case .error(let message):
            return message
        }
    }
}

struct ScanSnapshot: Equatable {
    let phase: ScanPhase
    let recognizedFrame: Bool
    let needsSharpen: Bool
    let extractedBytes: Int
    let completedFileID: Int64
}

struct ScanState: Equatable {
    var phase: ScanPhase = .idle
    var isRecognizedFrame = false
    var needsSharpen = false
    var extractedBytes = 0
    var completedFileID: Int64?
    var statusText = ScanPhase.idle.statusText

    init() {}

    init(snapshot: ScanSnapshot) {
        phase = snapshot.phase
        isRecognizedFrame = snapshot.recognizedFrame
        needsSharpen = snapshot.needsSharpen
        extractedBytes = snapshot.extractedBytes
        completedFileID = snapshot.completedFileID > 0 ? snapshot.completedFileID : nil
        statusText = snapshot.phase.statusText
    }
}
