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

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct b_fcb {
	DirectoryEntry* fi; //holds the file information

	/** TODO add al the information you need in the file control block **/
	char* buf;			//holds the open file buffer
	int index;			//holds the current position in the buffer
	int buflen;			//holds how many valid bytes are in the buffer

	int fileIndex;		//holds the index at where the file is located at in the parent
} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init() {
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
			return i;		//Not thread safe (But do not worry about it for this assignment)
		}
	}
	return (-1); //all in use
}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char* filename, int flags) {
	b_io_fd returnFd;
	ppInfo ppi;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB(); // get our own file descriptor
	if (returnFd == -1) { // check for error - all used FCB's
		return returnFd;
	}

	if (parsePath(filename, &ppi) == -1) {
        return -1;
    }

	b_fcb fcb = fcbArray[returnFd];
	fcb.fi = malloc(sizeof(DirectoryEntry));
	fcb.buf = malloc(vcb->blockSize);
	
	// Create a new file
	if (flags & O_CREAT) {
		strncpy(fcb.fi->name, ppi.lastElement, sizeof(fcb.fi->name));
		fcb.fi->size = 0;
		fcb.fi->location = -2; // Special sentinel to indicate file occupied space
		fcb.fi->isDirectory = ' ';
		time_t currTime = time(NULL);
		fcb.fi->dateCreated = currTime;
		fcb.fi->dateModified = currTime;

		//[TO-DO] update variable information needed for read/write

		DirectoryEntry* temp = loadDir(ppi.parent);
    	ppi.parent = temp;

		fcb.fileIndex = findUnusedDE(ppi.parent);
		if (fcb.fileIndex == -1) {
			ppi.parent = expandDirectory(ppi.parent);
        	fcb.fileIndex = findUnusedDE(ppi.parent);
		}

		// Fatal error if no vacant entry spot is found
		if (fcb.fileIndex < 0) {
			free(fcb.fi);
			free(fcb.buf);
			free(temp);
			return -1;
		}

		free(temp);
	}

	ppi.parent[fcb.fileIndex] = *fcb.fi; //dereference file information and write to disk
	
	// Write file to disk
	int numOfBlocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
	writeBlock(ppi.parent, numOfBlocks, ppi.parent[0].location);

	fcbArray[returnFd] = fcb;
	
	return (returnFd); // all set
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

	if (fcbArray[fd].activeFlags & O_CREAT) {

	}

	b_fcb fcb = fcbArray[fd];

	int bytesInBlock; // The space the file occupies on disk
	int part1, part2, part3;
	int numberOfBlocksToWrite;
	
	// Calculate the amount of free space in file block(s)
	if (fcb.fi->size <= 0) {
		fcb.fi->size = 0;
		bytesInBlock = vcb->blockSize;
		fcb.buflen = vcb->blockSize;
	} else {
		bytesInBlock = fcb.buflen - fcb.index;
	}

	int remainingSpace = (fcb.fi->size + count) - (fcb.blockSize * vcb->blockSize);
	int newBlocksRequired = (remainingSpace + vcb->blockSize - 1) / vcb->blockSize;
	
	// Grow file size if necessary

	if (newBlocksRequired > 0) {
		newBlocksRequired = newBlocksRequired > fcb.blockSize ? newBlocksRequired : fcb.blockSize;
		int newBlocks = getFreeBlocks(newBlocksRequired, 0);
		int lastBlockInDisk = seekBlock(fcb.blockSize - 1, fcb.fi->location);
		if (fcb.fi->location == -2) { // Fresh file data in disk
			fcb.currentBlock = newBlocks;
			fcb.fi->location = newBlocks;
		} else { // Existing file data in disk
			FAT[lastBlockInDisk] = newBlocks;
		}
		fcb.blockSize += newBlocksRequired;
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
		memcpy(fcb.buf + fcb.index, buffer, part1);
		// for (int i = 1; i < 20; i++) {
		// 	printf("%s", fcb.buf[i]);
		// }
		// printf(fcb.buf[1]);
		printf("%d", fcb.currentBlock);
		writeBlock(fcb.buf, 1, fcb.currentBlock);
		fcb.index += part1;
	}

	if (part2 > 0) {
		int blocksWritten = writeBlock(buffer + part1, numberOfBlocksToWrite, fcb.currentBlock);
		fcb.currentBlock = seekBlock(numberOfBlocksToWrite, fcb.currentBlock);
		part2 = blocksWritten * vcb->blockSize;
	}

	if (part3 > 0) {
		memcpy(fcb.buf, buffer + part1 + part2, part3);
		fcb.currentBlock = seekBlock(1, fcb.currentBlock);
		writeBlock(fcb.buf, 1, fcb.currentBlock);
		fcb.index = part3;
	}

	fcb.fi->size += part1 + part2 + part3;

	// Update Directory Entry in parent
	fcbArray[fd] = fcb;
    struct DirectoryEntry* parent = fcbArray[fd].parent;
    parent[fcb.fileIndex].size = fcb.fi;

    int parentSizeInBlocks = (parent->size + vcb->blockSize - 1) / vcb->blockSize;
    writeBlock(parent, parentSizeInBlocks, parent->location);
	
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

	if (startup == 0) b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS)) {
		return (-1); // invalid file descriptor
	}

	// and check that the specified FCB is actually in use
	if (fcbArray[fd].fi == NULL) { // File not open for this descriptor 
		return -1;
	}

	return 0;

// 	b_fcb *fcb = &fcbArray[fd];
// 	int bytesRead = 0;

// 	int bytesRemainingInFile = fcb->fi->fileSize - fcb->position;
// 	int bytesToRead = MIN(count, bytesRemainingInFile);

// 	// At the end of the file
// 	if (bytesToRead <= 0)
// 		return 0;

// 	// Calculate current buffer position and bytes left in buffer
// 	int bytesRemainingInBuffer = B_CHUNK_SIZE - fcb->bufferPosition;
// 	int bytesToReadFromBuffer = MIN(bytesToRead, bytesRemainingInBuffer);

// 	// when there are already bytes in the buffer
// 	if (bytesToReadFromBuffer > 0)
// 	{
// 		// Read any remaining bytes from a previous read into the buffer.
// 		memcpy(buffer + bytesRead, fcb->buffer + fcb->bufferPosition, bytesToReadFromBuffer);

// 		// update trackers
// 		fcb->position += bytesToReadFromBuffer;
// 		fcb->bufferPosition = fcb->position % B_CHUNK_SIZE;
// 		bytesRead += bytesToReadFromBuffer;
// 		bytesToRead -= bytesToReadFromBuffer;
// 	}

// 	// Read full chunks directly to the user buffer
// 	if (bytesToRead >= B_CHUNK_SIZE)
// 	{
// 		int fullChunksToRead = bytesToRead / B_CHUNK_SIZE;
// // NEED TO UPDATE CURRENT BLOCK TO FIND CORRECT BLOCK IN FAT
// 		int currentBlock = (fcb->position / B_CHUNK_SIZE) + fcb->fi->location;

// 		// Read as many chunks needed ant calculate the bytes that were read to update trackers
// 		int chunksRead = readBlock(fcb->buf, fullChunksToRead,	currentBlock);
// 		if (chunksRead < 1) {
// 			return -1;
// 		}
// 		int bytesReadFromChunks = chunksRead * B_CHUNK_SIZE;

// 		// update trackers
// 		fcb->position += bytesReadFromChunks;
// 		fcb->bufferPosition = fcb->position % B_CHUNK_SIZE;
// 		bytesRead += bytesReadFromChunks;
// 		bytesToRead -= bytesReadFromChunks;
// 	}

// 	// Read any remaining bytes into the buffer and copy them to the user buffer
// 	if (bytesToRead > 0)
// 	{
// 		int currentBlock = (fcb->position / B_CHUNK_SIZE) + fcb->fi->location;
// // NEED TO UPDATE CURRENT BLOCK TO FIND CORRECT BLOCK IN FAT
// 		if(readBlock(fcb->buffer, 1, currentBlock) != 1) {
// 			return -1;
// 		}
// 		memcpy(buffer + bytesRead, fcb->buffer, bytesToRead);

// 		// update trackers
// 		fcb->position += bytesToRead;
// 		fcb->bufferPosition = fcb->position % B_CHUNK_SIZE;
// 		bytesRead += bytesToRead;
// 	}

// 	return bytesRead;
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd) {
	if (fd < 0 || fd >= MAXFCBS || fcbArray[fd].fi == NULL) {
		return -1;
	}

	fcbArray[fd].fi = NULL;
	free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;

	return 0;
}