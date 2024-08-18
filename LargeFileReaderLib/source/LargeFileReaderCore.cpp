//
//  LargeFileReaderCore.cpp
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 31/07/2024.
//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cassert>

#include "LargeFileReaderCore.hpp"

LargeFileReaderCore::LargeFileReaderCore()
{
    isOpen = false;
    isEof = false;
    isFail = false;
    isBad = false;
}

LargeFileReaderCore::~LargeFileReaderCore()
{
    
}

bool LargeFileReaderCore::open(std::string fullFilePath)
{
    return open(fullFilePath, 0, 0);
}

bool LargeFileReaderCore::open(std::string fullFilePath, size_t cacheMaxSize, size_t cacheBlockSize)
{
    if (isOpen)
    {
        return false;
    }
    
    filePath = fullFilePath;
    
    if (::stat(filePath.c_str(), &fileStatus) != 0)
    {
        return false;
    }
    
    if (cacheMaxSize <= 0)
    {
        cacheMaxSize = cacheDefaultMaxSize;
    }
    if (cacheBlockSize <= 0)
    {
        cacheBlockSize = cacheDefaultBlockSize;
    }

    if (cacheBlockSize > cacheMaxSize)
    {
        throw std::out_of_range("Cache block size must be less than cache max size");
    }

    this->cacheMaxSize = cacheMaxSize;
    this->cacheBlockSize = cacheBlockSize;

    // Calculate size of cache, number of blocks, etc.
    
    // If file size is less than cacheMaxSize, then it makes no sense to waste memory,
    // make the cache size less in that case.
    if (fileStatus.st_size < cacheMaxSize)
    {
        // Actual size will be a multiple of cacheBlockSize, so that the whole file can fit.
        // Probably wasting a little memory due to aliasing, but who cares in this case.
        cacheActualSize = (size_t)ceil((double)fileStatus.st_size / (double)cacheBlockSize) * cacheBlockSize;
    }
    else
    {
        // Actual size will be the maximum requested size. Aliasing (and small memory waste)
        // will happen if user did not request a multiple of cacheBlockSize.
        cacheActualSize = cacheMaxSize;
    }
    // Maximum number of file data blocks that we can actually store.
    maxNumberOfCachedFileDataBlocks = (int64_t)floor((double)cacheActualSize / (double)cacheBlockSize);
    // Number of datablocks necessary to fit the whole file (the number of indexes in the file cache).
    totalNumberOfFileCacheIndexEntries = (size_t)ceil((double)fileStatus.st_size / (double)cacheBlockSize);
    
    // Currently, we have cached nothing.
    currentNumberOfCachedFileDataBlocks = 0;

    // Allocate memory.
    
    fileDataBlocks = new unsigned char[cacheActualSize];
    fileCacheIndex = new FileCacheIndexEntry[totalNumberOfFileCacheIndexEntries];
    
    mostRecentlyUsedIndex = -1;
    leastRecentlyUsedIndex = -1;
    
    // Open the file.
    
    if ((fileDescriptor = ::open(filePath.c_str(), O_RDONLY)) < 0)
    {
        delete [] fileDataBlocks;
        delete [] fileCacheIndex;
        return false;
    }
    
    currentFileOffset = 0;
    
    isOpen = true;
    isEof = false;
    isFail = false;
    isBad = false;

    return true;
}

void LargeFileReaderCore::close()
{
    if (!isOpen)
    {
        return;
    }
    
    ::close(fileDescriptor);
    
    delete [] fileDataBlocks;
    fileDataBlocks = NULL;
    delete [] fileCacheIndex;
    fileCacheIndex = NULL;
    
    isOpen = false;
    isEof = false;
    isFail = false;
    isBad = false;
}

off_t LargeFileReaderCore::lseek(off_t offsetInBytes, int whence)
{
    if (!isOpen)
    {
        return -1;
    }
    
    isEof = false;
    isFail = false;
    isBad = false;
    
    off_t newFileOffset;
    
    switch (whence)
    {
        case SEEK_SET:
            newFileOffset = offsetInBytes;
            break;
        case SEEK_CUR:
            newFileOffset = currentFileOffset + offsetInBytes;
            break;
        case SEEK_END:
            newFileOffset = fileStatus.st_size + offsetInBytes;
            break;
        default:
            return -1;
    }
    
    // If the new file offset is beyond the last byte of the file, set the file pointer to the end
    // and return EOF. We do not support reading from beyond the end of the file.
    // NOTE: This is different behaviour than the normal stdlib 'lseek' and 'read' functions. They
    //       do allow setting the file offset beyond the end of the file. They also allow 'reading'
    //       from beyond the end of the file. But do we need that?
    
    if (newFileOffset >= fileStatus.st_size)
    {
        isEof = true;
        currentFileOffset = fileStatus.st_size;
    }
    else
    {
        currentFileOffset = newFileOffset;
    }

    return currentFileOffset;
}

size_t LargeFileReaderCore::read(unsigned char* buffer, size_t numberOfBytes)
{
    if (!isOpen)
    {
        return -1;
    }
    
    isEof = false;
    isFail = false;
    isBad = false;
    
    // Check if we are going to try to read beyond the end of the file. If so, adjust numberOfBytes
    // to the amount of data left in the file, from currentFileOffset. If the numberOfBytes becomes
    // 0 or even negative, we can break out early. If the numberOfBytes becomes less than the user
    // passed, we will still go through the while loop exactly as many times as needed, without
    // extra checks.
    
    if ((currentFileOffset + numberOfBytes) > fileStatus.st_size)
    {
        numberOfBytes = fileStatus.st_size - currentFileOffset;
        
        // We were trying to read beyond the end of file, and after reading, the currentFileOffset
        // will point beyond the last data byte in the file, so we can set the EOF flag.
        isEof = true;
        
        if (numberOfBytes <= 0)
        {
            return 0;
        }
    }
    
    size_t totalBytesRead = 0;

    while (totalBytesRead < numberOfBytes)
    {
        // Calculate the data block that should contain (part of) our data. Don't forget, this
        // is the entry in the index that should point to the data that we want.
        uint64_t dataBlockIndex = floor(currentFileOffset / cacheBlockSize);
        assert(dataBlockIndex < totalNumberOfFileCacheIndexEntries);
        // If the block is faulted, we don't have its data in the cache, so fetch it.
        if (fileCacheIndex[dataBlockIndex].isFault)
        {
            // We must fetch data for this index entry.
            size_t bytesRead = fetchDataBlockForIndex(dataBlockIndex);
            if (bytesRead < 0)
            {
                // Abort, we failed to read/cache any data.
                isFail = true;
                return -1;
            }
        }
        // If the block is still faulted here, then we couldn't read the data from the file.
        if (fileCacheIndex[dataBlockIndex].isFault)
        {
            // Abort
            isFail = true;
            return -2;
        }
        
        // We want the bytes from 'currentFileOffset' to either the 'numberOfBytes' or the end of the block,
        // depending on if the block contains enough data.
        
        // Calculate offset inside the data block where we think our data is.
        uint64_t offsetInDataBlock = currentFileOffset - (dataBlockIndex * cacheBlockSize);
        
        // Calculate the length of the data we want to copy. Start by assuming that we will need everything
        // from the offset to the end of the block, and truncate accordingly for numberOfBytes and
        // fileStatus.st_size.
        
        uint64_t lengthInDataBlock = cacheBlockSize - offsetInDataBlock;
        if (lengthInDataBlock > (numberOfBytes - totalBytesRead))
        {
            // All of our (possibly remaining) data is in this data block. Read only the necessary.
            lengthInDataBlock = (numberOfBytes - totalBytesRead);
        }
        // Maybe we are about to read beyond the end of the file. Truncate the length even more, if so.
        if ((currentFileOffset + lengthInDataBlock) > fileStatus.st_size)
        {
            lengthInDataBlock = fileStatus.st_size - currentFileOffset;
            // After reading, we will be at EOF.
            isEof = true;
            // If we are reading far beyond the file (i.e. even currentFileOffset is beyond the file's end),
            // we should read 0 bytes.
            if (lengthInDataBlock < 0)
            {
                lengthInDataBlock = 0;
            }
        }
        // If we got here and are now trying to read 0 bytes, we're trying to read beyond the end of the
        // file's data. We can break out early. Note that we may have copied data to the buffer, and want
        // to keep it. So return the totalBytesRead.
        if (lengthInDataBlock == 0)
        {
            assert(isEof == true);  // If it is false here, there must have been a use-case that I forgot.
            return totalBytesRead;
        }

        // Point to the data.
        unsigned char* cacheBlockPointer = &fileDataBlocks[fileCacheIndex[dataBlockIndex].offsetInFileBuffer];
        // Copy the data.
        memcpy(&buffer[totalBytesRead], &cacheBlockPointer[offsetInDataBlock], lengthInDataBlock);
        // Adjust current file offset for the next read.
        currentFileOffset += lengthInDataBlock;
        // Update total bytes read to see if we are finished.
        totalBytesRead += lengthInDataBlock;
    }
    
    // We might be at EOF now.
    if (currentFileOffset >= fileStatus.st_size)
    {
        isEof = true;
    }
    
    return totalBytesRead;
}

size_t LargeFileReaderCore::fetchDataBlockForIndex(int64_t index)
{
    // Fetch data for an index entry.
    
    // Step 1: Find a place in fileDataBlocks that we can use. Either:
    //         1) find an unused buffer block
    //         2) take the LRU index's buffer block, overwrite it for our own use, and become the MRU,
    //            while making the predecessor of the LRU the new LRU, and the previous MRU our PRU.
    
    // We initially start with all blocks empty and fill them linearly from 0 to max. Once we filled
    // all blocks, we start stealing LRU's blocks.
    
    uint64_t cacheBlockOffset;
    
    if (currentNumberOfCachedFileDataBlocks < maxNumberOfCachedFileDataBlocks)
    {
        // The cache is not full yet, and we fill from start to end first.
        cacheBlockOffset = currentNumberOfCachedFileDataBlocks * cacheBlockSize;
        currentNumberOfCachedFileDataBlocks++;

        // Store the offset in the index, this will become the next MRU.
        fileCacheIndex[index].isFault = false;
        fileCacheIndex[index].offsetInFileBuffer = cacheBlockOffset;
        // Point previous used of new index to previous MRU.
        fileCacheIndex[index].previousUsed = mostRecentlyUsedIndex;

        // Update the previous MRU to point to us.
        if (mostRecentlyUsedIndex != -1)
        {
            fileCacheIndex[mostRecentlyUsedIndex].nextUsed = index;
        }

        // Update the LRU if we didn't set it before. We set it to us: the first
        // cached block will become both LRU and MRU.
        if (leastRecentlyUsedIndex == -1)
        {
            leastRecentlyUsedIndex = index;
        }
        
        // We are now the new MRU.
        mostRecentlyUsedIndex = index;
    }
    else
    {
        // We're going to reuse the LRU's cache block.
        cacheBlockOffset = fileCacheIndex[leastRecentlyUsedIndex].offsetInFileBuffer;
        
        // Save the current LRU's next used. It will become the new LRU later.
        int64_t nextToBecomeLRU = fileCacheIndex[leastRecentlyUsedIndex].nextUsed;
        if (nextToBecomeLRU != -1)
        {
            fileCacheIndex[nextToBecomeLRU].previousUsed = -1;
        }
        
        // Fault the LRU.
        fileCacheIndex[leastRecentlyUsedIndex].isFault = true;
        fileCacheIndex[leastRecentlyUsedIndex].offsetInFileBuffer = -1;
        fileCacheIndex[leastRecentlyUsedIndex].previousUsed = -1;
        fileCacheIndex[leastRecentlyUsedIndex].nextUsed = -1;
        
        // Update the new index.
        fileCacheIndex[index].isFault = false;
        fileCacheIndex[index].offsetInFileBuffer = cacheBlockOffset;
        fileCacheIndex[index].previousUsed = mostRecentlyUsedIndex;
        fileCacheIndex[index].nextUsed = -1;
        
        if (mostRecentlyUsedIndex != -1)
        {
            fileCacheIndex[mostRecentlyUsedIndex].nextUsed = index;
        }

        // Update LRU.
        leastRecentlyUsedIndex = nextToBecomeLRU;

        // Update MRU.
        mostRecentlyUsedIndex = index;
    }

    unsigned char* cacheBlockPointer = &fileDataBlocks[fileCacheIndex[index].offsetInFileBuffer];
    
    return ::read(fileDescriptor, cacheBlockPointer, cacheBlockSize);
}

