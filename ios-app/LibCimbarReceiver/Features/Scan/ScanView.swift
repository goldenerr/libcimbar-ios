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
        switch camera.authorizationStatus {
        case .authorized:
            Label(
                camera.isRunning ? "Camera preview active" : "Starting camera…",
                systemImage: "camera.viewfinder"
            )
            .font(.headline)
            .padding()
            .frame(maxWidth: .infinity)
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
