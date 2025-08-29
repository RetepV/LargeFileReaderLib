//
//  LineIndexerCore.cpp
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 02/08/2024.
//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cassert>

#include "LineIndexerCore.hpp"
#include "FixedBlockAllocatedArray.hpp"

LineIndexerCore::LineIndexerCore()
{
    
}

LineIndexerCore::~LineIndexerCore()
{
    
}

int LineIndexerCore::indexLinesForFileReader(LargeFileReaderCore* reader)
{
    assert(reader != NULL);

    if (!reader->isOpen)
    {
        return -1;
    }
    
    // Allocate enough space for 2 full buffers
    
    uint8_t buffer[reader->cacheBlockSize];
    size_t totalNumberOfBytesInBuffer;

    // Start from file position 0.
    reader->lseek(0, SEEK_SET);

    // Initially read two full buffers
    size_t lastNumberOfBytesRead;

    lastNumberOfBytesRead = reader->read(&buffer[0], reader->cacheBlockSize);
    totalNumberOfBytesInBuffer = lastNumberOfBytesRead;
    lastNumberOfBytesRead = reader->read(&buffer[reader->cacheBlockSize], reader->cacheBlockSize);
    totalNumberOfBytesInBuffer += lastNumberOfBytesRead;
    
    bool hasDataToProcess = totalNumberOfBytesInBuffer > 0;

    uint64_t currentLineNumber = 0;
    off_t currentStartOfLineInBuffer = 0;
    off_t currentSearchIndexInbuffer = 0;
    
    while (hasDataToProcess)
    {
        // Scan the buffer for our end-of-line marker, or until we have reached a maximum line length.
        bool lineEndFound = false;

        while (!lineEndFound &&
               (currentSearchIndexInbuffer < totalNumberOfBytesInBuffer))
        {
            if ((buffer[currentSearchIndexInbuffer] == '\n') ||                         // TODO: Configurable end of line.
                (currentSearchIndexInbuffer - currentStartOfLineInBuffer) == 2048)      // TODO: Configurable max line length.
            {
                lineEndFound = true;
            }
            else
            {
                currentSearchIndexInbuffer++;
            }
        }
        
        if (lineEndFound)
        {
            lineIndex[currentLineNumber].offset = currentStartOfLineInBuffer;
            lineIndex[currentLineNumber].length = currentSearchIndexInbuffer - currentStartOfLineInBuffer;
            
            currentSearchIndexInbuffer++;
            currentStartOfLineInBuffer = currentSearchIndexInbuffer;
            
            lineEndFound = false;
        }
        else
        {
            // Did not find a line end
            
            // NOTE: THIS IS DEBUG STUFF TO SILENCE THE COMPILER FOR NOW!
            hasDataToProcess = true;
        }
        
    }
    
    return 0;
}
