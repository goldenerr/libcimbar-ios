import AVFoundation
import SwiftUI

struct ScanView: View {
    @EnvironmentObject private var appState: AppState
    @StateObject private var camera = CameraCaptureService()
    @State private var isFlashlightPlaceholderEnabled = false

    var body: some View {
        NavigationStack {
            ZStack {
                CameraPreviewView(session: camera.session)
                    .ignoresSafeArea()

                displayScanGuide

                VStack(spacing: 16) {
                    topControls
                    Spacer()
                    statusPanel
                }
                .padding()
            }
            .background(Color.black)
            .navigationTitle("Scan")
        }
        .sheet(item: $appState.latestReceivedFile, onDismiss: appState.dismissLatestReceivedFile) { file in
            successSheet(for: file)
        }
        .sheet(item: $appState.shareSheetFile, onDismiss: appState.dismissShareSheet) { file in
            ShareSheet(items: [appState.fileURL(for: file)])
        }
        .onAppear {
            appState.connect(camera: camera)
            camera.start()
        }
        .onDisappear {
            camera.stop()
        }
    }

    @ViewBuilder
    private var statusPanel: some View {
        switch camera.authorizationStatus {
        case .authorized:
            VStack(alignment: .leading, spacing: 12) {
                if let latestReceivedFile = appState.latestReceivedFile {
                    successBanner(for: latestReceivedFile)
                }

                ScanStatusView(scanState: appState.decoderBridge.scanState)

                HStack(spacing: 12) {
                    Button {
                        appState.resetScanningSession()
                    } label: {
                        Label("Reset", systemImage: "arrow.clockwise")
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.borderedProminent)

                    NavigationLink {
                        ReceivedFilesView()
                    } label: {
                        Label("Files", systemImage: "folder")
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                }
            }
        case .notDetermined:
            ProgressView("Requesting camera access…")
                .padding()
                .frame(maxWidth: .infinity)
                .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 16))
        case .denied, .restricted:
            UnavailableStateView(
                title: "Camera Unavailable",
                systemImage: "camera.fill.badge.xmark",
                description: "Enable camera access in Settings to scan cimbar codes."
            )
        @unknown default:
            EmptyView()
        }
    }

    private var topControls: some View {
        HStack {
            VStack(alignment: .leading, spacing: 6) {
                Label("显示器扫码模式", systemImage: "display")
                    .font(.headline)
                    .foregroundStyle(.white)
                Text("把码尽量放进中间取景框，贴近一些，填满大约 70% 宽度再停稳。")
                    .font(.caption)
                    .foregroundStyle(.white.opacity(0.85))
                if !camera.cameraDebugSummary.isEmpty {
                    Text(camera.cameraDebugSummary)
                        .font(.caption2.monospaced())
                        .foregroundStyle(.white.opacity(0.75))
                        .textSelection(.enabled)
                }
                if !appState.decoderBridge.diagnosticsSummary.isEmpty {
                    Text(appState.decoderBridge.diagnosticsSummary)
                        .font(.caption2.monospaced())
                        .foregroundStyle(.white.opacity(0.72))
                        .lineLimit(nil)
                        .multilineTextAlignment(.leading)
                        .textSelection(.enabled)
                }
                if !appState.decoderBridge.lastSnapshotSummary.isEmpty {
                    Text(appState.decoderBridge.lastSnapshotSummary)
                        .font(.caption2.monospaced())
                        .foregroundStyle(.white.opacity(0.72))
                        .lineLimit(nil)
                        .multilineTextAlignment(.leading)
                        .textSelection(.enabled)
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 10)
            .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 16))

            Spacer()

            Button {
                isFlashlightPlaceholderEnabled.toggle()
            } label: {
                Label(
                    isFlashlightPlaceholderEnabled ? "Torch Soon" : "Flashlight",
                    systemImage: isFlashlightPlaceholderEnabled ? "flashlight.on.fill" : "flashlight.off.fill"
                )
            }
            .buttonStyle(.bordered)
            .tint(.white)
            .foregroundStyle(.white)
            .background(.ultraThinMaterial, in: Capsule())
            .accessibilityHint("Placeholder toggle until torch controls are wired up.")
        }
    }

    private var displayScanGuide: some View {
        GeometryReader { geometry in
            let width = min(geometry.size.width * 0.82, 340)
            let height = width * 0.68

            VStack {
                Spacer()
                RoundedRectangle(cornerRadius: 24)
                    .strokeBorder(Color.green.opacity(0.92), lineWidth: 3)
                    .frame(width: width, height: height)
                    .background(
                        RoundedRectangle(cornerRadius: 24)
                            .fill(Color.black.opacity(0.10))
                    )
                    .overlay(alignment: .top) {
                        Text("把屏幕上的码对准这里")
                            .font(.caption.weight(.semibold))
                            .padding(.horizontal, 10)
                            .padding(.vertical, 6)
                            .background(.ultraThinMaterial, in: Capsule())
                            .offset(y: -18)
                    }
                Spacer()
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .ignoresSafeArea()
        .allowsHitTesting(false)
    }

    private func successBanner(for file: ReceivedFile) -> some View {
        HStack(alignment: .center, spacing: 12) {
            Image(systemName: "checkmark.circle.fill")
                .foregroundStyle(.green)
                .font(.title3)

            VStack(alignment: .leading, spacing: 4) {
                Text("Received \(file.filename)")
                    .font(.headline)
                Text(file.formattedSize)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            Button("Share") {
                appState.presentShareSheet(for: file)
            }
            .buttonStyle(.bordered)
        }
        .padding()
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))
    }

    private func successSheet(for file: ReceivedFile) -> some View {
        NavigationStack {
            VStack(spacing: 20) {
                Image(systemName: "checkmark.circle.fill")
                    .font(.system(size: 60))
                    .foregroundStyle(.green)

                Text("Transfer Complete")
                    .font(.title2.weight(.semibold))

                VStack(spacing: 6) {
                    Text(file.filename)
                        .font(.headline)
                    Text(file.formattedSize)
                        .foregroundStyle(.secondary)
                }

                Button {
                    appState.dismissLatestReceivedFile()
                    appState.presentShareSheet(for: file)
                } label: {
                    Label("Share File", systemImage: "square.and.arrow.up")
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.borderedProminent)

                Button("Keep Scanning") {
                    appState.dismissLatestReceivedFile()
                }
                .buttonStyle(.bordered)
            }
            .padding(24)
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
            .background(Color(.systemGroupedBackground))
            .navigationTitle("Success")
            .navigationBarTitleDisplayMode(.inline)
        }
        .presentationDetents([.medium])
    }
}

#Preview {
    ScanView()
        .environmentObject(AppState())
}
