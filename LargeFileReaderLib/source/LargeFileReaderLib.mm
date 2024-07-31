//
//  LargeFileReaderLib.mm
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 31/07/2024.
//

#import <Foundation/Foundation.h>
#import "LargeFileReaderLib.h"
#import "LargeFileReaderLibCore.hpp"

@interface LargeFileReaderLib()

@property (nonatomic, assign) LargeFileReaderLibCore *LargeFileReaderLib;

@end

@implementation LargeFileReaderLib

- (instancetype)init
{
    self = [super init];
    
    if (self)
    {
        self.LargeFileReaderLib = new LargeFileReaderLibCore;
    }
    
    return self;
}

- (void)dealloc
{
    self.LargeFileReaderLib = nil;
}

- (NSInteger)cacheDefaultBlockSize
{
    return self.LargeFileReaderLib->cacheDefaultBlockSize;
}

- (NSInteger)cacheDefaultMaxSize
{
    return self.LargeFileReaderLib->cacheDefaultMaxSize;
}

- (NSInteger)cacheBlockSize
{
    return self.LargeFileReaderLib->cacheBlockSize;
}

- (NSInteger)cacheMaxSize
{
    return self.LargeFileReaderLib->cacheMaxSize;
}

- (NSInteger)cacheActualSize
{
    return self.LargeFileReaderLib->cacheActualSize;
}

- (BOOL)isOpen
{
    return self.LargeFileReaderLib->isOpen;
}

- (BOOL)isEof
{
    return self.LargeFileReaderLib->isEof;
}

- (BOOL)isFail
{
    return self.LargeFileReaderLib->isFail;
}

- (BOOL)isBad
{
    return self.LargeFileReaderLib->isBad;
}

- (BOOL)open:(NSString *)fullFilePath;
{
    return self.LargeFileReaderLib->open([fullFilePath cStringUsingEncoding:NSASCIIStringEncoding]);
}

- (BOOL)open:(NSString *)fullFilePath cacheMaxSize:(NSInteger)cacheMaxSize cacheBlockSize:(NSInteger)cacheBlockSize
{
    return self.LargeFileReaderLib->open([fullFilePath cStringUsingEncoding:NSASCIIStringEncoding], cacheMaxSize, cacheBlockSize);
}

- (void)close
{
    self.LargeFileReaderLib->close();
}

- (NSInteger)lseek:(NSInteger)offsetInBytes whence:(NSInteger)whence
{
    return self.LargeFileReaderLib->lseek(offsetInBytes, (int)whence);
}

- (NSInteger)read:(unsigned char *)buffer bytes:(NSInteger)numberOfBytes
{
    return self.LargeFileReaderLib->read(buffer, numberOfBytes);
}

@end
