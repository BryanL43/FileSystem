#include "freeSpace.h"
#include "fsDesign.h"
#include "fsLow.h"

int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize) {
    int bytesNeeded = numberOfBlocks * sizeof(int)
    int blocksNeeded = (bytesNeeded + blockSize - 1) / blockSize;

    FAT[0] = 0xFFFFFFFD;
    for (int i = 1; i < numberOfBlocks; i++) {
        FAT[i] = i+1;
    }

    // Sentinel value for FAT storage
    FAT[blocksNeeded] = 0xFFFFFFFD;

    // Sentinel value for free space storage 
    FAT[numberOfBlocks] = 0xFFFFFFFD;

    int blocksWritten = LBAwrite(FAT, blocksNeeded, 1);
    // TO DO: Update vcb values after initializing vcb
    vcb -> totalFreeSpace = numberOfBlocks - blocksNeeded;
    vcb -> freeSpaceLocation = 1;

    return (blocksWritten == -1) ? -1 : blocksNeeded + 1; // Index at the end of FAT block + 1
}

int getFreeBlocks(uint64_t numberOfBlocks) {
    // TO DO: Input handling

    int head = vcb->freeSpaceLocation;
    int currentBlock = vcb->freeSpaceLocation;
    int nextBlock = FAT[currentBlock];
    // derement total free space because head block
    vcb->totalFreeSpace--;

    for( int i = 1; i < numberOfBlocks; i ++ ) {
        currentBlock = nextBlock; //Careful with this part will look later (Kevin looked and fixed)
        nextBlock = fat[currentBlock];
        volumeControlBlock->totalFreeSpace--;
    }
    fat[currentBlock] = 0xFFFFFFFD;
    volumeControlBlock->freeSpaceLocation = nextBlock;
    return head;
}

int writeBlock(uint64_t numberOfBlocks, void * buffer, int location) {
    // TO DO: Input handling

    int blockSize = vcb->blockSize;
    int blocksWritten = 0;
    for (int i = 0; i < numberOfBlocks; i++) {
        if (LBAwrite(buffer + blockSize * blockWritten, 1, location) == -1) {
            return -1;
        }
        location = FAT[location];
        blocksWritten++;
    }

    return blocksWritten;
}