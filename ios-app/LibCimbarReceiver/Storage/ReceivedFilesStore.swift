import Foundation

struct ReceivedFilesStore {
    private let fileManager: FileManager
    private let encoder: JSONEncoder
    private let decoder: JSONDecoder

    init(fileManager: FileManager = .default) {
        self.fileManager = fileManager

        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        encoder.dateEncodingStrategy = .iso8601
        self.encoder = encoder

        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        self.decoder = decoder
    }

    func loadReceivedFiles() -> [ReceivedFile] {
        do {
            try ensureStorageDirectoryExists()

            guard fileManager.fileExists(atPath: indexURL.path) else {
                return []
            }

            let data = try Data(contentsOf: indexURL)
            let files = try decoder.decode([ReceivedFile].self, from: data)
            return files.sorted { $0.receivedAt > $1.receivedAt }
        } catch {
            return []
        }
    }

    func saveCompletedFile(filename: String, data: Data) throws -> ReceivedFile {
        try ensureStorageDirectoryExists()

        let id = UUID()
        let safeFilename = sanitizedFilename(filename, fallbackID: id)
        let relativePath = "\(id.uuidString)-\(safeFilename)"
        let fileURL = receivedFilesDirectoryURL.appendingPathComponent(relativePath)

        try data.write(to: fileURL, options: .atomic)

        let receivedFile = ReceivedFile(
            id: id,
            filename: safeFilename,
            relativePath: relativePath,
            size: data.count,
            receivedAt: Date()
        )

        var files = loadReceivedFiles()
        files.insert(receivedFile, at: 0)
        let indexData = try encoder.encode(files)
        try indexData.write(to: indexURL, options: .atomic)

        return receivedFile
    }

    func fileURL(for file: ReceivedFile) -> URL {
        receivedFilesDirectoryURL.appendingPathComponent(file.relativePath)
    }

    private var appSupportDirectoryURL: URL {
        let baseURL = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
            ?? fileManager.temporaryDirectory.appendingPathComponent("ApplicationSupport", isDirectory: true)
        return baseURL
    }

    private var receivedFilesDirectoryURL: URL {
        appSupportDirectoryURL.appendingPathComponent("ReceivedFiles", isDirectory: true)
    }

    private var indexURL: URL {
        receivedFilesDirectoryURL.appendingPathComponent("index.json")
    }

    private func ensureStorageDirectoryExists() throws {
        try fileManager.createDirectory(at: receivedFilesDirectoryURL, withIntermediateDirectories: true)
    }

    private func sanitizedFilename(_ filename: String, fallbackID: UUID) -> String {
        let candidate = URL(fileURLWithPath: filename).lastPathComponent.trimmingCharacters(in: .whitespacesAndNewlines)
        if candidate.isEmpty {
            return "received-\(fallbackID.uuidString).bin"
        }
        return candidate
    }
}
