import Foundation

struct ReceivedFile: Identifiable, Equatable {
    let id = UUID()
    let filename: String
    let byteCount: Int
    let receivedAt: Date

    var formattedSize: String {
        ByteCountFormatter.string(fromByteCount: Int64(byteCount), countStyle: .file)
    }
}
