import AVFoundation
import Foundation

struct CompletedDecodedFile {
    let filename: String
    let data: Data
}

final class CimbarDecoderBridgeService: ObservableObject {
    @Published private(set) var scanState = ScanState()
    @Published private(set) var diagnosticsSummary = ""
    @Published private(set) var lastSnapshotSummary = ""

    var onCompletedFile: ((String, Data) -> Void)?

    private let native = CimbarDecoderBridge()
    private var incomingFrameCount = 0
    private var processAttemptCount = 0
    private var nilSnapshotCount = 0
    private var successfulSnapshotCount = 0
    private var lastSnapshotAt: Date?
    private var lastDiagnosticsPublishAt: Date?
    private let diagnosticsPublishInterval: TimeInterval = 0.25

    func noteIncomingFrame() {
        incomingFrameCount += 1
    }

    func reset() {
        native.reset()
        scanState = ScanState()
        diagnosticsSummary = ""
        lastSnapshotSummary = ""
        incomingFrameCount = 0
        processAttemptCount = 0
        nilSnapshotCount = 0
        successfulSnapshotCount = 0
        lastSnapshotAt = nil
        lastDiagnosticsPublishAt = nil
    }

    func configure(mode: Int) {
        native.configureMode(mode)
    }

    @discardableResult
    func process(sampleBuffer: CMSampleBuffer) -> ScanSnapshot? {
        processAttemptCount += 1
        guard let nativeSnapshot = native.processSampleBuffer(sampleBuffer) else {
            nilSnapshotCount += 1
            publishDiagnosticsIfNeeded(lastEvent: "native=nil")
            return nil
        }

        successfulSnapshotCount += 1
        lastSnapshotAt = Date()
        let snapshot = ScanSnapshot(nativeSnapshot: nativeSnapshot)
        let nextState = ScanState(snapshot: snapshot)
        publishDiagnosticsIfNeeded(lastEvent: "native=ok")
        publishLastSnapshotSummary(snapshot)

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

    private func publishDiagnosticsIfNeeded(lastEvent: String) {
        let now = Date()
        if let lastDiagnosticsPublishAt,
           now.timeIntervalSince(lastDiagnosticsPublishAt) < diagnosticsPublishInterval,
           lastEvent != "native=nil" {
            return
        }

        let lastAgeText: String
        if let lastSnapshotAt {
            lastAgeText = String(format: "%.1fs", now.timeIntervalSince(lastSnapshotAt))
        } else {
            lastAgeText = "never"
        }

        lastDiagnosticsPublishAt = now
        let summary = "diag frames=\(incomingFrameCount) attempts=\(processAttemptCount) ok=\(successfulSnapshotCount) nil=\(nilSnapshotCount) last=\(lastEvent) lastOK=\(lastAgeText)"
        if Thread.isMainThread {
            diagnosticsSummary = summary
        } else {
            DispatchQueue.main.async { [weak self] in
                self?.diagnosticsSummary = summary
            }
        }
    }

    private func publishLastSnapshotSummary(_ snapshot: ScanSnapshot) {
        let status = snapshot.statusMessage.isEmpty ? "<empty>" : snapshot.statusMessage
        let summary = "snap phase=\(snapshot.phase.debugLabel) recognized=\(snapshot.recognizedFrame ? 1 : 0) sharpen=\(snapshot.needsSharpen ? 1 : 0) bytes=\(snapshot.extractedBytes) chunks=\(snapshot.scannedChunks)/\(snapshot.totalChunks) status=\(status)"
        guard summary != lastSnapshotSummary else {
            return
        }
        if Thread.isMainThread {
            lastSnapshotSummary = summary
        } else {
            DispatchQueue.main.async { [weak self] in
                self?.lastSnapshotSummary = summary
            }
        }
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
    var debugLabel: String {
        switch self {
        case .idle:
            return "idle"
        case .searching:
            return "searching"
        case .detecting:
            return "detecting"
        case .decoding:
            return "decoding"
        case .completed:
            return "completed"
        case .error:
            return "error"
        }
    }

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
