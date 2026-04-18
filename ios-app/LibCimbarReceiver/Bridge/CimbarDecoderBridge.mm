#import "CimbarDecoderBridge.h"

#import "cimbar_ios_recv_c.h"

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

@end
