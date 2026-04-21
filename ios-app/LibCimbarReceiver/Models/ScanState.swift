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
    let scannedChunks: Int
    let totalChunks: Int
    let statusMessage: String
}

struct ScanState: Equatable {
    var phase: ScanPhase = .idle
    var isRecognizedFrame = false
    var needsSharpen = false
    var extractedBytes = 0
    var completedFileID: Int64?
    var scannedChunks = 0
    var totalChunks = 0
    var nativeStatusMessage = ""
    var statusText = ScanPhase.idle.statusText

    var hasChunkProgress: Bool {
        totalChunks > 0
    }

    var hasScannedChunks: Bool {
        normalizedScannedChunks > 0
    }

    var hasDecodedPayload: Bool {
        extractedBytes > 0
    }

    var isLockedFrameWithoutChunks: Bool {
        isRecognizedFrame && !hasDecodedPayload
    }

    var normalizedScannedChunks: Int {
        guard totalChunks > 0 else { return max(0, scannedChunks) }
        return min(max(0, scannedChunks), totalChunks)
    }

    var remainingChunks: Int? {
        guard totalChunks > 0 else { return nil }
        return max(0, totalChunks - normalizedScannedChunks)
    }

    var progressFraction: Double {
        guard totalChunks > 0 else { return 0 }
        return Double(normalizedScannedChunks) / Double(totalChunks)
    }

    var progressPercent: Int {
        Int((progressFraction * 100).rounded())
    }

    init() {}

    init(snapshot: ScanSnapshot) {
        phase = snapshot.phase
        isRecognizedFrame = snapshot.recognizedFrame
        needsSharpen = snapshot.needsSharpen
        extractedBytes = snapshot.extractedBytes
        completedFileID = snapshot.completedFileID > 0 ? snapshot.completedFileID : nil
        scannedChunks = snapshot.scannedChunks
        totalChunks = snapshot.totalChunks
        nativeStatusMessage = snapshot.statusMessage
        statusText = snapshot.phase.statusText
    }
}
