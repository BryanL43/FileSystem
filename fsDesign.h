#ifndef _FSDES_H
#define _FSDES_H
#include <time.h>

typedef struct DirectoryEntry
{
   time_t dateCreated;     // seconds since epoch
   time_t dateModified;    // seconds since epoch

   char name[51];          // directory entry name
   char isDirectory;       // is directory entry a directory

   int size;               // directory entry size
   int location;           // starting block number for the file data
   int permissions;        // file modes converted to decimal format

   // total size = 80, divisible by 16, and no internal paddings
} DirectoryEntry;


typedef struct VCB
{
   long signature;         // to identify VCB

   int blockSize;          // size of each block
   int totalBlocks;        // total blocks in volume

   int freeSpaceLocation;  // location to free space block
   int firstFreeBlock;     // location to the first free block
   int totalFreeSpace;     // total free blocks

   int rootLocation;       // location to root directory
} VCB;

extern struct VCB* vcb;
extern int* FAT;

#endif