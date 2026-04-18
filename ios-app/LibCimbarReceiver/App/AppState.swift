import SwiftUI

@MainActor
final class AppState: ObservableObject {
    @Published var receivedFiles: [ReceivedFile] = []

    let decoderBridge = CimbarDecoderBridgeService()

    func connect(camera: CameraCaptureService) {
        _ = decoderBridge
        camera.onSampleBuffer = { _ in }
    }
}
