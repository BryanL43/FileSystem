#include "freeSpace.h"
#include "fsDesign.h"
#include "fsLow.h"

int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize) {
    int bytesNeeded = numberOfBlocks * sizeof(int);
    int blocksNeeded = (bytesNeeded + blockSize - 1) / blockSize;

    // Sentinel value for VCB
    FAT[0] = 0xFFFFFFFD;

    // Free space links
    for (int i = 1; i < numberOfBlocks; i++) {
        FAT[i] = i+1;
    }

    // Sentinel value for FAT storage
    FAT[blocksNeeded] = 0xFFFFFFFD;

    // Sentinel value for free space storage 
    FAT[numberOfBlocks] = 0xFFFFFFFD;

    int blocksWritten = LBAwrite(FAT, blocksNeeded, 1);

    // Initialize vcb values
    vcb->totalFreeSpace = numberOfBlocks - blocksNeeded -1; // -1 because of VCB

    // FAT table starts at block 1, right after VCB block
    vcb->freeSpaceLocation = 1;
    
    vcb->firstFreeBlock = blocksNeeded + 1;
    
    return (blocksWritten == -1) ? -1 : 0; //-1 fail; 0 success
}

//numberOfBlocks = number of blocks needed [temp will fix method heading later]
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

    // Decrement total free space for the head block
    vcb->totalFreeSpace--;

    // Traverse and acquire the free space requested
    for (int i = 1; i < numberOfBlocks; i++) {
        currentBlock = nextBlock;
        nextBlock = FAT[currentBlock];
        vcb->totalFreeSpace--;
    }

    // Break link to indicate end of requested free space
    FAT[currentBlock] = 0xFFFFFFFD;
    vcb->firstFreeBlock = nextBlock;

    return head;
}

//Write block to volume
int writeBlock(uint64_t numberOfBlocks, void* buffer, int location) {
    int blockSize = vcb->blockSize;
    int blocksWritten = 0;

    for (int i = 0; i < numberOfBlocks; i++) {
        if (LBAwrite(buffer + (blockSize * blocksWritten), 1, location) == -1) {
            return -1;
        }
        location = FAT[location];
        blocksWritten++;
    }

    return blocksWritten;
}