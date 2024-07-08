#include "freeSpace.h"
#include "fsDesign.h"

int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize) {
    int blocksNeeded = (numberOfBlocks + blockSize -1)/blockSize;
    int *FAT = malloc(blocksNeeded*blockSize);

    FAT[0] = 0xFFFFFFFF;
    for (int i = 1; i < numberOfBlocks; i++) {
        FAT[i] = i+1;
    }

    // Sentinel value for FAT storage
    FAT(blocksNeeded) = 0xFFFFFFFF;

    // Sentinel value for free space storage 
    FAT(numberOfBlocks) = 0xFFFFFFFF

    int blocksWritten = LBAwrite(FAT, blocksNeeded, 1);
    // TO DO: Update VCB values after initializing VCB

    if (blocksWritten == -1) {
        return -1;
    } else {
        return blocksNeeded + 1; // index at the end of FAT block + 1
    }

}

int getFreeBlocks(uint64_t numberOfBlocks) {

}

int writeBlock(uint64_t numberOfBlocks, void * buffer, int location) {

}