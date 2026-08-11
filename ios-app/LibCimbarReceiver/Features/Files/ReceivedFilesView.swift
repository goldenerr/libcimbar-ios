import SwiftUI

struct ReceivedFilesView: View {
    @EnvironmentObject private var appState: AppState
    @State private var filePendingDeletion: ReceivedFile?
    @State private var deletionErrorMessage = ""
    @State private var isShowingDeletionError = false

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
                    UnavailableStateView(
                        title: "No Files Yet",
                        systemImage: "doc.text.magnifyingglass",
                        description: "Completed transfers will appear here."
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

                            Button(role: .destructive) {
                                filePendingDeletion = file
                            } label: {
                                Label("Delete", systemImage: "trash")
                            }
                        }
                        .swipeActions(edge: .leading, allowsFullSwipe: true) {
                            Button {
                                appState.presentShareSheet(for: file)
                            } label: {
                                Label("Share", systemImage: "square.and.arrow.up")
                            }
                            .tint(.accentColor)
                        }
                        .swipeActions(edge: .trailing, allowsFullSwipe: false) {
                            Button(role: .destructive) {
                                filePendingDeletion = file
                            } label: {
                                Label("Delete", systemImage: "trash")
                            }
                        }
                    }
                }
            }
            .navigationTitle("Files")
        }
        .sheet(item: $appState.shareSheetFile, onDismiss: appState.dismissShareSheet) { file in
            ShareSheet(items: [appState.fileURL(for: file)])
        }
        .confirmationDialog(
            "Delete \(filePendingDeletion?.filename ?? "file")?",
            isPresented: Binding(
                get: { filePendingDeletion != nil },
                set: { if !$0 { filePendingDeletion = nil } }
            ),
            titleVisibility: .visible
        ) {
            Button("Delete", role: .destructive) {
                deletePendingFile()
            }
            Button("Cancel", role: .cancel) {
                filePendingDeletion = nil
            }
        } message: {
            Text("This permanently removes the received file from this device.")
        }
        .alert("Unable to Delete File", isPresented: $isShowingDeletionError) {
            Button("OK", role: .cancel) {}
        } message: {
            Text(deletionErrorMessage)
        }
    }

    private func deletePendingFile() {
        guard let file = filePendingDeletion else { return }
        filePendingDeletion = nil

        do {
            try appState.deleteReceivedFile(file)
        } catch {
            deletionErrorMessage = error.localizedDescription
            isShowingDeletionError = true
        }
    }
}

struct ReceivedFilesView_Previews: PreviewProvider {
    static var previews: some View {
        ReceivedFilesView()
            .environmentObject(AppState())
    }
}
