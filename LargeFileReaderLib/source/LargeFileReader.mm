//
//  LargeFileReader.mm
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 31/07/2024.
//

#import <Foundation/Foundation.h>
#import "LargeFileReader.h"
#import "LargeFileReaderCore.hpp"

@interface LargeFileReader()

@property (nonatomic, assign) LargeFileReaderCore *largeFileReaderCore;

@end

@implementation LargeFileReader

- (instancetype)init
{
    self = [super init];
    
    if (self)
    {
        _largeFileReaderCore = new LargeFileReaderCore;
    }
    
    return self;
}

- (void)dealloc
{
    _largeFileReaderCore = nil;
}

- (NSInteger)cacheDefaultBlockSize
{
    return self.largeFileReaderCore->cacheDefaultBlockSize;
}

- (NSInteger)cacheDefaultMaxSize
{
    return self.largeFileReaderCore->cacheDefaultMaxSize;
}

- (NSInteger)cacheBlockSize
{
    return self.largeFileReaderCore->cacheBlockSize;
}

- (NSInteger)cacheMaxSize
{
    return self.largeFileReaderCore->cacheMaxSize;
}

- (NSInteger)cacheActualSize
{
    return self.largeFileReaderCore->cacheActualSize;
}

- (BOOL)isOpen
{
    return self.largeFileReaderCore->isOpen;
}

- (BOOL)isEof
{
    return self.largeFileReaderCore->isEof;
}

- (BOOL)isFail
{
    return self.largeFileReaderCore->isFail;
}

- (BOOL)isBad
{
    return self.largeFileReaderCore->isBad;
}

- (BOOL)open:(NSString *)fullFilePath;
{
    return self.largeFileReaderCore->open([fullFilePath cStringUsingEncoding:NSASCIIStringEncoding]);
}

- (BOOL)open:(NSString *)fullFilePath cacheMaxSize:(NSInteger)cacheMaxSize cacheBlockSize:(NSInteger)cacheBlockSize
{
    return self.largeFileReaderCore->open([fullFilePath cStringUsingEncoding:NSASCIIStringEncoding], cacheMaxSize, cacheBlockSize);
}

- (void)close
{
    self.largeFileReaderCore->close();
}

- (NSInteger)lseek:(NSInteger)offsetInBytes whence:(NSInteger)whence
{
    return self.largeFileReaderCore->lseek(offsetInBytes, (int)whence);
}

- (NSInteger)read:(unsigned char *)buffer bytes:(NSInteger)numberOfBytes
{
    return self.largeFileReaderCore->read(buffer, numberOfBytes);
}

@end
