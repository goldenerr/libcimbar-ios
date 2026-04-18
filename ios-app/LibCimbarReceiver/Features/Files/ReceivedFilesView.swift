import SwiftUI

struct ReceivedFilesView: View {
    @EnvironmentObject private var appState: AppState

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
                        }
                    }
                }
            }
            .navigationTitle("Files")
        }
    }
}

#Preview {
    ReceivedFilesView()
        .environmentObject(AppState())
}
