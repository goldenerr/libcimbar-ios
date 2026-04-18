import SwiftUI
import Combine

@MainActor
final class AppState: ObservableObject {
    @Published var receivedFiles: [ReceivedFile] = []

    let decoderBridge = CimbarDecoderBridgeService()
    private var cancellables: Set<AnyCancellable> = []

    init() {
        decoderBridge.objectWillChange
            .receive(on: DispatchQueue.main)
            .sink { [weak self] _ in
                self?.objectWillChange.send()
            }
            .store(in: &cancellables)
    }

    func connect(camera: CameraCaptureService) {
        camera.onSampleBuffer = { [decoderBridge] sampleBuffer in
            decoderBridge.process(sampleBuffer: sampleBuffer)
        }
    }
}
