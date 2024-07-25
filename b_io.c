/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "freeSpace.h"
#include "fsUtils.h"
#include "fsDesign.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct b_fcb {
	DirectoryEntry* fi; //holds the file information
	DirectoryEntry* parent;

	/** TODO add al the information you need in the file control block **/
	char* buf;			//holds the open file buffer
	int index;			//holds the current position in the buffer
	int buflen;			//holds how many valid bytes are in the buffer
	int currentBlock;	//holds the current block number
	int blockSize;		//holds the total number of blocks the file occupies

	int fileIndex;		//holds the index at where the file is located at in the parent
	int activeFlags;	//holds the flags for the opened file
} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init() {
	if (startup) { // already initialized
		return;
	}

	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++) {
		fcbArray[i].buf = NULL; //indicates a free fcbArray
	}
		
	startup = 1;
}

//Method to get a free FCB element
b_io_fd b_getFCB() {
	for (int i = 0; i < MAXFCBS; i++) {
		if (fcbArray[i].buf == NULL) {
			fcbArray[i].fi = (DirectoryEntry*)-2; // used but not assigned
			return i;		//Not thread safe (But do not worry about it for this assignment)
		}
	}
	return (-1); //all in use
}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char* filename, int flags) {
	if (startup == 0) b_init();  //Initialize our system

	// // Ensure flag exists
	// if (!flags) {
	// 	return -1;
	// }

	b_io_fd returnFd;
	ppInfo ppi;
	
	returnFd = b_getFCB(); // get our own file descriptor
	if (returnFd == -1) { // check for error - all used FCB's
		return returnFd;
	}

	// Check if last element index is a directory
	if (fs_isDir(filename)) {
		return -1;
	}

	b_fcb fcb = fcbArray[returnFd];

	if (parsePath(filename, &ppi) == -1) {
        return -1;
    }

	// Create a file if it doesn't exist
	if (ppi.lastElementIndex == -1 && flags & O_CREAT) {
		createFile(filename, &ppi);
		fcb.fileIndex = ppi.lastElementIndex;
	}

	// Truncate a file (requires write access)
	if (ppi.lastElementIndex > 0 && (flags & O_TRUNC) && (flags & O_WRONLY)) {
		printf("Truncate fired!\n");
		// Populate the file information
		// file = ppi.parent[ppi.lastElementIndex];
		// file.location = -2;
		// file.size = 0;

		// Initialize variable information needed for b_read/b_write
		// fcb.index = fcb.buflen = fcb.currentBlock = fcb.blockSize = 0;

		// deleteBlob(ppi);
	}
	
	// Append a file
	if (flags & O_APPEND) {
		printf("Append fired!\n");
	}
	fcb.fi = &ppi.parent[ppi.lastElementIndex]; //assign temp file to fcb.fi
	// Instantiate file control block buffer
	fcb.buf = malloc(vcb->blockSize);
	if (fcb.buf == NULL) {
		return -1;
	}

	// Populate/verify file control block's variable
	fcb.index = (fcb.index > 0) ? fcb.index : 0;
	fcb.buflen = (fcb.buflen > 0) ? fcb.buflen : 0;
	fcb.currentBlock = (fcb.currentBlock > 0) ? fcb.currentBlock : 0;
	fcb.blockSize = (fcb.blockSize > 0) ? fcb.blockSize : 0;
	fcb.fileIndex = (fcb.fileIndex == 0) ? ppi.lastElementIndex : fcb.fileIndex;

	fcb.parent = ppi.parent; //allow access to write
	// ppi.parent[fcb.fileIndex] = *fcb.fi; //dereference file information and write to disk

	// Write parent to disk for updating
	// int numOfBlocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
	// writeBlock(ppi.parent, numOfBlocks, ppi.parent[0].location);

	fcbArray[returnFd] = fcb;

	return (returnFd);
}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence) {
	if (startup == 0) b_init(); //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS)) {
		return (-1); //invalid file descriptor
	}

	return (0); //Change this
}



// Interface to write function    
int b_write (b_io_fd fd, char * buffer, int count) {
	time_t currentTime = time(NULL);
    if (startup == 0) b_init();  //Initialize our system

    // check that fd is between 0, (MAXFCBS-1), active fd, and valid length
    if ((fd < 0) || (fd >= MAXFCBS) || (fcbArray[fd].fi == NULL) || (count < 0)) {
        return (-1); //invalid file descriptor
    }

    // check for valid write flag
    if (fcbArray[fd].activeFlags & O_RDONLY) {
        printf("File is read only!\n");
        return -1;
    }

    b_fcb* fcb = &fcbArray[fd];

	int location = fcb->fi->location; //stores the location temporarily
	int part1, part2, part3;

	fcb->buflen = vcb->blockSize;
    // Grow file size if necessary
	if (location == -2) {
		int newFreeIndex = getFreeBlocks(1, 0);
		fcb->blockSize++;
		fcb->currentBlock = newFreeIndex;
		fcb->fi->location = newFreeIndex;
		fcb->parent[fcb->fileIndex].location = newFreeIndex;
	} else if (fcb->fi->size + count > fcb->blockSize * vcb->blockSize) {
		int bytesNeeded = count - (fcb->buflen - fcb->index);
		int blocksNeeded = (bytesNeeded + vcb->blockSize - 1) / vcb->blockSize;

		// Updating the FAT values to point old sentinel value to next index
		FAT[fcb->currentBlock] = fcb->currentBlock + 1;
		getFreeBlocks(blocksNeeded, 0);
		fcb->blockSize += blocksNeeded;
	}

	// Writing in blocks at a time
	if (count >= fcb->buflen) {
		int blocksToWrite = count / vcb->blockSize;
		writeBlock(buffer, blocksToWrite, fcb->currentBlock);
		fcb->currentBlock += blocksToWrite;
		fcb->blockSize += blocksToWrite;
	}

	// Our buffer is not completely filled
    if (fcb->index + count < fcb->buflen) {
		memcpy(fcb->buf + fcb->index, buffer, count);
		fcb->index += count;
	} else { // Our buffer is full but there are more data to be filled
		int remainingBytes = fcb->buflen - fcb->index;
		memcpy(fcb->buf + fcb->index, buffer, remainingBytes);
		// Commit the full buffer
		writeBlock(fcb->buf, 1, fcb->currentBlock);

		buffer += remainingBytes;
		fcb->currentBlock++;

		// Write the remaining bytes into our buffer
		memcpy(fcb->buf, buffer, count - remainingBytes);
		fcb->index = count - remainingBytes;
	}

	// Truncate our buffer to remove trash bytes before writing
	char *temp = calloc(1, vcb->blockSize);
	memcpy(temp, fcb->buf, fcb->index);
	writeBlock(temp, 1, fcb->currentBlock);
	free(temp);




    fcb->fi->size += count;

    // Update Directory Entry in parent
    fcbArray[fd] = *fcb;
	fcb->parent[fcb->fileIndex].size = fcb->fi->size;
	fcb->parent[fcb->fileIndex].dateModified = time;

	int parentSizeInBlocks = (fcb->parent->size + vcb->blockSize - 1) / vcb->blockSize;
    writeBlock(fcb->parent, parentSizeInBlocks, fcb->parent->location);
    
    return count;
}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+




// Interface to Close the file	
int b_close (b_io_fd fd) {
	if (fd < 0 || fd >= MAXFCBS || fcbArray[fd].fi == NULL) {
		return -1;
	}
	fcbArray[fd].buflen =0;
	fcbArray[fd].currentBlock = 0;
	fcbArray[fd].index =0;
	fcbArray[fd].fi = NULL;
	// free(fcbArray[fd].parent);
	// free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;

	return 0;
}