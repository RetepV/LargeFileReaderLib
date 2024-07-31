//
//  LargeFileReaderLib.h
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 31/07/2024.
//

#import <Foundation/Foundation.h>

//! Project version number for `LargeFileReaderLib`.
FOUNDATION_EXPORT double LargeFileReaderLibVersionNumber;

//! Project version string for `LargeFileReaderLib`.
FOUNDATION_EXPORT const unsigned char LargeFileReaderLibVersionString[];

@interface LargeFileReaderLib : NSObject

@property (nonatomic, readonly) NSInteger cacheDefaultBlockSize;
@property (nonatomic, readonly) NSInteger cacheDefaultMaxSize;

@property (nonatomic, readonly) NSInteger cacheBlockSize;
@property (nonatomic, readonly) NSInteger cacheMaxSize;
@property (nonatomic, readonly) NSInteger cacheActualSize;

@property (nonatomic, readonly) BOOL isOpen;
@property (nonatomic, readonly) BOOL isEof;
@property (nonatomic, readonly) BOOL isFail;
@property (nonatomic, readonly) BOOL isBad;

- (BOOL)open:(NSString *)fullFilePath;
- (BOOL)open:(NSString *)fullFilePath cacheMaxSize:(NSInteger)cacheMaxSize cacheBlockSize:(NSInteger)cacheBlockSize;
- (void)close;

- (NSInteger)lseek:(NSInteger)offsetInBytes whence:(NSInteger)whence;
- (NSInteger)read:(unsigned char *)buffer bytes:(NSInteger)numberOfBytes;

@end
