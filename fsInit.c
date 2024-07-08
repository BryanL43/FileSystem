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
#include "fsLow.h"
#include "mfs.h"

#define SIGNATURE 7263366117696533168

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize)
{
	printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

	int blocksNeeded = (numberOfBlocks + blockSize - 1) / blockSize;

	//Instantiate a volume control block instance
	VCB* vcb = malloc(blockSize);
	if (vcb == NULL) {
		printf("Failed to instantiate VCB!\n");
		return -1;
	}

	//Instantiate root directory
	DirectoryEntry* root = malloc(sizeof(DirectoryEntry));
	if (root == NULL) {
		printf("Failed to instantiate root directory!\n");
		return -1;
	}

	LBAread(vcb, 1, 0);

	if (vcb->signature == SIGNATURE)
	{
		//I will do more research in this section to check what we need to do
		//for if the signature exist 


		//load freespace
		//@parameters(VCB* vcb, int blocksize, or whatever else you need)
		//no return value up to change


		//load rootdirectiory
		//@parameters(int rootlocation, or whatever else you need)
		//no return value for now
		
	}else{
		vcb->signature = SIGNATURE;
		vcb->blockSize = blockSize;
		vcb->totalBlocks = numberOfBlocks;
		// call init free space,
		//@parameters (vcb, or whatever else you need)
		// return value should be int for location 
		vcb->totalFreeSpace;
		vcb->freeSpaceLocation = 1;

		//call initDirectory
		// return value should be int for location of root directory
		// initDirectory(vcb->blocksize, Null);
		vcb->rootLocation;

	}
	

	return 0;
}

void exitFileSystem()
{
	printf("System exiting\n");
}