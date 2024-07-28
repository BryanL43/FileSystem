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
 * Sanitize path for pwd.
 * 
 * @param pathname the path name.
 * @return the sanitized absolute path or NULL on failure.
*/
char* normalizePath(const char* pathname) {
    int capacity = 10;
    char** pathArray = malloc(sizeof(char*) * capacity);
    if (pathArray == NULL) {
        return NULL;
    }

    // Make a mutable copy of the pathname (discards const for warning issue)
    char* mutablePath = strdup(pathname);
    if (mutablePath == NULL) {
        free(pathArray);
        return NULL;
    }

    char* saveptr;
    char* token = strtok_r(mutablePath, "/", &saveptr);
    int size = 0;

    while (token != NULL) {
        // Expand pathArray if it gets full
        if (size >= capacity) {
            capacity *= 2;
            char** temp = realloc(pathArray, sizeof(char*) * capacity);
            if (temp == NULL) { // clear pathArray on realloc failure
                for (int i = 0; i < size; i++) {
                    free(pathArray[i]);
                }
                free(pathArray);
                free(mutablePath);
                return NULL;
            }
            pathArray = temp;
        }

        // Parse each individual component of path
        pathArray[size] = strdup(token);
        if (pathArray[size] == NULL) { // Handle memory allocation failure
            for (int i = 0; i < size; i++) {
                free(pathArray[i]);
            }
            free(pathArray);
            free(mutablePath);
            return NULL;
        }
        size++;
        
        token = strtok_r(NULL, "/", &saveptr);
    }

    int* indexToKeep = malloc(sizeof(int) * size);
    if (indexToKeep == NULL) { // Handle memory allocation failure
        for (int i = 0; i < size; i++) {
            free(pathArray[i]);
        }
        free(pathArray);
        free(mutablePath);
        return NULL;
    }

    // Determine which index of path arguments we should keep
    int index = 0;
    for (int i = 0; i < size; i++) {
        if (strcmp(pathArray[i], ".") == 0) {
            // Do nothing for current directory
        } else if (strcmp(pathArray[i], "..") == 0) {
            if (index > 0) {
                index--;
            }
        } else {
            indexToKeep[index++] = i;
        }
    }

    // Create return string buffer
    int returnLength = 2;
    for (int i = 0; i < index; i++) {
        returnLength += strlen(pathArray[indexToKeep[i]]) + 1;
    }

    char* returnString = malloc(returnLength);
    if (returnString == NULL) { // Handle memory allocation failure
        for (int i = 0; i < size; i++) {
            free(pathArray[i]);
        }
        free(pathArray);
        free(indexToKeep);
        free(mutablePath);
        return NULL;
    }

    // Merge the individual path arguments to form the sanitized absolute path
    strcpy(returnString, "/");
    for (int i = 0; i < index; i++) {
        strcat(returnString, pathArray[indexToKeep[i]]);
        if (i < index - 1) {
            strcat(returnString, "/");
        }
    }
    
    // Append "/" to end of path
    if (strcmp(returnString, "/") != 0) {
        strcat(returnString, "/");
    }

    // Release resources
    for (int i = 0; i < size; i++) {
        free(pathArray[i]);
    }
    free(pathArray);
    free(indexToKeep);
    free(mutablePath);
    
    return returnString;
}

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
        freeDirectory(temp);
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
        return -2;
    }
    
    for (int i = 0; i < directory->size / sizeof(DirectoryEntry); i++) {
        if (directory[i].location == -1) {
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

    // Make a mutable copy of the pathname (discards const for warning issue)
    // Strict enforcement for strtok_r to not alter path
    char* mutablePath = strdup(path);
    if (mutablePath == NULL) {
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
        free(mutablePath);
        return -1;
    }

    char* saveptr;
    char* token1 = strtok_r(mutablePath, "/", &saveptr);
    if (token1 == NULL) { // Special case "/"
        ppi->parent = parent;
        ppi->lastElement = NULL;
        ppi->lastElementIndex = -2; // special sentinel
        free(mutablePath);
        freeDirectory(currentDir);
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
            free(mutablePath);
            freeDirectory(currentDir);
            return -1;
        }

        if (currentDir[ppi->lastElementIndex].isDirectory != 'd') {
            free(mutablePath);
            freeDirectory(currentDir);
            return -1;
        }

        DirectoryEntry* temp = loadDir(&(currentDir[ppi->lastElementIndex]));
        if (temp == NULL) {
            free(mutablePath);
            freeDirectory(currentDir);
            return -1;
        }
        freeDirectory(currentDir);
        currentDir = temp;
        token1 = token2;
    } while (token2 != NULL);

    return 0;
}

/**
 * Updates the FAT to release the file location to the free space so it can be used by a different file
 * 
 * @param ppi the parse path information 
 * @return 0 on success and -1 on failure.
*/
int deleteBlob(ppInfo ppi) {
    if (ppi.lastElementIndex == -1) {
        return -1;
    }
    int locationOfFile = ppi.parent[ppi.lastElementIndex].location;
    int sizeOfFile = (ppi.parent[ppi.lastElementIndex].size + vcb->blockSize - 1) / vcb->blockSize;

    // Special case to determine how to clear sentinel value
    if (sizeOfFile == 1) {
        // Clearing the sentinel value
        FAT[locationOfFile] = vcb->firstFreeBlock;
        vcb->firstFreeBlock = locationOfFile;
    } else if (sizeOfFile > 1) {
        // Traverse the FAT until it reaches the sentinel value
        int endOfFileIndex = seekBlock(sizeOfFile - 1, locationOfFile);
        if (endOfFileIndex < 0) {
            return -1;
        }
        // Clearing the sentinel value
        FAT[endOfFileIndex] = vcb->firstFreeBlock;
        vcb->firstFreeBlock = locationOfFile;
    }
    // Write updated FAT to disk
    if (writeBlock(FAT, vcb->freeSpaceSize, vcb->freeSpaceLocation) == -1) {
        return -1;
    }
    
    return 0;
}

/**
 * Creates a new file.
 * 
 * @param ppi parse pass info needed to create the new file
 * 
 * @return -1 on fail 0 on sucess
*/
int createFile(ppInfo* ppi) {
    time_t currentTime = time(NULL);

    // find first available DE or make the directory bigger 
    int vacantDE = findUnusedDE(ppi->parent);
    if (vacantDE == -1) {
		ppi->parent = expandDirectory(ppi->parent);
		vacantDE = findUnusedDE(ppi->parent);
	}
    if (vacantDE < 0) {
        return -1;
    }

    // init all of the metadata
    ppi->parent[vacantDE].dateCreated = currentTime;
    ppi->parent[vacantDE].dateModified = currentTime;
    ppi->parent[vacantDE].isDirectory = ' ';
    ppi->parent[vacantDE].location = -2;
    strncpy(ppi->parent[vacantDE].name, ppi->lastElement, sizeof(ppi->parent[vacantDE].name));
    ppi->parent[vacantDE].size = 0;
    ppi->lastElementIndex = vacantDE;
    
    // save data to the disk
    int blocksToWrite = (ppi->parent->size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppi->parent, blocksToWrite, ppi->parent->location) == -1) {
        return -1;
    }

    return 0;
}

/**
 * ensure that the users memory is updated so all files on disk are represented 
 * 
 * @param ppi parse path info for the newly updated directory
 */
void updateWorkingDir(ppInfo ppi) {
    if (ppi.parent->location == root->location) {
        root = ppi.parent;
    }
    if (ppi.parent->location == cwd->location) {
        cwd = ppi.parent;
    }
}