#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CimbarDecoderBridge : NSObject

- (instancetype)init;
- (void)reset;
- (void)configureMode:(NSInteger)mode;

@end

NS_ASSUME_NONNULL_END
