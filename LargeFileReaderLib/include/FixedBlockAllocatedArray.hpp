//
//  FixedBlockAllocatedArray.hpp
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 09/08/2024.
//

#ifndef FixedBlockAllocatedArray_hpp
#define FixedBlockAllocatedArray_hpp

#include <swift/bridging>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include <swift/bridging>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

/* The classes below are exported */
#pragma GCC visibility push(default)

template <class T>
class FixedBlockAllocatedArray {
public:
    
    // MARK: - Public consts
    const uint64_t numberOfEntriesInBlock = 16384;

    // MARK: - Public definitions
    
    struct AllocatedBlockType {
        AllocatedBlockType *prevBlock;
        AllocatedBlockType *nextBlock;
        T *allocatedBlock;
    };

    // MARK: - Public properties
    
    // MARK: - Public methods
    
    FixedBlockAllocatedArray();
    ~FixedBlockAllocatedArray();

    T& operator [] (size_t index);
    const T& operator [] (size_t index) const;

private:

    // MARK: - Private definitions
    
    // MARK: - Private properties
    
    uint64_t totalNumberOfAllocatedBlocks = 0;
    AllocatedBlockType* firstBlock = nullptr;
    AllocatedBlockType* lastBlock = nullptr;

    // MARK: - Private methods
};

#include "FixedBlockAllocatedArray.hxx"

#pragma GCC visibility pop

#endif /* FixedBlockAllocatedArray_hpp */
