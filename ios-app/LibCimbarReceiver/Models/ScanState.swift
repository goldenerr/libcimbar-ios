import Foundation

enum ScanPhase: Equatable {
    case idle
    case searching
    case detecting
    case decoding
    case reconstructing
    case completed
    case error(String)
}

struct ScanState: Equatable {
    var phase: ScanPhase = .idle
    var isRecognizedFrame = false
    var needsSharpen = false
    var extractedBytes = 0
    var completedFileID: Int64?
}
