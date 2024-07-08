#ifndef _FREESPC_H
#define _FREESPC_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"

int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize);

int getFreeBlocks(uint64_t numberOfBlocks);

int writeBlock(uint64_t numberOfBlocks, void * buffer, int location);

#endif