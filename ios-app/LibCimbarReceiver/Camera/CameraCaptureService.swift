import AVFoundation
import SwiftUI

final class CameraCaptureService: NSObject, ObservableObject {
    let session = AVCaptureSession()

    private enum DisplayScanProfile {
        case macLock
        case windowsFallback

        var preferredZoom: CGFloat {
            switch self {
            case .macLock:
                return 3.0
            case .windowsFallback:
                return 2.15
            }
        }

        var minimumZoomToApply: CGFloat {
            switch self {
            case .macLock:
                return 2.0
            case .windowsFallback:
                return 1.25
            }
        }

        var preferredFramesPerSecond: Int32 {
            switch self {
            case .macLock:
                return 20
            case .windowsFallback:
                return 24
            }
        }

        var autoFocusRangeRestriction: AVCaptureDevice.AutoFocusRangeRestriction {
            switch self {
            case .macLock:
                return .near
            case .windowsFallback:
                return .none
            }
        }
    }

    @Published private(set) var prefersWindowsDisplayTuning = false

    private let output = AVCaptureVideoDataOutput()
    private let captureQueue = DispatchQueue(label: "camera.capture.queue")
    private let decodeQueue = DispatchQueue(label: "camera.decode.queue")

    @Published private(set) var authorizationStatus = AVCaptureDevice.authorizationStatus(for: .video)
    @Published private(set) var isRunning = false

    private var isConfigured = false
    private var decodeInFlight = false
    private var pendingSampleBuffer: CMSampleBuffer?
    private var activeCamera: AVCaptureDevice?
    private var activeDisplayScanProfile: DisplayScanProfile = .macLock

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
            self.applyCurrentDisplayScanProfileIfNeeded()
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

    func setWindowsDisplayTuningEnabled(_ enabled: Bool) {
        DispatchQueue.main.async {
            self.prefersWindowsDisplayTuning = enabled
        }

        captureQueue.async { [weak self] in
            guard let self else { return }
            self.activeDisplayScanProfile = enabled ? .windowsFallback : .macLock
            self.applyCurrentDisplayScanProfileIfNeeded()
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

        session.sessionPreset = .photo

        guard let camera = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .back) else {
            throw CameraCaptureError.noRearCamera
        }

        try configureCameraForScanning(camera)

        let input = try AVCaptureDeviceInput(device: camera)
        guard session.canAddInput(input) else {
            throw CameraCaptureError.cannotAddInput
        }
        session.addInput(input)

        output.alwaysDiscardsLateVideoFrames = true
        output.videoSettings = [
            kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
        ]
        output.setSampleBufferDelegate(self, queue: captureQueue)
        if let connection = output.connection(with: .video) {
            connection.videoOrientation = .portrait
            if connection.isVideoMirroringSupported {
                connection.isVideoMirrored = false
            }
        }

        guard session.canAddOutput(output) else {
            throw CameraCaptureError.cannotAddOutput
        }
        session.addOutput(output)
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

    private func configureCameraForScanning(_ camera: AVCaptureDevice) throws {
        try camera.lockForConfiguration()
        defer { camera.unlockForConfiguration() }

        if camera.isFocusPointOfInterestSupported {
            camera.focusPointOfInterest = CGPoint(x: 0.5, y: 0.5)
        }
        if camera.isExposurePointOfInterestSupported {
            camera.exposurePointOfInterest = CGPoint(x: 0.5, y: 0.5)
        }

        if camera.isFocusModeSupported(.continuousAutoFocus) {
            camera.focusMode = .continuousAutoFocus
        } else if camera.isFocusModeSupported(.autoFocus) {
            camera.focusMode = .autoFocus
        }

        if camera.isSmoothAutoFocusSupported {
            camera.isSmoothAutoFocusEnabled = false
        }

        if camera.isExposureModeSupported(.continuousAutoExposure) {
            camera.exposureMode = .continuousAutoExposure
        }

        if camera.isWhiteBalanceModeSupported(.continuousAutoWhiteBalance) {
            camera.whiteBalanceMode = .continuousAutoWhiteBalance
        }

        activeDisplayScanProfile = prefersWindowsDisplayTuning ? .windowsFallback : .macLock
        applyDisplayScanProfile(activeDisplayScanProfile, to: camera)
        activeCamera = camera
    }

    private func applyCurrentDisplayScanProfileIfNeeded() {
        guard let camera = activeCamera else { return }

        do {
            try camera.lockForConfiguration()
            applyDisplayScanProfile(activeDisplayScanProfile, to: camera)
            camera.unlockForConfiguration()
        } catch {
            return
        }
    }

    private func applyDisplayScanProfile(_ profile: DisplayScanProfile, to camera: AVCaptureDevice) {
        if camera.isAutoFocusRangeRestrictionSupported {
            camera.autoFocusRangeRestriction = profile.autoFocusRangeRestriction
        }

        let maxZoom = min(camera.activeFormat.videoMaxZoomFactor, camera.maxAvailableVideoZoomFactor)
        if maxZoom >= profile.minimumZoomToApply {
            camera.videoZoomFactor = min(profile.preferredZoom, maxZoom)
        }

        let preferredFrameDuration = CMTime(value: 1, timescale: profile.preferredFramesPerSecond)
        if camera.activeFormat.videoSupportedFrameRateRanges.contains(where: {
            $0.minFrameDuration <= preferredFrameDuration && $0.maxFrameDuration >= preferredFrameDuration
        }) {
            camera.activeVideoMinFrameDuration = preferredFrameDuration
            camera.activeVideoMaxFrameDuration = preferredFrameDuration
        }

        camera.isSubjectAreaChangeMonitoringEnabled = false
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
