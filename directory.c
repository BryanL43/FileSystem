/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: directory.c
*
* Description:: Robust directory creation method
*
**************************************************************/

#include "directory.h"
#include <time.h>

/**
 * Creates a directory.
 * 
 * @param minEntries Minimum number of entries the directory should contain.
 * @param parent Pointer to the parent directory entry.
 * @return Pointer to the initialized directory entries or NULL if initialization failed.
*/
DirectoryEntry *initDirectory(int minEntries, DirectoryEntry *parent)
{
    // Calculate memory requirements for the directory
    int bytesNeeded = minEntries * sizeof(DirectoryEntry);
    int blocksNeeded = (bytesNeeded + vcb->blockSize - 1) / vcb->blockSize;
    int bytesToAlloc = blocksNeeded * vcb->blockSize;
    int actualEntries = bytesToAlloc / sizeof(DirectoryEntry);
    
    // Prepare space for the directory entries
    DirectoryEntry *DEs = malloc(bytesToAlloc);
    if (DEs == NULL) {
        return NULL;
    }

    // Initialize each directory entry to a known free state
    for(int i = 2; i < actualEntries; i++) {
        time_t currentTime = time(NULL);
        DEs[i].dateCreated = currentTime;
        DEs[i].dateModified = currentTime;
        strcpy(DEs[i].name, "");
        DEs[i].isDirectory = ' ';
        DEs[i].size = 0;
        DEs[i].location = -1;
    }

    // Request free space for the directory
    int newLoc = getFreeBlocks(blocksNeeded, NULL);
    if (newLoc == -1) {
        return NULL;
    }

    time_t currentTime = time(NULL);

    // Initialize "."
    DEs[0].location = newLoc;
    DEs[0].size = actualEntries * sizeof(DirectoryEntry);
    strcpy(DEs[0].name, ".");
    DEs[0].isDirectory = 'd';
    DEs[0].dateCreated = currentTime;
    DEs[0].dateModified = currentTime;

    // Initialize ".."
    DirectoryEntry *dotdot = parent;
    if (dotdot == NULL) {
        dotdot = &DEs[0];
    }

    // Copy the parent into the ".." directory
    memcpy(&DEs[1], dotdot, sizeof(DirectoryEntry));
    strcpy(DEs[1].name, "..");

    // Write the directory blocks to volume
    if (writeBlock(DEs, blocksNeeded, newLoc) == -1) {
        return NULL;
    }

    return DEs;
}