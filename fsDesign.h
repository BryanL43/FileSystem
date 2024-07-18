/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: fsDesign.h
*
* Description:: Header file for basic file system controls.
*
**************************************************************/

#ifndef _FSDES_H
#define _FSDES_H
#include <time.h>
#include "directory.h"

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

typedef struct ppInfo {
   struct DirectoryEntry* parent;
   char* lastElement;
   int lastElementIndex;
} ppInfo;

//Note: actual return is ppi while int is status return (0 or -1).
int parsePath(char* path, ppInfo* ppi);
struct DirectoryEntry* loadDir(struct DirectoryEntry* directory);

extern struct VCB* vcb;
extern int* FAT;

#endif