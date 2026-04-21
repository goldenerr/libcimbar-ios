import AVFoundation
import Foundation

struct CompletedDecodedFile {
    let filename: String
    let data: Data
}

final class CimbarDecoderBridgeService: ObservableObject {
    @Published private(set) var scanState = ScanState()

    var onCompletedFile: ((String, Data) -> Void)?

    private let native = CimbarDecoderBridge()

    func reset() {
        native.reset()
        scanState = ScanState()
    }

    func configure(mode: Int) {
        native.configureMode(mode)
    }

    @discardableResult
    func process(sampleBuffer: CMSampleBuffer) -> ScanSnapshot? {
        guard let nativeSnapshot = native.processSampleBuffer(sampleBuffer) else {
            return nil
        }

        let snapshot = ScanSnapshot(nativeSnapshot: nativeSnapshot)
        let nextState = ScanState(snapshot: snapshot)

        if Thread.isMainThread {
            scanState = nextState
        } else {
            DispatchQueue.main.async { [weak self] in
                self?.scanState = nextState
            }
        }

        if let completedFile = takeCompletedFile() {
            onCompletedFile?(completedFile.filename, completedFile.data)
        }

        return snapshot
    }

    private func takeCompletedFile() -> CompletedDecodedFile? {
        guard let completedFile = native.takeCompletedFile() else {
            return nil
        }

        return CompletedDecodedFile(filename: completedFile.filename, data: completedFile.data)
    }
}

private extension ScanSnapshot {
    init(nativeSnapshot: CimbarDecoderBridgeSnapshot) {
        let phase = ScanPhase(bridgePhase: nativeSnapshot.phase)
        let finalPhase: ScanPhase
        if case .error = phase {
            finalPhase = phase
        } else if phase == .detecting && nativeSnapshot.extractedBytes > 0 {
            finalPhase = .decoding
        } else {
            finalPhase = phase
        }

        self.init(
            phase: finalPhase,
            recognizedFrame: nativeSnapshot.recognizedFrame,
            needsSharpen: nativeSnapshot.needsSharpen,
            extractedBytes: nativeSnapshot.extractedBytes,
            completedFileID: nativeSnapshot.completedFileID,
            scannedChunks: nativeSnapshot.scannedChunks,
            totalChunks: nativeSnapshot.totalChunks,
            statusMessage: nativeSnapshot.statusMessage
        )
    }
}

private extension ScanPhase {
    init(bridgePhase: CimbarDecoderBridgePhase) {
        switch bridgePhase {
        case .idle:
            self = .idle
        case .searching:
            self = .searching
        case .detecting:
            self = .detecting
        case .decoding, .reconstructing:
            self = .decoding
        case .completed:
            self = .completed
        case .error:
            self = .error("Decoder error")
        @unknown default:
            self = .error("Unknown decoder state")
        }
    }
}
