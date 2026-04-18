import SwiftUI

struct ScanView: View {
    var body: some View {
        NavigationStack {
            VStack(spacing: 16) {
                Image(systemName: "viewfinder.circle")
                    .font(.system(size: 56))
                    .foregroundStyle(.accent)

                Text("Scanner placeholder")
                    .font(.title3)
                    .fontWeight(.semibold)

                Text("Camera capture and decoder integration will be added in a later task.")
                    .multilineTextAlignment(.center)
                    .foregroundStyle(.secondary)
                    .padding(.horizontal)
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .navigationTitle("Scan")
        }
    }
}

#Preview {
    ScanView()
}
