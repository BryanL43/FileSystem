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

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize)
{
	printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);


	//Instantiate a volume control block instance
	vcb = malloc(blockSize);
	if (vcb == NULL) {
		printf("Failed to instantiate VCB!\n");
		return -1;
	}

	//Instantiate a FAT instance
	FAT = malloc(numberOfBlocks * sizeof(int));
	if (FAT == NULL) {
		printf("Failed to instantiate FAT!\n");
		free(vcb);
		return -1;
	}

	LBAread(vcb, 1, 0);

	if (vcb->signature == SIGNATURE) {
		//I will do more research in this section to check what we need to do
		//for if the signature exist 


		//load freespace
		//@parameters(VCB* vcb, int blocksize, or whatever else you need)
		//no return value up to change


		//load rootdirectiory
		//@parameters(int rootlocation, or whatever else you need)
		//no return value for now
		
	} else {
		vcb->signature = SIGNATURE;
		vcb->blockSize = blockSize;
		vcb->totalBlocks = numberOfBlocks;

		//Initalize Freespace
		if (initFreeSpace(numberOfBlocks, blockSize) != 0) {
			printf("Failed to initialize free space!\n");
			free(vcb);
			free(FAT);
			return -1;
		}

		// Initialize the root directory
		DirectoryEntry* root = initDirectory(20, NULL);
		if (root == NULL) {
			printf("Failed to initialize the root directory!\n");
			free(vcb);
			free(FAT);
			return -1;
		}

		vcb->rootLocation = root->location;

		if (LBAwrite(vcb, 1, 0) == -1) {
			printf("Error writing vcb!\n");
			free(vcb);
			free(FAT);
			return -1;
		}
	}

	return 0;
}

void exitFileSystem()
{
	printf("System exiting\n");
}