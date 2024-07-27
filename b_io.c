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

	b_io_fd returnFd;
	ppInfo ppi;
	
	returnFd = b_getFCB(); // get our own file descriptor
	if (returnFd == -1) { // check for error - all used FCB's
		return returnFd;
	}

	// Setting our fcb to what is stored in fcb array
	b_fcb fcb = fcbArray[returnFd];

	// Filling out ppi
	if (parsePath(filename, &ppi) == -1) {
        return -1;
    }

	// Create a file if it doesn't exist
	if (ppi.lastElementIndex == -1 && flags & O_CREAT) {
		createFile(filename, &ppi);
		fcb.fileIndex = ppi.lastElementIndex;
	}

	// Check if last element index is a valid file if create is not specified
	if (ppi.parent[ppi.lastElementIndex].isDirectory != ' ') {
		return -1;
	}

	// Truncate a file (requires write access)
	if (ppi.lastElementIndex > 0 && (flags & O_TRUNC) && (flags & O_WRONLY)) {
		// Populate the file information
		ppi.parent[ppi.lastElementIndex].location = -2;
		ppi.parent[ppi.lastElementIndex].size = 0;

		// Initialize variable information needed for b_read/b_write
		fcb.index = fcb.buflen = fcb.currentBlock = fcb.blockSize = 0;

		deleteBlob(ppi);
	}

	fcb.fi = &ppi.parent[ppi.lastElementIndex]; //assign temp file to fcb.fi
	// Instantiate file control block buffer
	fcb.buf = malloc(vcb->blockSize);
	if (fcb.buf == NULL) {
		return -1;
	}

	// Append a file
	if (flags & O_APPEND) {
		fcb.index = fcb.fi->size;
	}

	// Populate/verify file control block's variable
	fcb.index = (fcb.index > 0) ? fcb.index : 0;
	fcb.buflen = (fcb.buflen > 0) ? fcb.buflen : 0;
	fcb.currentBlock = (fcb.currentBlock > 0) ? fcb.currentBlock : 0;
	fcb.blockSize = (fcb.blockSize > 0) ? fcb.blockSize : 0;
	fcb.fileIndex = (fcb.fileIndex == 0) ? ppi.lastElementIndex : fcb.fileIndex;

	fcb.parent = ppi.parent; //allow access to write
	fcb.activeFlags = flags;
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
    if ((fcbArray[fd].activeFlags & O_ACCMODE) == O_RDONLY) {
        return -1;
    }

    b_fcb* fcb = &fcbArray[fd];

	int location = fcb->fi->location; //stores the location temporarily
	int part1, part2, part3;

	fcb->buflen = vcb->blockSize;


    // Grow file size if necessary
	if (location == -2) { // When the file doesn't take up space in disk
		int newFreeIndex = getFreeBlocks(1, 0);
		fcb->blockSize++;

		// Set index to start writing based on free space
		fcb->currentBlock = newFreeIndex;
		fcb->fi->location = newFreeIndex;
		fcb->parent[fcb->fileIndex].location = newFreeIndex;

	// When the file already exist and needs to write more
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

		// Completing filling our buffer
		memcpy(fcb->buf + fcb->index, buffer, remainingBytes);

		// Commit the full buffer
		writeBlock(fcb->buf, 1, fcb->currentBlock);

		buffer += remainingBytes;
		fcb->currentBlock++;

		// Copy the remaining bytes into our buffer
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
	fcb->parent[fcb->fileIndex].dateModified = currentTime;

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
int b_read(b_io_fd fd, char *buffer, int count) {
    int bytesRead = 0;       // Bytes read in each operation
    int bytesReturned = 0;   // Total bytes returned
    int part = 0, part2 = 0, part3 = 0;  // Parts to read
    int numberOfBlocksToCopy = 0;  // Blocks to read
    int remainingBytesInMyBuffer = 0;  // Remaining bytes in buffer
    
    if (startup == 0) b_init(); // Initialize system if needed

    // Validate file descriptor
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return -1; // Invalid file descriptor
    }

    // Check if file is open
    if (fcbArray[fd].fi == NULL) { 
        return -1; // File not open for this descriptor
    }

    // Check if file is open in write-only mode
    if ((fcbArray[fd].activeFlags & O_ACCMODE) == O_WRONLY) {
        return -1;
    }

    // Use a pointer to modify the global fcb
    b_fcb *fcb = &fcbArray[fd];  

    // Calculate remaining bytes in buffer
    remainingBytesInMyBuffer = fcb->buflen - fcb->index;

    // Calculate amount already delivered to ensure count doesn't exceed file size
    int amountAlreadyDelivered = (fcb->currentBlock * B_CHUNK_SIZE) - remainingBytesInMyBuffer;
    
    // Adjust count to ensure we don't read beyond the file size
    if ((count + amountAlreadyDelivered) > fcb->fi->size) {
        count = fcb->fi->size - amountAlreadyDelivered;
        if (count < 0) {
            return -1;
        }
    }

    // Determine how many parts need to be read
    if (remainingBytesInMyBuffer >= count) {
        part = count;
        part2 = 0;
        part3 = 0;
    } else {
        part = remainingBytesInMyBuffer;
        part3 = count - remainingBytesInMyBuffer;
        numberOfBlocksToCopy = part3 / B_CHUNK_SIZE;
        part2 = numberOfBlocksToCopy * B_CHUNK_SIZE;
        part3 = part3 - part2;
    }

    // Read the first part from the current buffer
    if (part > 0) {
        if (fcb->index < fcb->buflen) {
            memcpy(buffer, fcb->buf + fcb->index, part);
            fcb->index += part;
            bytesReturned += part;
        } else {
            return -1; // Error: trying to read beyond available buffer
        }
    }

    // Read part2 directly from the disk if needed
    if (part2 > 0) {
        bytesRead = readBlock(buffer + part, numberOfBlocksToCopy, fcb->currentBlock + fcb->fi->location);

        // Ensure read was successful
        if (bytesRead < numberOfBlocksToCopy) {
            part2 = bytesRead * B_CHUNK_SIZE;
        }

        fcb->currentBlock += bytesRead;
        bytesReturned += part2;
    }

    // Read any remaining bytes into the buffer
    if (part3 > 0) {
        bytesRead = readBlock(fcb->buf, 1, fcb->currentBlock + fcb->fi->location);

        // Check if read was successful
        if (bytesRead <= 0) {
            return -1; // Error: read failed or no more data
        }

        fcb->currentBlock += 1;
        fcb->index = 0;
        fcb->buflen = bytesRead * B_CHUNK_SIZE;

        if (bytesRead * B_CHUNK_SIZE < part3) {
            part3 = bytesRead * B_CHUNK_SIZE;
        }

        if (part3 > 0) {
            memcpy(buffer + part + part2, fcb->buf + fcb->index, part3);
            fcb->index += part3;
            bytesReturned += part3;
        }
    }

    // Return total bytes read
    return bytesReturned;
}



// Interface to Close the file	
int b_close (b_io_fd fd) {
	if (fd < 0 || fd >= MAXFCBS || fcbArray[fd].fi == NULL) {
		return -1;
	}
	fcbArray[fd].buflen = 0;
	fcbArray[fd].currentBlock = 0;
	fcbArray[fd].index = 0;
	fcbArray[fd].fi = NULL;
	// free(fcbArray[fd].parent);
	// free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;

	return 0;
}