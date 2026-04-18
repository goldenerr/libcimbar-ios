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
            ContentUnavailableView(
                "Camera Unavailable",
                systemImage: "camera.fill.badge.xmark",
                description: Text("Enable camera access in Settings to scan cimbar codes.")
            )
            .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 16))
        @unknown default:
            EmptyView()
        }
    }

    private var topControls: some View {
        HStack {
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
