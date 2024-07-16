/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: freeSpace.h
*
* Description:: Interface for free space functions.
*
**************************************************************/

#ifndef _FREESPC_H
#define _FREESPC_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "fsDesign.h"
#include "fsLow.h"

int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize);
int getFreeBlocks(uint64_t numberOfBlocks, uint64_t file_to_extend);
int writeBlock(void * buffer, uint64_t numberOfBlocks, int location);
int readBlock(void * buffer, uint64_t numberOfBlocks, int location);
int seekBlock(uint64_t numberOfBlocks, int location);

#endif