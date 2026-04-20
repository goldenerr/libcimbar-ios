import SwiftUI

struct ScanStatusView: View {
    let scanState: ScanState

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Label(scanState.phase.statusText, systemImage: phaseSymbol)
                .font(.headline)

            Text(detailText)
                .font(.subheadline)
                .foregroundStyle(.secondary)

            if scanState.totalChunks > 0 {
                VStack(alignment: .leading, spacing: 8) {
                    ProgressView(value: progressValue)
                        .tint(.accentColor)

                    HStack {
                        Label("已扫 \(scanState.scannedChunks)", systemImage: "viewfinder")
                        Spacer()
                        Label("总数 \(scanState.totalChunks)", systemImage: "square.grid.3x3")
                    }
                    .font(.caption)
                    .foregroundStyle(.secondary)
                }
            }

            if scanState.needsSharpen {
                Label("Try moving closer or improving lighting.", systemImage: "wand.and.stars")
                    .font(.caption)
                    .foregroundStyle(.yellow)
            }

            if let completedFileID = scanState.completedFileID {
                Label("Completed file #\(completedFileID)", systemImage: "checkmark.circle")
                    .font(.caption)
                    .foregroundStyle(.green)
            }
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 16))
    }

    private var detailText: String {
        switch scanState.phase {
        case .idle:
            return "Point the camera at an animated cimbar stream to begin."
        case .searching:
            return "Looking for a recognizable frame."
        case .detecting:
            return scanState.isRecognizedFrame ? "Frame locked. Waiting for payload data." : "Align the code fully in view."
        case .decoding:
            if scanState.totalChunks > 0 {
                return "已扫描 \(scanState.scannedChunks)/\(scanState.totalChunks) 个码，累计解出 \(scanState.extractedBytes) bytes。"
            }
            return "Decoded \(scanState.extractedBytes) bytes so far."
        case .completed:
            return "Transfer finished. You can share the received file now."
        case .error(let message):
            return message
        }
    }

    private var phaseSymbol: String {
        switch scanState.phase {
        case .idle, .searching:
            return "viewfinder"
        case .detecting:
            return "camera.metering.center.weighted"
        case .decoding:
            return "tray.and.arrow.down"
        case .completed:
            return "checkmark.circle.fill"
        case .error:
            return "exclamationmark.triangle.fill"
        }
    }

    private var progressValue: Double {
        guard scanState.totalChunks > 0 else { return 0 }
        return min(1.0, max(0.0, Double(scanState.scannedChunks) / Double(scanState.totalChunks)))
    }
}

#Preview {
    ScanStatusView(scanState: ScanState(snapshot: ScanSnapshot(phase: .decoding, recognizedFrame: true, needsSharpen: false, extractedBytes: 2048, completedFileID: 0, scannedChunks: 12, totalChunks: 40)))
        .padding()
        .background(Color.black)
}
