import SwiftUI
import Combine
import Foundation

@MainActor
final class AppState: ObservableObject {
    @Published var receivedFiles: [ReceivedFile] = []

    let store: ReceivedFilesStore
    let decoderBridge: CimbarDecoderBridgeService
    private var cancellables: Set<AnyCancellable> = []

    init(
        store: ReceivedFilesStore = ReceivedFilesStore(),
        decoderBridge: CimbarDecoderBridgeService = CimbarDecoderBridgeService()
    ) {
        self.store = store
        self.decoderBridge = decoderBridge
        self.receivedFiles = store.loadReceivedFiles()

        decoderBridge.onCompletedFile = { [weak self] filename, data in
            Task { @MainActor [weak self] in
                self?.persistCompletedFile(filename: filename, data: data)
            }
        }

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

    private func persistCompletedFile(filename: String, data: Data) {
        do {
            _ = try store.saveCompletedFile(filename: filename, data: data)
            receivedFiles = store.loadReceivedFiles()
        } catch {
            return
        }
    }
}
