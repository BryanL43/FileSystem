/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: fsUtils.h
*
* Description:: Important utility/helper functions for directory operations.
*
**************************************************************/

#include "fsUtils.h"

/**
 * Load a directory
 * 
 * @param directory the parent directory to be loaded.
 * @return the loaded directory which **needs to be freed**.
*/
DirectoryEntry* loadDir(DirectoryEntry* directory) {
    int location = directory->location;
    DirectoryEntry* temp = malloc(directory->size);
    if (temp == NULL) {
        return NULL;
    }

    int numberOfBlocks = (directory->size + vcb->blockSize - 1) / vcb->blockSize;
    if (readBlock(temp, numberOfBlocks, location) == -1) {
        free(temp);
        return NULL;
    }

    return temp;
}

/**
 * Find the index of an empty directory entry.
 * 
 * @param directory the directory entry that is being searched for unoccupied space.
 * @return the index on success or -1 if not found.
*/
int findUnusedDE(DirectoryEntry* directory) {
    for (int i = 0; i < directory->size / sizeof(DirectoryEntry); i++) {
        if ((directory + i)->location == -1) {
            return i;
        }
    }
    return -1;
}

/**
 * Find the index of a child directory/directory entry.
 * 
 * @param directory the directory entry that is being searched
 *                  (could be the directory not the individual entry).
 * @param name the name of the directory that is being searched for.
 * @return the index of the directory entry or -1 if not found.
*/
int findNameInDir(DirectoryEntry* directory, char* name) {
    if (directory == NULL || name == NULL) {
        return -1;
    }

    int numEntries = directory->size / sizeof(DirectoryEntry);
    for (int i = 0; i < numEntries; i++) {
        if (directory[i].location != -1 && strcmp(directory[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Parse the given path
 * 
 * @param path the path that is being parsed.
 * @param ppi the parse path information structure that will be populated
 *            with information extracted from the given path.
 * @return 0 on success and -1 on failure.
*/
int parsePath(char* path, ppInfo* ppi) {
    if (path == NULL || ppi == NULL) {
        return -1;
    }

    DirectoryEntry* start;
    if (path[0] == '/') {
        start = root;
    } else {
        start = cwd; //breakpoint pls verify!!!
    }

    DirectoryEntry* parent = start;
    char* saveptr;
    char* token1 = strtok_r(path, "/", &saveptr);
    if (token1 == NULL) { // Special case "/"
        ppi->parent = parent;
        ppi->lastElement = NULL;
        ppi->lastElementIndex = -2; // special sentinel
        return 0;
    }

    ppi->lastElement = token1;
    ppi->lastElementIndex = findNameInDir(parent, token1);
    
    char* token2 = strtok_r(NULL, "/", &saveptr);
    if (token2 == NULL) {
        ppi->parent = parent;
        return 0;
    }

    if (ppi->lastElementIndex < 0) { // Name doesn't exist (Invalid path)
        return -1;
    }

    //Not finished

    return 0;
}