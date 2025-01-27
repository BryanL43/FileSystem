/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: freeSpace.c
*
* Description:: Free space management using File Allocation Table.
*
**************************************************************/

#include "freeSpace.h"
#include "fsDesign.h"
#include "fsLow.h"

/**
 * Initializes the FAT free space management system.
 * 
 * @param numberOfBlocks The total number of blocks in the file system.
 * @param blockSize The size of each block in the file system.
 * @return Returns 0 on success, -1 on failure.
*/
int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize) {
    int bytesNeeded = numberOfBlocks * sizeof(int);
    int blocksNeeded = (bytesNeeded + blockSize - 1) / blockSize;

    // Sentinel value for VCB
    FAT[0] = 0xFFFFFFFD;

    // Link free space blocks
    for (int i = 1; i < numberOfBlocks; i++) {
        FAT[i] = i+1;
    }

    // Sentinel value for FAT storage
    FAT[blocksNeeded] = 0xFFFFFFFD;

    // Sentinel value for free space storage 
    FAT[numberOfBlocks] = 0xFFFFFFFD;

    int blocksWritten = LBAwrite(FAT, blocksNeeded, 1);

    // Assign VCB initial values
    vcb->totalFreeSpace = numberOfBlocks - blocksNeeded - 1; // -1 because of VCB
    vcb->freeSpaceSize = blocksNeeded;
    vcb->freeSpaceLocation = 1; // FAT table starts at block 1, right after VCB block
    vcb->firstFreeBlock = blocksNeeded + 1;
    
    return (blocksWritten == -1) ? -1 : 0;
}

/**
 * Retrieves a specified number of free blocks.
 * 
 * @param numberOfBlocks The number of contiguous blocks requested.
 * @param last_block_in_file The last block in the file being extended. 0 if new file
 * @return Returns the index of the first free block, or -1 if unsuccessful.
*/
int getFreeBlocks(uint64_t numberOfBlocks, uint64_t last_block_in_file) {
    if (numberOfBlocks < 1) {
        return -1;
    }

    if (numberOfBlocks > vcb->totalFreeSpace) {
        return -1;
    }

    int head = vcb->firstFreeBlock;
    int currentBlock = vcb->firstFreeBlock;
    int nextBlock = FAT[currentBlock];

    //modify the tail of the exsiting file to point at new blocks
    if (last_block_in_file > 0) {
        FAT[last_block_in_file] = head;
    }

    vcb->totalFreeSpace--;

    // Traverse and acquire the free space requested
    for (int i = 1; i < numberOfBlocks; i++) {
        currentBlock = nextBlock;
        nextBlock = FAT[currentBlock];
        vcb->totalFreeSpace--;
    }

    // Sentinel value to indicate end of requested free space
    FAT[currentBlock] = 0xFFFFFFFD;
    vcb->firstFreeBlock = nextBlock;

    // Update the FAT table in Volume
    LBAwrite(FAT, vcb->freeSpaceSize, vcb->freeSpaceLocation);

    return head;
}

/**
 * Write block to volume.
 * 
 * @param buffer Pointer to the buffer with data to be written.
 * @param numberOfBlocks The number of blocks to write.
 * @param location The starting location of where the blocks are read from.
 * @return Returns the number of blocks successfully written, or -1 if an error occurs.
*/
int writeBlock(void* buffer, uint64_t numberOfBlocks, int location) {
    int blockSize = vcb->blockSize;
    int blocksWritten = 0;

    // Write each block from buffer to volume
    for (int i = 0; i < numberOfBlocks; i++) {
        if (LBAwrite(buffer + blockSize * i, 1, location) == -1) {
            return -1;
        }
        location = FAT[location];
        blocksWritten++;
    }

    return blocksWritten;
}

/**
 * Loads the specified blocks into memory.
 * 
 * @param buffer the buffer to load the data into.
 * @param numberOfBlocks the specified number of blocks.
 * @param location the starting block to load from.
 * @return blocks read or -1 on failure.
*/
int readBlock(void* buffer, uint64_t numberOfBlocks, int location) {
    int blockSize = vcb->blockSize;
    int blocksRead = 0;

    for (int i = 0; i < numberOfBlocks; i++) {
        if (LBAread(buffer + blockSize * i, 1, location) == -1) {
            return -1;
        }
        location = FAT[location];
        blocksRead++;
    }
    return blocksRead;
}

/**
 * Seeks the location of the specified block.
 * 
 * @param numberOfBlocks the specified number of blocks.
 * @param location the starting block to search from.
 * @return the location of the found block or -1 on failure.
*/
int seekBlock(uint64_t numberOfBlocks, int location) {
    if (location < 0) {
        return -1;
    }
    for (int i = 0; i < numberOfBlocks; i++) {
        location = FAT[location];
    }
    return location;
}