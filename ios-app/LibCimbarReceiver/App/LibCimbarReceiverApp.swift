import SwiftUI

@main
struct LibCimbarReceiverApp: App {
    @StateObject private var appState = AppState()

    var body: some Scene {
        WindowGroup {
            TabView {
                ScanView()
                    .tabItem {
                        Label("Scan", systemImage: "viewfinder")
                    }

                ReceivedFilesView()
                    .tabItem {
                        Label("Files", systemImage: "folder")
                    }
            }
            .environmentObject(appState)
        }
    }
}
