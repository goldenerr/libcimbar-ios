#import "CimbarDecoderBridge.h"

#import "cimbar_ios_recv_c.h"

#import <CoreVideo/CoreVideo.h>

@implementation CimbarDecoderBridgeSnapshot

- (instancetype)initWithPhase:(CimbarDecoderBridgePhase)phase
              recognizedFrame:(BOOL)recognizedFrame
                 needsSharpen:(BOOL)needsSharpen
               extractedBytes:(NSInteger)extractedBytes
              completedFileID:(int64_t)completedFileID {
    self = [super init];
    if (self) {
        _phase = phase;
        _recognizedFrame = recognizedFrame;
        _needsSharpen = needsSharpen;
        _extractedBytes = extractedBytes;
        _completedFileID = completedFileID;
    }
    return self;
}

@end

namespace {

CimbarDecoderBridgePhase bridge_phase_for_progress(int phase) {
    switch (phase) {
        case 0:
            return CimbarDecoderBridgePhaseIdle;
        case 1:
            return CimbarDecoderBridgePhaseSearching;
        case 2:
            return CimbarDecoderBridgePhaseDetecting;
        case 3:
            return CimbarDecoderBridgePhaseDecoding;
        case 4:
            return CimbarDecoderBridgePhaseReconstructing;
        case 5:
            return CimbarDecoderBridgePhaseCompleted;
        case 6:
            return CimbarDecoderBridgePhaseError;
        default:
            return CimbarDecoderBridgePhaseError;
    }
}

} // namespace

@interface CimbarDecoderBridge () {
    cimbar_ios_recv_handle *_handle;
}
@end

@implementation CimbarDecoderBridge

- (instancetype)init {
    self = [super init];
    if (self) {
        _handle = cimbar_ios_recv_create();
    }
    return self;
}

- (void)dealloc {
    if (_handle != nullptr) {
        cimbar_ios_recv_destroy(_handle);
        _handle = nullptr;
    }
}

- (void)reset {
    if (_handle != nullptr) {
        cimbar_ios_recv_reset(_handle);
    }
}

- (void)configureMode:(NSInteger)mode {
    if (_handle != nullptr) {
        cimbar_ios_recv_configure_mode(_handle, (int)mode);
    }
}

- (nullable CimbarDecoderBridgeSnapshot *)processSampleBuffer:(CMSampleBufferRef)sampleBuffer {
    if (_handle == nullptr || sampleBuffer == nullptr) {
        return nil;
    }

    CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (pixelBuffer == nullptr) {
        return nil;
    }

    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

    unsigned char *baseAddress = static_cast<unsigned char *>(CVPixelBufferGetBaseAddress(pixelBuffer));
    size_t width = CVPixelBufferGetWidth(pixelBuffer);
    size_t height = CVPixelBufferGetHeight(pixelBuffer);
    size_t stride = CVPixelBufferGetBytesPerRow(pixelBuffer);

    CimbarDecoderBridgeSnapshot *snapshot = nil;
    if (baseAddress != nullptr && width > 0 && height > 0) {
        cimbar_ios_recv_progress progress = {0};
        int result = cimbar_ios_recv_process_frame(
            _handle,
            baseAddress,
            static_cast<unsigned>(width),
            static_cast<unsigned>(height),
            4,
            static_cast<unsigned>(stride),
            &progress
        );

        if (result == 0) {
            snapshot = [[CimbarDecoderBridgeSnapshot alloc] initWithPhase:bridge_phase_for_progress(progress.phase)
                                                          recognizedFrame:(progress.recognized_frame != 0)
                                                             needsSharpen:(progress.needs_sharpen != 0)
                                                           extractedBytes:progress.extracted_bytes
                                                          completedFileID:progress.completed_file_id];
        }
    }

    CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    return snapshot;
}

@end
