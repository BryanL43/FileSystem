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
    vcb->freeSpaceLocation = 1; // FAT table starts at block 1, right after VCB block
    vcb->firstFreeBlock = blocksNeeded + 1;
    
    return (blocksWritten == -1) ? -1 : 0;
}

/**
 * Retrieves a specified number of free blocks.
 * 
 * @param numberOfBlocks The number of contiguous blocks requested.
 * @return Returns the index of the first free block, or -1 if unsuccessful.
*/
int getFreeBlocks(uint64_t numberOfBlocks) {
    if (numberOfBlocks < 1) {
        printf("Invalid free block request amount!\n");
        return -1;
    }

    if (numberOfBlocks > vcb->totalFreeSpace) {
        printf("Not enough free space!\n");
        return -1;
    }

    int head = vcb->firstFreeBlock;
    int currentBlock = vcb->firstFreeBlock;
    int nextBlock = FAT[currentBlock];

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
    LBAwrite(FAT, vcb->totalFreeSpace, vcb->freeSpaceLocation);

    return head;
}

/**
 * Write block to volume.
 * 
 * @param numberOfBlocks The number of blocks to write.
 * @param buffer Pointer to the buffer with data to be written.
 * @param location The starting location of where the blocks are read from.
 * @return Returns the number of blocks successfully written, or -1 if an error occurs.
*/
int writeBlock(uint64_t numberOfBlocks, void* buffer, int location) {
    int blockSize = vcb->blockSize;
    int blocksWritten = 0;

    // Write each block from buffer to volume
    for (int i = 0; i < numberOfBlocks; i++) {
        if (LBAwrite(buffer + (blockSize * blocksWritten), 1, location) == -1) {
            return -1;
        }
        location = FAT[location];
        blocksWritten++;
    }

    return blocksWritten;
}