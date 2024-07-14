/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for the file system assignment.
* This file initialize the volume control block, file allocation table,
* and the root directory.
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
DirectoryEntry* cwd;
char* cwdPathName;

/**
 * Initialize the file system's VCB, FAT, and root directory.
 * 
 * @param numberOfBlocks The total number of blocks available in the file system.
 * @param blockSize The size of each block in bytes.
 * @return Returns 0 on success, and -1 on failure.
*/
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

	// Instantiate current working directory path name
	cwdPathName = malloc(blockSize); //Size up for debate
	if (cwdPathName == NULL) {
		printf("Failed to instantiate cwd path name!\n");
		free(vcb);
		free(FAT);
		return -1;
	}

	LBAread(vcb, 1, 0);

	// Check if the file system is initialized already
	if (vcb->signature == SIGNATURE) { // Case: Initialized already
		printf("Disk already formatted!\n");
		// Load the FAT from the storage
		int bytesNeeded = numberOfBlocks * sizeof(int);
		int blocksNeeded = (bytesNeeded + blockSize - 1) / blockSize;
        LBAread(FAT, blocksNeeded, vcb->freeSpaceLocation);

		// Instantiate and load root directory
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
		
	} else { //Case: File system is not yet initialized
		vcb->signature = SIGNATURE;
		vcb->blockSize = blockSize;
		vcb->totalBlocks = numberOfBlocks;

		// Initalize the FAT free space management system
		if (initFreeSpace(numberOfBlocks, blockSize) != 0) {
			printf("Failed to initialize free space!\n");
			free(vcb);
			free(FAT);
			return -1;
		}

		// Initialize the root directory
		root = initDirectory(DEFAULT_DIR_SIZE, NULL);
		if (root == NULL) {
			printf("Failed to initialize the root directory!\n");
			free(vcb);
			free(FAT);
			return -1;
		}

		// Update root information in VCB
		vcb->rootLocation = root->location;
		vcb->rootSize = (root->size + blockSize - 1) / blockSize;

		// Write the VCB to the volume
		if (LBAwrite(vcb, 1, 0) == -1) {
			printf("Error writing VCB!\n");
			free(vcb);
			free(FAT);
			free(root);
			return -1;
		}
	}
	
	//Set current working directory as root
	fs_setcwd("/");
	strncpy(cwdPathName, "/", 2);
	cwdPathName[1] = '\0';

	return 0;
}

void exitFileSystem()
{
	free(vcb);
	free(FAT);
	free(root);
	free(cwd);
	free(cwdPathName);
	printf("System exiting\n");
}