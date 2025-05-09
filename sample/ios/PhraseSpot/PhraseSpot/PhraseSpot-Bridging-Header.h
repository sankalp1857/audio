//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#import <snsr.h>

// Bridging doesn't handle variadic C functions,
// wrap the most common use of snsrStreamFromAudioDevice().
SnsrStream
snsrStreamFromDefaultAudioDevice(void) {
    return snsrStreamFromAudioDevice(SNSR_ST_AF_DEFAULT);
}
