/**************************************************************
 * Class::  CSC-415-0# Spring 2024
 * Name::
 * Student IDs::
 * GitHub-Name::
 * Group-Name::
 * Project:: Basic File System
 *
 * File:: fsInit.c
 *
 * Description:: Main driver for file system assignment.
 *
 * This file is where you will start and initialize your system
 *
 **************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "freeSpace.h"
#include "fsDesign.h"
#include "directory.h"
#include "fsLow.h"
#include "mfs.h"

#define SIGNATURE 7953758755008102994

struct VCB* vcb;
int* FAT;
DirectoryEntry* root;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize)
{
	printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

	// Instantiate a volume control block instance
	vcb = malloc(blockSize);
	if (vcb == NULL) {
		printf("Failed to instantiate VCB!\n");
		return -1;
	}

	// Instantiate a FAT instance
	int blocksNeeded = ((numberOfBlocks * sizeof(int)) + blockSize - 1) / blockSize;
	FAT = malloc(blocksNeeded * blockSize);
	if (FAT == NULL) {
		printf("Failed to instantiate FAT!\n");
		free(vcb);
		return -1;
	}

	LBAread(vcb, 1, 0);

	if (vcb->signature == SIGNATURE) {
		
		// Load FAT
		int bytesNeeded = numberOfBlocks * sizeof(int);
		int blocksNeeded = (bytesNeeded + blockSize - 1) / blockSize;
        LBAread(FAT, blocksNeeded, vcb->freeSpaceLocation);

		// Load root directory
		root = malloc(vcb->rootSize * blockSize);
		if (root == NULL) {
			printf("Failed to allocate root space!\n");
			free(vcb);
			free(FAT);
			return -1;
		}

		if (LBAread(root, vcb->rootSize, vcb->rootLocation) < 0) {
			printf("Failed to read root!\n");
			free(vcb);
			free(FAT);
			free(root);
			return -1;
		}
		
	} else {
		vcb->signature = SIGNATURE;
		vcb->blockSize = blockSize;
		vcb->totalBlocks = numberOfBlocks;

		// Initalize Freespace
		if (initFreeSpace(numberOfBlocks, blockSize) != 0) {
			printf("Failed to initialize free space!\n");
			free(vcb);
			free(FAT);
			return -1;
		}

		// Initialize the root directory
		root = initDirectory(20, NULL);
		if (root == NULL) {
			printf("Failed to initialize the root directory!\n");
			free(vcb);
			free(FAT);
			return -1;
		}
		vcb->rootLocation = root[0].location;
		vcb->rootSize = (root[0].size + blockSize - 1) / blockSize;

		if (LBAwrite(vcb, 1, 0) == -1) {
			printf("Error writing vcb!\n");
			free(vcb);
			free(FAT);
			free(root);
			return -1;
		}
	}

	return 0;
}

void exitFileSystem()
{
	printf("System exiting\n");
}