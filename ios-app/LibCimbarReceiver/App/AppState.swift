import SwiftUI

final class AppState: ObservableObject {
    @Published var receivedFiles: [ReceivedFile] = []
}
