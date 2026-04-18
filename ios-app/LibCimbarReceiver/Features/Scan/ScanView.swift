import AVFoundation
import SwiftUI

struct ScanView: View {
    @EnvironmentObject private var appState: AppState
    @StateObject private var camera = CameraCaptureService()

    var body: some View {
        NavigationStack {
            ZStack {
                CameraPreviewView(session: camera.session)
                    .ignoresSafeArea()

                VStack(spacing: 16) {
                    Spacer()

                    statusView
                }
                .padding()
            }
            .background(Color.black)
            .navigationTitle("Scan")
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
    private var statusView: some View {
        let scanState = appState.decoderBridge.scanState

        switch camera.authorizationStatus {
        case .authorized:
            VStack(alignment: .leading, spacing: 8) {
                Label(
                    camera.isRunning ? scanState.statusText : "Starting camera…",
                    systemImage: "camera.viewfinder"
                )
                .font(.headline)

                if scanState.isRecognizedFrame || scanState.extractedBytes > 0 {
                    Text("Recognized: \(scanState.isRecognizedFrame ? "yes" : "no") · Bytes: \(scanState.extractedBytes)")
                        .font(.subheadline)
                }

                if scanState.needsSharpen {
                    Text("Frame may need sharpening")
                        .font(.subheadline)
                }

                if let completedFileID = scanState.completedFileID {
                    Text("Completed file ID: \(completedFileID)")
                        .font(.subheadline)
                }
            }
            .padding()
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 16))
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
}

#Preview {
    ScanView()
        .environmentObject(AppState())
}
