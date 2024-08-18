//
//  LargeFileReader.h
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 31/07/2024.
//

#ifndef LargeFileReader_h
#define LargeFileReader_h

#import <Foundation/Foundation.h>

//! Project version number for `LargeFileReader`.
FOUNDATION_EXPORT double LargeFileReaderVersionNumber;

//! Project version string for `LargeFileReader`.
FOUNDATION_EXPORT const unsigned char LargeFileReaderVersionString[];

@interface LargeFileReader : NSObject

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

#endif /* LargeFileReader_h */
