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

            if scanState.hasChunkProgress {
                VStack(alignment: .leading, spacing: 8) {
                    HStack(alignment: .firstTextBaseline) {
                        Label("进度 \(scanState.progressPercent)%", systemImage: "chart.bar.fill")
                        Spacer()
                        if let remainingChunks = scanState.remainingChunks {
                            Text("还差 \(remainingChunks) 码")
                        }
                    }
                    .font(.caption)
                    .foregroundStyle(.secondary)

                    ProgressView(value: scanState.progressFraction)
                        .tint(.accentColor)

                    HStack {
                        Label("已扫 \(scanState.normalizedScannedChunks)", systemImage: "viewfinder")
                        Spacer()
                        Label("总数 \(scanState.totalChunks)", systemImage: "square.grid.3x3")
                    }
                    .font(.caption)
                    .foregroundStyle(.secondary)
                }
            } else {
                HStack {
                    Label("已扫 \(scanState.normalizedScannedChunks) 个码", systemImage: "viewfinder")
                    Spacer()
                    Text("总数 未知")
                }
                .font(.caption)
                .foregroundStyle(.secondary)
            }

            if scanState.isLockedFrameWithoutChunks {
                Label("诊断：已锁定码框，但这一帧还没解出有效 chunk。", systemImage: "stethoscope")
                    .font(.caption)
                    .foregroundStyle(.yellow)
                    .lineLimit(nil)
                    .multilineTextAlignment(.leading)
                    .fixedSize(horizontal: false, vertical: true)
            } else if scanState.hasDecodedPayload && !scanState.hasChunkProgress {
                Label("诊断：已读到 payload bytes，正在等待 fountain 元数据给出总码数。", systemImage: "waveform.path.ecg")
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(nil)
                    .multilineTextAlignment(.leading)
                    .fixedSize(horizontal: false, vertical: true)
            }

            if !scanState.nativeStatusMessage.isEmpty {
                Label("Native: \(scanState.nativeStatusMessage)", systemImage: "terminal")
                    .font(.caption2)
                    .foregroundStyle(.secondary)
                    .lineLimit(nil)
                    .multilineTextAlignment(.leading)
                    .fixedSize(horizontal: false, vertical: true)
                    .textSelection(.enabled)
            }

            if scanState.needsSharpen {
                Label("Try moving closer or improving lighting.", systemImage: "wand.and.stars")
                    .font(.caption)
                    .foregroundStyle(.yellow)
                    .lineLimit(nil)
                    .multilineTextAlignment(.leading)
                    .fixedSize(horizontal: false, vertical: true)
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
        .overlay(
            RoundedRectangle(cornerRadius: 16)
                .stroke(borderColor, lineWidth: 1)
        )
    }

    private var detailText: String {
        switch scanState.phase {
        case .idle:
            return "Point the camera at an animated cimbar stream to begin."
        case .searching:
            return scanState.nativeStatusMessage.isEmpty ? "Looking for a recognizable frame." : nativeStatusDescription
        case .detecting:
            return scanState.isLockedFrameWithoutChunks
                ? "已锁定码框，但这一帧还没读出有效数据。"
                : nativeStatusDescription
        case .decoding:
            if scanState.hasChunkProgress {
                let remainingText = scanState.remainingChunks.map { "，还差 \($0) 个码" } ?? ""
                return "已扫描 \(scanState.normalizedScannedChunks)/\(scanState.totalChunks) 个码（\(scanState.progressPercent)%）\(remainingText)，累计解出 \(scanState.extractedBytes) bytes。"
            }
            if scanState.hasScannedChunks {
                return "已扫描 \(scanState.normalizedScannedChunks) 个码，累计解出 \(scanState.extractedBytes) bytes。"
            }
            return nativeStatusDescription
        case .completed:
            return "Transfer finished. You can share the received file now."
        case .error(let message):
            return message
        }
    }

    private var nativeStatusDescription: String {
        if !scanState.nativeStatusMessage.isEmpty {
            switch scanState.nativeStatusMessage {
            case "searching":
                return "Looking for a recognizable frame."
            case "recognized frame without chunks":
                return "已识别到码框，但这一帧还没解出有效 chunk。"
            case "recognized frame without chunks after clarity fallback":
                return "已识别到码框，clarity fallback 也试过了，但这一帧仍没解出有效 chunk。"
            case "recognized frame without chunks after third-tier fallback":
                return "已识别到码框，第三档多变体 fallback 也试过了，但这一帧仍没解出有效 chunk。"
            case "recognized frame without chunks after fourth-tier fallback":
                return "已识别到码框，第四档 soft-only fallback 也试过了，但这一帧仍没解出有效 chunk。"
            case "recognized frame without chunks after second-round fallback":
                return "已识别到码框，第二轮显示器兼容 fallback 也试过了，但这一帧仍没解出有效 chunk。"
            case "recognized frame without chunks after third-round display fallback":
                return "已识别到码框，第三轮显示器兼容 fallback 也试过了，但这一帧仍没解出有效 chunk。"
            case "decoded frame chunks":
                return "这一帧已经解出了 chunk，正在推进重组进度。"
            case "decoded frame chunks after clarity fallback":
                return "这一帧原本太糊，clarity fallback 已救回部分 chunk，正在推进重组进度。"
            case "decoded frame chunks after display crop fallback":
                return "这一帧命中了居中取景裁切 fallback，已救回部分 chunk，正在推进重组进度。"
            case "decoded frame chunks after secondary clarity fallback":
                return "这一帧更糊，第二档 fallback 已救回部分 chunk，正在推进重组进度。"
            case "decoded frame chunks after third-tier fallback":
                return "这一帧命中了第三档多变体 fallback，已救回部分 chunk，正在推进重组进度。"
            case "decoded frame chunks after fourth-tier fallback":
                return "这一帧命中了第四档 soft-only fallback，已救回部分 chunk，正在推进重组进度。"
            case "decoded frame chunks after second-round fallback":
                return "这一帧命中了第二轮显示器兼容 fallback，已救回部分 chunk，正在推进重组进度。"
            case "decoded frame chunks after third-round display fallback":
                return "这一帧命中了第三轮显示器兼容 fallback，已救回部分 chunk，正在推进重组进度。"
            case "completed file":
                return "文件已重组完成。"
            case "invalid frame":
                return "当前相机帧无效，等待下一帧。"
            case "failed to recover completed file":
                return "文件已拼齐，但在恢复输出文件时失败。"
            default:
                return scanState.nativeStatusMessage
            }
        }

        switch scanState.phase {
        case .searching:
            return "Looking for a recognizable frame."
        case .detecting:
            return scanState.isRecognizedFrame ? "已锁定码框，但这一帧还没读出有效数据。" : "Align the code fully in view."
        case .decoding:
            return "Decoded \(scanState.extractedBytes) bytes so far."
        default:
            return scanState.statusText
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

    private var borderColor: Color {
        switch scanState.phase {
        case .completed:
            return .green.opacity(0.45)
        case .error:
            return .red.opacity(0.55)
        case .detecting:
            return .yellow.opacity(0.4)
        case .decoding:
            return .accentColor.opacity(0.4)
        case .idle, .searching:
            return .white.opacity(0.18)
        }
    }
}

#Preview {
    ScanStatusView(scanState: ScanState(snapshot: ScanSnapshot(phase: .decoding, recognizedFrame: true, needsSharpen: false, extractedBytes: 2048, completedFileID: 0, scannedChunks: 12, totalChunks: 40, statusMessage: "decoded frame chunks")))
        .padding()
        .background(Color.black)
}
