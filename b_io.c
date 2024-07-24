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

	// Ensure flag exists
	if (!flags) {
		return -1;
	}

	b_io_fd returnFd;
	ppInfo ppi;

	DirectoryEntry file; //holds the temp file that will be placed into fcb.fi
	
	returnFd = b_getFCB(); // get our own file descriptor
	if (returnFd == -1) { // check for error - all used FCB's
		return returnFd;
	}

	if (parsePath(filename, &ppi) == -1) {
        return -1;
    }

	// Check if last element index is a directory
	if (ppi.lastElementIndex > 0 && ppi.parent[ppi.lastElementIndex].isDirectory == 'd') {
		return -1;
	}

	b_fcb fcb = fcbArray[returnFd];

	// Create a file if it doesn't exist
	if (ppi.lastElementIndex == -1 && flags & O_CREAT) {
		printf("Create fired!\n");
		// Populate the file information
		strncpy(file.name, ppi.lastElement, sizeof(file.name));
		file.size = 0;
		file.location = -2; // Special sentinel to indicate fresh file with 0 size
		file.isDirectory = ' ';
		time_t currTime = time(NULL);
		file.dateCreated = currTime;
		file.dateModified = currTime;

		// Initialize variable information needed for b_read/b_write
		DirectoryEntry* temp = loadDir(ppi.parent);
    	ppi.parent = temp;

		fcb.fileIndex = findUnusedDE(ppi.parent);
		if (fcb.fileIndex == -1) {
			ppi.parent = expandDirectory(ppi.parent);
        	fcb.fileIndex = findUnusedDE(ppi.parent);
		}

		// Fatal error if no vacant entry spot is found
		if (fcb.fileIndex < 0) {
			fcb.fi = NULL;
			free(fcb.buf);
			free(temp);
			return -1;
		}
	}

	// Truncate a file (requires write access)
	if (ppi.lastElementIndex > 0 && (flags & O_TRUNC) && (flags & O_WRONLY)) {
		printf("Truncate fired!\n");
		// Populate the file information
		file = ppi.parent[ppi.lastElementIndex];
		file.location = -2;
		file.size = 0;

		// Initialize variable information needed for b_read/b_write
		fcb.index = fcb.buflen = fcb.currentBlock = fcb.blockSize = 0;

		fs_delete(ppi.lastElement);
	}
	
	// Append a file
	if (flags & O_APPEND) {
		printf("Append fired!\n");
	}

	fcb.fi = &file; //assign temp file to fcb.fi

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
	ppi.parent[fcb.fileIndex] = *fcb.fi; //dereference file information and write to disk

	// Write parent to disk for updating
	int numOfBlocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
	writeBlock(ppi.parent, numOfBlocks, ppi.parent[0].location);

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

    int bytesInBlock; // The space the file occupies on disk
    int part1, part2, part3;
    int numberOfBlocksToWrite;
	int newFreeIndex;

    // Calculate the amount of free space in file block(s)
    if (fcb->fi->size <= 0) {
        fcb->fi->size = 0;
        bytesInBlock = vcb->blockSize;
        fcb->buflen = vcb->blockSize;
    } else {
        bytesInBlock = fcb->buflen - fcb->index;
    }
	
    // Grow file size if necessary
    int remainingSpace = (fcb->fi->size + count) - (fcb->blockSize * vcb->blockSize);
	int blocksNeeded = (remainingSpace + vcb->blockSize - 1) / vcb->blockSize;

	if (blocksNeeded > 0) {
		newFreeIndex = getFreeBlocks(blocksNeeded, 0);
		fcb->currentBlock = newFreeIndex;
		fcb->fi->location = newFreeIndex;
		fcb->blockSize += blocksNeeded;
	}
    
    if (bytesInBlock >= count) {
        part1 = count;
        part2 = 0;
        part3 = 0;
    } else {
        part1 = bytesInBlock;
        part3 = count - bytesInBlock;
        numberOfBlocksToWrite = part3 / vcb->blockSize;
        part2 = numberOfBlocksToWrite * vcb->blockSize;
        part3 = part3 - part2;
    }

    if (part1 > 0) {
        printf("Fired part 1\n");
		printf("fcb->index: %d\n", fcb->index);
		printf("buffer: %s\n", buffer);
        memcpy(fcb->buf + fcb->index, buffer, part1);
		printf("fcb->buf: %s\n", fcb->buf);
        writeBlock(fcb->buf, 1, fcb->currentBlock);
        fcb->index += part1;
    }

    if (part2 > 0) {
		printf("Fired part 2\n");
        int blocksWritten = writeBlock(buffer + part1, numberOfBlocksToWrite, fcb->currentBlock);
        fcb->currentBlock = seekBlock(numberOfBlocksToWrite, fcb->currentBlock);
        part2 = blocksWritten * vcb->blockSize;
    }

    if (part3 > 0) {
		printf("Fired part 3\n");
        memcpy(fcb->buf, buffer + part1 + part2, part3);
        fcb->currentBlock = seekBlock(1, fcb->currentBlock);
        writeBlock(fcb->buf, 1, fcb->currentBlock);
        fcb->index = part3;
    }

    fcb->fi->size += part1 + part2 + part3;

    // Update Directory Entry in parent
    fcbArray[fd] = *fcb;
	fcb->parent[fcb->fileIndex].location = newFreeIndex;
	fcb->parent[fcb->fileIndex].size = fcb->fi->size;

	int parentSizeInBlocks = (fcb->parent->size + vcb->blockSize - 1) / vcb->blockSize;
    writeBlock(fcb->parent, parentSizeInBlocks, fcb->parent->location);
    
    return part1 + part2 + part3;
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
int b_read (b_io_fd fd, char * buffer, int count) {
	int bytesRead;
	int bytesReturned;
	int part, part2, part3;
	int numberOfBlocksToCopy;
	int remainingBytesInMyBuffer;
	

	if (startup == 0) b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS)) {
		return (-1); // invalid file descriptor
	}

	// and check that the specified FCB is actually in use
	if (fcbArray[fd].fi == NULL) { // File not open for this descriptor 
		return -1;
	}

	// check for valid write flag
	if (fcbArray[fd].activeFlags & O_WRONLY) {
		printf("Error file is writeonly, but you tried to read!\n");
		return -1;
	}

	// the meat and potates here
	
	b_fcb fcb = fcbArray[fd];

	// number of bytes avaiable to copy from buffer
	remainingBytesInMyBuffer = fcb.buflen - fcb.index;

	//Limit count to file length
	int amountAlreadyDelivered = (fcb.currentBlock * B_CHUNK_SIZE) - remainingBytesInMyBuffer;
	if((count + amountAlreadyDelivered) > fcb.fi->size){
		count = fcb.fi->size = amountAlreadyDelivered;

		if(count < 0){
			//testing use only
			printf("ERROR: Count: %d - Delivered: %d - CurBlk: %d = Index: %d\n",
			amountAlreadyDelivered, fcb.currentBlock, fcb.index);
		}
	}

	//part 1:
	if(remainingBytesInMyBuffer >= count){
		part = count;
		part2 = 0;
		part3 = 0;
	}
	else
	{
		part = remainingBytesInMyBuffer;

		part3 = count - remainingBytesInMyBuffer;

		numberOfBlocksToCopy = part3/ B_CHUNK_SIZE;
		part2 = numberOfBlocksToCopy * B_CHUNK_SIZE;

		part3 = part3 - part2;
	}

	if(part > 0){
		memcpy(buffer, fcb.buf + fcb.index, part);
		fcb.index = fcb.index + part;
	}

	if(part2 > 0){
		bytesRead = LBAread(buffer+part,numberOfBlocksToCopy,fcb.currentBlock + fcb.fi->location);

		fcb.currentBlock += numberOfBlocksToCopy;
		part2 = bytesRead * B_CHUNK_SIZE;
	}

	if (part3 > 0){
		bytesRead = LBAread(fcb.buf, 1, fcb.currentBlock + fcb.fi->location);

		bytesRead = bytesRead * B_CHUNK_SIZE;

		fcb.currentBlock += 1;

		fcb.index = 0;
		fcb.buflen = bytesRead;

		if (bytesRead < part3)
		{
			part3 = bytesRead;
		}

		if (part3 > 0)
		{
			memcpy(buffer + part + part2, fcb.buf = fcb.index,part3);
			fcb.index = fcb.index + part3;
		}
	}
	
	bytesReturned = part + part2 + part3;

	return bytesReturned;
}


// Interface to Close the file	
int b_close (b_io_fd fd) {
	if (fd < 0 || fd >= MAXFCBS || fcbArray[fd].fi == NULL) {
		return -1;
	}

	fcbArray[fd].fi = NULL;
	free(fcbArray[fd].parent);
	free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;

	return 0;
}