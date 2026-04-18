import SwiftUI

struct ReceivedFilesView: View {
    @EnvironmentObject private var appState: AppState

    private let dateFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateStyle = .medium
        formatter.timeStyle = .short
        return formatter
    }()

    var body: some View {
        NavigationStack {
            Group {
                if appState.receivedFiles.isEmpty {
                    ContentUnavailableView(
                        "No Files Yet",
                        systemImage: "doc.text.magnifyingglass",
                        description: Text("Completed transfers will appear here.")
                    )
                } else {
                    List(appState.receivedFiles) { file in
                        VStack(alignment: .leading, spacing: 4) {
                            Text(file.filename)
                                .font(.headline)
                            Text(file.formattedSize)
                                .font(.subheadline)
                                .foregroundStyle(.secondary)
                            Text(file.relativePath)
                                .font(.caption)
                                .foregroundStyle(.secondary)
                            Text(dateFormatter.string(from: file.receivedAt))
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .contentShape(Rectangle())
                        .onTapGesture {
                            appState.presentShareSheet(for: file)
                        }
                        .contextMenu {
                            Button {
                                appState.presentShareSheet(for: file)
                            } label: {
                                Label("Share", systemImage: "square.and.arrow.up")
                            }
                        }
                        .swipeActions(edge: .trailing, allowsFullSwipe: true) {
                            Button {
                                appState.presentShareSheet(for: file)
                            } label: {
                                Label("Share", systemImage: "square.and.arrow.up")
                            }
                            .tint(.accentColor)
                        }
                    }
                }
            }
            .navigationTitle("Files")
        }
        .sheet(item: $appState.shareSheetFile, onDismiss: appState.dismissShareSheet) { file in
            ShareSheet(items: [appState.fileURL(for: file)])
        }
    }
}

#Preview {
    ReceivedFilesView()
        .environmentObject(AppState())
}
