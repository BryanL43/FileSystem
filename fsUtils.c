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
 * @return the loaded directory which **needs to be freed** or NULL if failed.
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
    if (directory == NULL) {
        return -1;
    }

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
 * Checks if the directory is empty or not.
 * 
 * @param directory the directory that is being checked.
 * @return -1 if not empty, 1 if empty.
*/
int isDirEmpty(DirectoryEntry *directory) {
    for (int i = 2; i < directory->size / sizeof(DirectoryEntry); i++) {
        if (directory[i].location > 0) {
            return -1;
        }
    }
    return 0;
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

    DirectoryEntry* parent;
    if (path[0] == '/') {
        parent = root;
    } else {
        parent = cwd;
    }

    DirectoryEntry* currentDir = loadDir(parent);
    if (currentDir == NULL) {
        return -1;
    }

    char* saveptr;
    char* token1 = strtok_r(path, "/", &saveptr);
    if (token1 == NULL) { // Special case "/"
        ppi->parent = parent;
        ppi->lastElement = NULL;
        ppi->lastElementIndex = -2; // special sentinel
        free(currentDir);
        return 0;
    }

    char* token2;
    do {
        ppi->lastElement = token1;
        ppi->lastElementIndex = findNameInDir(currentDir, token1);

        token2 = strtok_r(NULL, "/", &saveptr);
        if (token2 == NULL) {
            ppi->parent = currentDir;
            return 0;
        }

        if (ppi->lastElementIndex < 0) { // Name doesn't exist (Invalid path)
            free(currentDir);
            return -1;
        }

        if (currentDir[ppi->lastElementIndex].isDirectory != 'd') {
            free(currentDir);
            return -1;
        }

        DirectoryEntry* temp = loadDir(&(currentDir[ppi->lastElementIndex]));
        if (temp == NULL) {
            free(currentDir);
            return -1;
        }
        free(currentDir);
        currentDir = temp;
        token1 = token2;
    } while (token2 != NULL);

    return 0;
}

/**
 * Checks if the directory is empty or not
 * 
 * @param dir the directory that is being checked
 * 
 * @return -1 if not empty, 1 if empty
*/
int isDirEmpty(DirectoryEntry *dir) {
    for (int i = 2; i < dir->size / sizeof(DirectoryEntry); i++) {
        if (dir[i].location > 0) {
            return -1;
        }
    }
    return 0;
}