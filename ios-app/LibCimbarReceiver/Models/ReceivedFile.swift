import Foundation

struct ReceivedFile: Identifiable, Codable, Equatable {
    let id: UUID
    let filename: String
    let relativePath: String
    let size: Int
    let receivedAt: Date

    var formattedSize: String {
        ByteCountFormatter.string(fromByteCount: Int64(size), countStyle: .file)
    }
}
