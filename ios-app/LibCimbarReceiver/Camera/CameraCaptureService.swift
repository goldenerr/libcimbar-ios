import AVFoundation
import SwiftUI

final class CameraCaptureService: NSObject, ObservableObject {
    let session = AVCaptureSession()

    private let output = AVCaptureVideoDataOutput()
    private let captureQueue = DispatchQueue(label: "camera.capture.queue")
    private let decodeQueue = DispatchQueue(label: "camera.decode.queue")

    @Published private(set) var authorizationStatus = AVCaptureDevice.authorizationStatus(for: .video)
    @Published private(set) var isRunning = false

    private var isConfigured = false
    private var decodeInFlight = false
    private var pendingSampleBuffer: CMSampleBuffer?

    var onSampleBuffer: ((CMSampleBuffer) -> ScanSnapshot?)?

    func start() {
        switch AVCaptureDevice.authorizationStatus(for: .video) {
        case .authorized:
            authorizationStatus = .authorized
            startSession()
        case .notDetermined:
            AVCaptureDevice.requestAccess(for: .video) { [weak self] granted in
                DispatchQueue.main.async {
                    self?.authorizationStatus = granted ? .authorized : .denied
                }
                guard granted else { return }
                self?.startSession()
            }
        case .denied, .restricted:
            authorizationStatus = AVCaptureDevice.authorizationStatus(for: .video)
        @unknown default:
            authorizationStatus = AVCaptureDevice.authorizationStatus(for: .video)
        }
    }

    func stop() {
        captureQueue.async { [weak self] in
            guard let self else { return }
            self.pendingSampleBuffer = nil
            self.decodeInFlight = false
            guard self.session.isRunning else {
                DispatchQueue.main.async {
                    self.isRunning = false
                }
                return
            }
            self.session.stopRunning()
            DispatchQueue.main.async {
                self.isRunning = false
            }
        }
    }

    private func startSession() {
        captureQueue.async { [weak self] in
            guard let self else { return }
            if !self.isConfigured {
                do {
                    try self.configureSession()
                    self.isConfigured = true
                } catch {
                    return
                }
            }
            guard !self.session.isRunning else {
                DispatchQueue.main.async {
                    self.isRunning = true
                }
                return
            }
            self.session.startRunning()
            DispatchQueue.main.async {
                self.isRunning = true
            }
        }
    }

    private func configureSession() throws {
        session.beginConfiguration()
        defer { session.commitConfiguration() }

        session.sessionPreset = .high

        guard let camera = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .back) else {
            throw CameraCaptureError.noRearCamera
        }

        let input = try AVCaptureDeviceInput(device: camera)
        guard session.canAddInput(input) else {
            throw CameraCaptureError.cannotAddInput
        }
        session.addInput(input)

        output.alwaysDiscardsLateVideoFrames = true
        output.videoSettings = [
            kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA
        ]
        output.setSampleBufferDelegate(self, queue: captureQueue)

        guard session.canAddOutput(output) else {
            throw CameraCaptureError.cannotAddOutput
        }
        session.addOutput(output)

        output.connection(with: .video)?.videoOrientation = .portrait
    }

    private func enqueueForDecode(_ sampleBuffer: CMSampleBuffer) {
        guard !decodeInFlight else {
            pendingSampleBuffer = sampleBuffer
            return
        }

        decodeInFlight = true
        dispatchToDecode(sampleBuffer)
    }

    private func dispatchToDecode(_ sampleBuffer: CMSampleBuffer) {
        let callback = onSampleBuffer
        decodeQueue.async { [weak self] in
            _ = callback?(sampleBuffer)
            self?.captureQueue.async {
                guard let self else { return }
                if let pending = self.pendingSampleBuffer {
                    self.pendingSampleBuffer = nil
                    self.dispatchToDecode(pending)
                } else {
                    self.decodeInFlight = false
                }
            }
        }
    }
}

extension CameraCaptureService: AVCaptureVideoDataOutputSampleBufferDelegate {
    func captureOutput(
        _ output: AVCaptureOutput,
        didOutput sampleBuffer: CMSampleBuffer,
        from connection: AVCaptureConnection
    ) {
        enqueueForDecode(sampleBuffer)
    }
}

private enum CameraCaptureError: Error {
    case noRearCamera
    case cannotAddInput
    case cannotAddOutput
}
