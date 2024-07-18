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

    time_t currentTime = time(NULL);
    // Initialize each directory entry to a known free state
    for(int i = 2; i < actualEntries; i++) {
        DEs[i].dateCreated = currentTime;
        DEs[i].dateModified = currentTime;
        strncpy(DEs[i].name, "", sizeof(DEs->name));
        DEs[i].isDirectory = ' ';
        DEs[i].size = 0;
        DEs[i].location = -1;
    }

    // Request free space for the directory
    int newLoc = getFreeBlocks(blocksNeeded, 0);
    if (newLoc == -1) {
        return NULL;
    }

    // Initialize "."
    DEs[0].location = newLoc;
    DEs[0].size = bytesToAlloc;
    strncpy(DEs[0].name, ".", sizeof(DEs[0].name));
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
    strncpy(DEs[1].name, "..", sizeof(DEs[1].name));

    // Write the directory blocks to volume
    if (writeBlock(DEs, blocksNeeded, newLoc) == -1) {
        return NULL;
    }

    return DEs;
}


DirectoryEntry* expandDirectory(DirectoryEntry* directory) {
    int oldNumBlocks = (directory->size + vcb->blockSize - 1) / vcb->blockSize;
    int oldNumEntries = directory->size / sizeof(DirectoryEntry);
    
    int numBlocks = oldNumBlocks * 2;
    int bytesToAlloc = numBlocks * vcb->blockSize;
    int actualEntries = bytesToAlloc / sizeof(DirectoryEntry);

    DirectoryEntry *DEs = malloc(bytesToAlloc);
    if (DEs == NULL) {
        return NULL;
    }
    memcpy(DEs, directory, directory->size);
    freeDirectory(directory);
    DEs[0].size *= 2;
    
    time_t currentTime = time(NULL);
    // Initialize each directory entry to a known free state
    for(int i = oldNumEntries; i < actualEntries; i++) {
        DEs[i].dateCreated = currentTime;
        DEs[i].dateModified = currentTime;
        strncpy(DEs[i].name, "", sizeof(DEs->name));
        DEs[i].isDirectory = ' ';
        DEs[i].size = 0;
        DEs[i].location = -1;
    }

    // Request free space for the expanded directory
    int newLoc = getFreeBlocks(oldNumBlocks, seekBlock(oldNumBlocks -1, DEs[0].location));
    if (newLoc == -1) {
        return NULL;
    }

    // Write the directory blocks to volume
    if (writeBlock(DEs, numBlocks, DEs[0].location) == -1) {
        return NULL;
    }

    return DEs;
}  

void freeDirectory(DirectoryEntry* dir) {
    if (dir->location != root->location && dir->location != cwd->location) {
        free(dir);
        dir = NULL;
    }
}

void printDirFromMemory(DirectoryEntry* dir) {
    for(int i = 0; i < dir->size / sizeof(DirectoryEntry); i++) {
        printf("test[%i] location : %i\t\t", i, dir[i].location);
        printf("test[%i] name : %s\n", i, dir[i].name);
    }
    printf("\n");
}

void printDirFromDisk(DirectoryEntry* dir) {
    DirectoryEntry* test = loadDir(dir);
    printDirFromMemory(test);
    freeDirectory(test);
}