import SwiftUI

@MainActor
final class CimbarDecoderBridgeService: ObservableObject {
    @Published private(set) var scanState = ScanState()

    private let native = CimbarDecoderBridge()

    func reset() {
        native.reset()
        scanState = ScanState()
    }

    func configure(mode: Int) {
        native.configureMode(mode)
    }
}
