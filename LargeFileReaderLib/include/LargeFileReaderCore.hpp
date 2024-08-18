//
//  LargeFileReaderCore.hpp
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 31/07/2024.
//

#ifndef LargeFileReaderCore_h
#define LargeFileReaderCore_h

/* The classes below are exported */
#pragma GCC visibility push(default)

#include <swift/bridging>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

class LargeFileReaderCore
{
public:
    
    // MARK: - Public consts
    
    const int cacheDefaultBlockSize = 65536;
    const int cacheDefaultMaxSize = 2097152;
    
    // MARK: - Public properties
    
    // Size of cache block.
    size_t cacheBlockSize;
    
    // The cache will never be larger than cacheMaxSize, but could be smaller.
    size_t cacheMaxSize;
    // This is the actual size of the cache.
    size_t cacheActualSize;
    
    bool isOpen;
    bool isEof;
    bool isFail;
    bool isBad;
    
    // MARK: - Public methods
    
    LargeFileReaderCore();
    ~LargeFileReaderCore();
    
    // Sets up the index, creates the cache, opens the file for reading.
    //
    // Returns true if the file was opened successfully, else false.
    //
    // cacheMaxSize is the maximum desired size for the cache, in bytes. cacheBlockSize is the desired
    // size of blocks of file data to cache.
    //
    // Note: If cacheMaxSize is not an exact multiple of cacheBlockSize, the class will still honor the
    //       desired cache max size, and will waste some memory.
    // Note: If the file is smaller than the cacheMaxSize, the cache will be made the smallest size of
    //       multiples of cacheBlockSize, that will fit the complete file.
    //
    // Note: if cacheMaxSize and/or cacheBlockSize are not given, or are passed as <=0, then the following
    //       defaults will be used: cacheMaxSize = 2097152, cacheBlockSize = 65536.
    //
    bool open(std::string fullFilePath);
    bool open(std::string fullFilePath, size_t cacheMaxSize, size_t cacheBlockSize);
    
    // Close the file. Deletes the index and the cache, and closes the file.
    void close();
    
    // Seek to data in file. This does not actually do a seek, but sets
    // currentOffset to the passed offset. currentOffset is then used to
    // check in the index if a block is available in the cache,
    off_t lseek(off_t offsetInBytes, int whence);
    
    // Read the file's data. This will return numberOfBytes bytes into the
    // buffer 'buffer'. The data will come from the cache if it is there. If
    // the data is not in the cache (fault), it will be physically read into
    // the cache and then returned.
    // - This call is synchronous and will block until the data is cached.
    // - Returns the number of bytes actually 'read'.
    size_t read(unsigned char* buffer, size_t numberOfBytes);
    
private:
    
    // MARK: - Private definitions
    
    struct FileCacheIndexEntry
    {
        // If true, then we do not have the corresponding data in the cache.
        bool isFault = true;
        // If this block is not faulted, then this points to the data in the cache.
        uint64_t offsetInFileBuffer = -1;
        // Point to the index of the entry that was previously used (i.e. the previous MRU).
        int64_t previousUsed = -1;
        // Point to the index of the entry that was next used (i.e. the previous MRU).
        int64_t nextUsed = -1;
    };
    
    // MARK: - Private properties
    
    // Path of the file that is open.
    std::string filePath;
    
    // Maximum number of blocks that we can cache.
    int64_t maxNumberOfCachedFileDataBlocks;
    // Number of indexes in fileCacheIndex
    int64_t totalNumberOfFileCacheIndexEntries;

    // Number of blocks that we have cached currently.
    int64_t currentNumberOfCachedFileDataBlocks;

    // File status of the open file.
    struct stat fileStatus;
    // File descriptor of the open file.
    int fileDescriptor;
    
    // Index of the file buffers. There are as many entries as fits the file size.
    // However, there might be fewer blocks in the file buffer, and with the index,
    // we can read blocks and remove least recently used blocks when necessary.
    FileCacheIndexEntry* fileCacheIndex;
    
    // Index to the FileBufferBlockEntry that is the most recently used.
    int64_t mostRecentlyUsedIndex = -1;
    // Index to the FileBufferBlockEntry that is the least recently used.
    int64_t leastRecentlyUsedIndex = -1;

    // Cache of file data blocks.
    unsigned char* fileDataBlocks;

    // 'Virtual' current offset pointer into the file. Note that this is not the
    // actual read offset of the file's file pointer. It points to the next data
    // that the 'read' memberfunction will return data from when called.
    off_t currentFileOffset;
    
    // MARK: - Private methods

    size_t fetchDataBlockForIndex(int64_t index);
};

#pragma GCC visibility pop
#endif /* LargeFileReaderCore_h */
