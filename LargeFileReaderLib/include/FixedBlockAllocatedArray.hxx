//
//  FixedBlockAllocatedArray.hxx
//  LargeFileReaderLib
//
//  Created by Peter de Vroomen on 09/08/2024.
//

#include "FixedBlockAllocatedArray.hpp"

// Strategy:
//
// We allocate and manage array element storage in blocks of size 'numberOfEntriesInBlock'. We store the
// blocks in a doubly linked list (dll), which we extend as necessary. The array storage is not sparse, so
// if we reference a high index, all storage in-between index 0 and the referenced index will be allocated.
// We use a dll so that we can allocate storage as needed without having to do a realloc (which might start
// making copies if the heap becomes fragmented). As the allocated blocks are all the same size, we will
// minimize heap fragmentation.
//
// Tradeoff:
//
// The tradeoff of using a dll is that we will need to iterate through the list if we want to reference a
// certain block. Therefore, the time to access an array element increases, the higher the given index.
//

template <class T>
FixedBlockAllocatedArray<T>::FixedBlockAllocatedArray()
{
    firstBlock = new AllocatedBlockType;
    assert(firstBlock != nullptr);
    
    firstBlock->allocatedBlock = new T[numberOfEntriesInBlock];
    assert(firstBlock->allocatedBlock != nullptr);
    firstBlock->nextBlock = nullptr;
    
    lastBlock = firstBlock;
    totalNumberOfAllocatedBlocks = 1;
}

template <class T>
FixedBlockAllocatedArray<T>::~FixedBlockAllocatedArray()
{
    AllocatedBlockType* block = lastBlock;
    
    if (block == nullptr)
    {
        return;
    }

    // Clean up from end to beginning.
    while (block->prevBlock != nullptr)
    {
        block = block->prevBlock;
        assert(block->nextBlock != nullptr);
        delete [] block->nextBlock->allocatedBlock;
        delete block->nextBlock;
        block->nextBlock = nullptr;
    }
    
    // Delete first and final entry
    assert(block != nullptr);
    delete [] block->allocatedBlock;
    delete block;
    
    firstBlock = nullptr;
    lastBlock = nullptr;
    
    totalNumberOfAllocatedBlocks = 0;
}

template <class T>
T& FixedBlockAllocatedArray<T>::operator [] (size_t index)
{
    assert(totalNumberOfAllocatedBlocks >= 1);
    
    uint64_t blockNumberToAccess = index / numberOfEntriesInBlock;
    uint64_t indexInBlockToAccess = index % numberOfEntriesInBlock;

    AllocatedBlockType* blockToAccess = firstBlock;

    if (blockNumberToAccess >= totalNumberOfAllocatedBlocks)
    {
        // If the index is beyond what we already have, keep adding blocks at the end until we have enough.
        // That last block is also the block we want, so we don't have to iterate to find it.
        while (blockNumberToAccess >= totalNumberOfAllocatedBlocks)
        {
            AllocatedBlockType* newBlock = new AllocatedBlockType;
            newBlock->allocatedBlock = new T[numberOfEntriesInBlock];
            assert(newBlock->allocatedBlock != nullptr);
            newBlock->nextBlock = nullptr;
            newBlock->prevBlock = lastBlock;
            
            lastBlock->nextBlock = newBlock;
            lastBlock = newBlock;
            
            totalNumberOfAllocatedBlocks++;
        }
        blockToAccess = lastBlock;
    }
    else
    {
        // The list of blocks is basically a doubly linked list, so we need to iterate. We use a DLL so that we can easily
        // extend our storage without having to do realloc (which does copies, and we don't want that). The tradeoff is that
        // we have to iterate through the list to find the block we need.
        for (uint64_t numberOfIterations = 0; (numberOfIterations < blockNumberToAccess) && (blockToAccess != nullptr); numberOfIterations++)
        {
            blockToAccess = blockToAccess->nextBlock;
        }
    }
    // This should not happen. Either we extended the array as necessary, or we didn't extend because we already have the block we need.
    assert(blockToAccess != nullptr);
    
    return blockToAccess->allocatedBlock[indexInBlockToAccess];
}

template <class T>
const T& FixedBlockAllocatedArray<T>::operator [] (size_t index) const
{
    assert(totalNumberOfAllocatedBlocks >= 1);

    uint64_t blockNumberToAccess = index / numberOfEntriesInBlock;
    uint64_t indexInBlockToAccess = index % numberOfEntriesInBlock;

    // Do not allocate in a const method. Just check bounds.
    assert(blockNumberToAccess < totalNumberOfAllocatedBlocks);

    const AllocatedBlockType* blockToAccess = firstBlock;
    for (uint64_t numberOfIterations = 0; (numberOfIterations < blockNumberToAccess) && (blockToAccess != nullptr); numberOfIterations++)
    {
        blockToAccess = blockToAccess->nextBlock;
    }
    assert(blockToAccess != nullptr);

    return blockToAccess->allocatedBlock[indexInBlockToAccess];
}

