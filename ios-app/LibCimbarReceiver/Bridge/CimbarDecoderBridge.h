#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, CimbarDecoderBridgePhase) {
    CimbarDecoderBridgePhaseIdle = 0,
    CimbarDecoderBridgePhaseSearching = 1,
    CimbarDecoderBridgePhaseDetecting = 2,
    CimbarDecoderBridgePhaseDecoding = 3,
    CimbarDecoderBridgePhaseReconstructing = 4,
    CimbarDecoderBridgePhaseCompleted = 5,
    CimbarDecoderBridgePhaseError = 6,
};

@interface CimbarDecoderBridgeSnapshot : NSObject

@property (nonatomic, readonly) CimbarDecoderBridgePhase phase;
@property (nonatomic, readonly) BOOL recognizedFrame;
@property (nonatomic, readonly) BOOL needsSharpen;
@property (nonatomic, readonly) NSInteger extractedBytes;
@property (nonatomic, readonly) int64_t completedFileID;
@property (nonatomic, readonly) NSInteger scannedChunks;
@property (nonatomic, readonly) NSInteger totalChunks;
@property (nonatomic, readonly) NSString *statusMessage;

- (instancetype)initWithPhase:(CimbarDecoderBridgePhase)phase
              recognizedFrame:(BOOL)recognizedFrame
                 needsSharpen:(BOOL)needsSharpen
               extractedBytes:(NSInteger)extractedBytes
              completedFileID:(int64_t)completedFileID
                scannedChunks:(NSInteger)scannedChunks
                  totalChunks:(NSInteger)totalChunks
                statusMessage:(NSString *)statusMessage NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

@interface CimbarDecoderBridgeCompletedFile : NSObject

@property (nonatomic, readonly) NSString *filename;
@property (nonatomic, readonly) NSData *data;

- (instancetype)initWithFilename:(NSString *)filename
                            data:(NSData *)data NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

@interface CimbarDecoderBridge : NSObject

- (instancetype)init;
- (void)reset;
- (void)configureMode:(NSInteger)mode;
- (nullable CimbarDecoderBridgeSnapshot *)processSampleBuffer:(CMSampleBufferRef)sampleBuffer;
- (nullable CimbarDecoderBridgeCompletedFile *)takeCompletedFile;

@end

NS_ASSUME_NONNULL_END
