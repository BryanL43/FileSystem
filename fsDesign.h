#ifndef _FSDES_H
#define _FSDES_H
#include <time.h>

typedef struct VCB
{
   long signature;         // to identify VCB

   int blockSize;          // size of each block
   int totalBlocks;        // total blocks in volume

   int freeSpaceLocation;  // location to free space block
   int firstFreeBlock;     // location to the first free block
   int totalFreeSpace;     // total free blocks

   int rootLocation;       // location to root directory
   int rootSize;           // size of root directory
} VCB;

extern struct VCB* vcb;
extern int* FAT;

#endif