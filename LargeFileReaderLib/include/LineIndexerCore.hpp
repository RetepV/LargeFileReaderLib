//
//  LineIndexerCore.hpp
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 02/08/2024.
//

#ifndef LineIndexerCore_hpp
#define LineIndexerCore_hpp

#include <swift/bridging>
#include <stdint.h>
#include <sys/types.h>

#include "LargeFileReaderCore.hpp"
#include "FixedBlockAllocatedArray.hpp"

class LineIndexerCore {
public:
    
    // MARK: - Public definitions
    
    struct LineIndexEntry {
        off_t   offset;
        size_t  length;
    };
    
    // MARK: - Public consts
    
    // MARK: - Public properties
    
    // TODO: Make it an array or string, so we can pass \x0D\x0A.
    uint8_t lineDelimiter = '\n';
    
    int numberOfLines = -1;
    FixedBlockAllocatedArray<LineIndexerCore::LineIndexEntry> lineIndex;
    
    // MARK: - Public methods
    
    LineIndexerCore();
    ~LineIndexerCore();
    
    int indexLinesForFileReader(LargeFileReaderCore* reader);

private:
    
    // MARK: - Private definitions
    
    // MARK: - Private consts
    
    // MARK: - Private properties
    
    // MARK: - Private methods

};

#endif /* LineCutterCore_hpp */
