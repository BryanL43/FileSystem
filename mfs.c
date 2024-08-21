/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: mfs.c
*
* Description:: File system directory operation methods that handles the following
* shell commands: ls, mv, md, rm, cd, and pwd.
*
**************************************************************/

#include "mfs.h"
#include "fsDesign.h"
#include "directory.h"
#include "fsUtils.h"

#define ROOT -2

/**
 * Sets the current working directory to the specified directory.
 * 
 * @param pathname the specified path.
 * @return 0 on success or -1 on failure.
*/
int fs_setcwd(char *pathname) {
    ppInfo ppi;

    // Parse path to acquire information about the directory
    int ret = parsePath(pathname, &ppi);
    if (ret == -1 || ppi.lastElementIndex == -1 || ppi.parent[ppi.lastElementIndex].isDirectory == ' ') {
        return -1;
    }

    if (ppi.lastElementIndex == -2) { // Special "root" case
        cwd = loadDir(&root[0]);
        if (cwd == NULL) {
            return -1;
        }
        strcpy(cwdPathName, "/");
        return 0;
    }

    // Handle ".." case (moving to parent directory)
    DirectoryEntry* temp;
    if (strcmp(ppi.parent[ppi.lastElementIndex].name, "..") == 0) {
        temp = loadDir(&(ppi.parent[ppi.lastElementIndex - 1])); // Load the grandparent
    } else { // Normal case, moving to a non-root parent directory
        temp = loadDir(&(ppi.parent[ppi.lastElementIndex]));
    }
    if (temp == NULL) {
        return -1;
    }
    memcpy(cwd, temp, temp->size);
    
    // Determine the new path name
    char* newPath;
    if (pathname[0] == '/') { // Absolute path
        newPath = strdup(pathname);
        if (newPath == NULL) {
            fprintf(stderr, "Fatal: insufficient memory for strdup!\n");
            return -1;
        }
    } else { // Relative path
        int newPathLen = strlen(cwdPathName) + strlen(pathname) + 2;
        newPath = malloc(newPathLen);
        if (newPath == NULL) {
            fprintf(stderr, "Fatal: insufficient memory for newPath malloc!\n");
            return -1;
        }
        strcpy(newPath, cwdPathName);

        // Append "/" to end of path
        strcat(newPath, pathname);
        if (newPath[strlen(cwdPathName)] != '/') {
            strcat(newPath, "/");
        }
    }

    cwdPathName = normalizePath(newPath);
    free(newPath);

    // Write the current working directory to disk
    int numOfBlocks = (cwd->size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(cwd, numOfBlocks, cwd->location) == -1) {
        freeDirectory(temp);
        return -1;
    }

    freeDirectory(temp);
    updateWorkingDir(ppi);
    return 0;
}

/**
 * Gets the current working directory.
 * 
 * @param pathname the specified path.
 * @param size the byte size of the path.
 * @return cwdPathName the current working directory path name.
*/
char* fs_getcwd(char *pathname, size_t size) {
    strncpy(pathname, cwdPathName, size);
    return cwdPathName;
}

/**
 * Creates a directory.
 * 
 * @param path the specified path.
 * @param mode file bit mask (not in use).
 * @return 0 on success. On error, -1 is returned.
 */
int fs_mkdir(const char *pathname, mode_t mode) {
    ppInfo ppi;

    // Make a mutable copy of the pathname (discards const warning issue)
    char* mutablePath = strdup(pathname);
    if (mutablePath == NULL) {
        return -1;
    }

    // Parse the path
    if (parsePath(mutablePath, &ppi) == -1) {
        free(mutablePath);
        return -1;
    }
    free(mutablePath);

    DirectoryEntry* temp = loadDir(ppi.parent);
    if (temp == NULL) {
        return -1;
    }
    ppi.parent = temp;

    // Find unused directory entry
    int vacantDE = findUnusedDE(ppi.parent);
    if (vacantDE == -1) {
        // Acquire parent directory
        char* result = secondToLastElement(pathname);
        int location = findNameInDir(ppi.previousDir, result);
        
        ppi.parent = expandDirectory(ppi.parent, ppi.previousDir, &ppi.previousDir[location]);
        if (ppi.parent == NULL) {
            freeDirectory(temp);
            return -1;
        }
        vacantDE = findUnusedDE(ppi.parent);
    }

    if (vacantDE < 0) {
        freeDirectory(ppi.parent);
        return -1;
    }

    if (ppi.lastElementIndex != -1) { // directory already exists
        freeDirectory(ppi.parent);
        return -2;
    }
    
    // Instantiate an directory instance
    DirectoryEntry* newDir = initDirectory(DEFAULT_DIR_SIZE, ppi.parent);
    if (newDir == NULL) {
        freeDirectory(ppi.parent);
        return -1;
    }
    
    // Copies the new instantiated directory to its specified destination
    memcpy(&(ppi.parent[vacantDE]), newDir, sizeof(DirectoryEntry));
    strncpy(ppi.parent[vacantDE].name, ppi.lastElement, sizeof(ppi.parent->name));
    
    // Write the new directory to disk
    int parentNumOfBlocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppi.parent, parentNumOfBlocks, ppi.parent->location) == -1) {
        free(newDir);
        freeDirectory(ppi.parent);
    }

    updateWorkingDir(ppi);

    freeDirectory(ppi.parent);
    freeDirectory(newDir);

    return 0;
}

/**
 * Returns information about a entry, in the fs_stat buffer.
 * 
 * @param path the specified path.
 * @param buf file information buffer.
 * @return 0 on success. On error, -1 is returned.
 */
int fs_stat(const char *path, struct fs_stat *buf) {
    ppInfo ppi;

    // Make a mutable copy of the pathname (discards const warning issue)
    char* mutablePath = strdup(path);
    if (mutablePath == NULL) {
        return -1;
    }
    
    // Parse the path
    int ret = parsePath(mutablePath, &ppi);
    free(mutablePath);
    if (ret == -1 || ppi.lastElementIndex == -1) {
        return -1;
    }

    // Populate the fs_stat buffer
    buf->st_size = ppi.parent[ppi.lastElementIndex].size;
    buf->st_blksize = vcb->blockSize;
    buf->st_blocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
    buf->st_createtime = ppi.parent->dateCreated;
    buf->st_modtime = ppi.parent->dateModified;

    freeDirectory(ppi.parent);
    return 0;
}

/**
 * Checks if the given path is a directory.
 * 
 * @param path the specified path.
 * @return 1 if Directory. On error or not Directory, 0 is returned. 
 */
int fs_isDir(char* path) {
    ppInfo ppi;

    if (parsePath(path, &ppi) != 0) {
        freeDirectory(ppi.parent);
        return 0;
    }

    if(ppi.parent[ppi.lastElementIndex].isDirectory == 'd') {
        freeDirectory(ppi.parent);
        return 1;
    }

    freeDirectory(ppi.parent);
    return 0;
}
// Warning: Since 'ls' calls fs_isDir with 1st a path then the individual name,
// there isn't a way to get the 'ls' to accurate display information when calling
// through a path. i.e. ls -la /car/car1
// Unless the fs_shell is reconstructed, then the aforementioned issue will be resolved.

/**
 * Checks if the given path is a file.
 * 
 * @param path the specified path.
 * @return 1 if the directory entry at the given path is a file, 0 otherwise.
 */
int fs_isFile(char* path) {
    ppInfo ppi;

    if (parsePath(path, &ppi) != 0) {
        freeDirectory(ppi.parent);
        return 0;
    }

    if(ppi.parent[ppi.lastElementIndex].isDirectory != 'd') {
        freeDirectory(ppi.parent);
        return 1;
    }

    freeDirectory(ppi.parent);
    return 0;
}

/**
 * Open the directory and populates fdDir structure 
 * for the uses of fs_readdir.
 * 
 * @param path the path of the directory in question.
 * @return fdDir structure
 */
fdDir * fs_opendir(const char *pathname) {
    ppInfo ppi;   

    // Make a mutable copy of the pathname (discards const warning issue)
    char* path = strdup(pathname);
    if (path == NULL) {
        return NULL;
    }

    // Traverse the path
    if (parsePath(path, &ppi) != 0) {
        free(path);
        freeDirectory(ppi.parent);
        return NULL;
    }
    free(path);

    // Load the parent directory into memory
    DirectoryEntry* temp = loadDir(ppi.parent);
    if (temp == NULL) {
        freeDirectory(ppi.parent);
    }
    ppi.parent = temp;

    // Special root case where index should refer to "." entry
    if (ppi.lastElementIndex == ROOT) {
        ppi.lastElementIndex = 0;
    }

    DirectoryEntry entry = ppi.parent[ppi.lastElementIndex];

    // Instantiate fdDir structure
    fdDir* dirp = malloc(sizeof(fdDir));
    if (dirp == NULL) {
        freeDirectory(ppi.parent);
        return NULL;
    }

    // Populate fdDir structure with information regarding the directory to open
    dirp->d_reclen = (entry.size + vcb->blockSize - 1) / vcb->blockSize;
    dirp->dirEntryPosition = ppi.lastElementIndex;
    dirp->dirEntryLocation = entry.location;
    dirp->index = 0;

    // Instantiate the directory item information structure
    dirp->di = malloc(sizeof(struct fs_diriteminfo));
    if (dirp->di == NULL) {
        free(dirp);
        freeDirectory(ppi.parent);
        return NULL;
    }

    // Populate the directory item information structure with entry information
    dirp->di->d_reclen = (entry.size + vcb->blockSize - 1) / vcb->blockSize;
    dirp->di->fileType = entry.isDirectory;

    // Copy the appropriate directory's name
    if (ppi.lastElement == NULL) {
        strcpy(dirp->di->d_name, ".");
    } else {
        strcpy(dirp->di->d_name, ppi.lastElement);
    }
    
    freeDirectory(ppi.parent);
    return dirp;
}

/**
 * Closes a directory.
 * 
 * @param dirp the directory's descriptor.
 * @return 1 on success or 0 on failure.
*/
int fs_closedir(fdDir *dirp)
{
    if (dirp == NULL) {
        return 0;
    }

    free(dirp->di);
	free(dirp);

    dirp = NULL;
    return 1;
}

/**
 * Get the information about a directory entry.
 * 
 * @param dirp A sort of private file descriptor to keep track of what
 *             directory entries are in the current directory.
 * @return fs_diriteminfo with metadata about each file in the directory or NULL if error.
 */
struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    // Instantiate the directory that is opened
    DirectoryEntry* entries = malloc(dirp->d_reclen * vcb->blockSize);
    if (entries == NULL) {
        return NULL;
    }
    
    // Load the directory into memory
    if (readBlock(entries, dirp->d_reclen, dirp->dirEntryLocation) == -1) {
        free(entries);
        return NULL;
    }

    // Iterate through the entries and find the next valid directory entry
    DirectoryEntry entry = entries[dirp->index];
    int numOfPredictedEntries = (dirp->d_reclen * vcb->blockSize) / sizeof(struct DirectoryEntry);
    while (dirp->index < numOfPredictedEntries && entry.location == -1) {
        entry = entries[dirp->index++];
    }

    // Check if we have iterated through the entire entry
    if (dirp->index == numOfPredictedEntries) {
        free(entries);
        return NULL;
    }

    // Populate directory item information structure with the entry's information
    dirp->di->d_reclen = dirp->d_reclen;
    dirp->di->fileType = entry.isDirectory;
    strcpy(dirp->di->d_name, entry.name);
    dirp->index++;

    free(entries);
    return dirp->di;
}

/**
 * Delete the given file.
 * 
 * @param filename the path of the file to delete.
 * @return 0 on success or -1 for error.
 */
int fs_delete(char* filename) {
    ppInfo ppi;

    // Make a mutable copy of the filename (discards const warning issue)
    char* path = strdup(filename);
    if (path == NULL) {
        return -1;
    }

    // Populate ppInfo
    if (parsePath(path, &ppi) == -1) {
        free(path);
        return -1;
    }
    free(path);

    // Deleting the file without touching directory
    if (deleteBlob(ppi) == -1) {
        return -1;
    }
    
    // Loading ppInfo into variables
    int index = ppi.lastElementIndex;

    // Mark the deleted file as unused
    ppi.parent[index].location = -1;

    // Write the new parent directory with deleted file into disk
    int sizeOfParentDir = (ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppi.parent, sizeOfParentDir, ppi.parent[0].location) == -1) {
        return -1;
    }

    freeDirectory(ppi.parent);
    return 0;
}


/**
 * Delete the given directory.
 * 
 * @param pathname the path of the directory to delete.
 * @return 0 on success or -1 for error.
 */
int fs_rmdir(const char *pathname) {
    ppInfo ppi;

    // Make a mutable copy of the pathname (discards const warning issue)
    char* path = strdup(pathname);
    if (path == NULL) {
        return -1;
    }

    // Populate ppInfo
    if (parsePath(path, &ppi) == -1) {
        free(path);
        return -1;
    }
    free(path);

    // Loading ppInfo into variables
    int index = ppi.lastElementIndex;
    int locationOfDir = ppi.parent[index].location;
    int sizeOfDir = (ppi.parent[index].size + vcb->blockSize - 1) / vcb->blockSize;
    if (index < 0 || locationOfDir < 0 || sizeOfDir < 0) {
        freeDirectory(ppi.parent);
        return -1;
    }

    // Load directory for empty check
    DirectoryEntry* directoryToDelete = loadDir(&(ppi.parent[ppi.lastElementIndex]));
    if (directoryToDelete == NULL) {
        freeDirectory(ppi.parent);
    }

    if(isDirEmpty(directoryToDelete) == -1) {
        // When the directory is not empty, return error
        printf("Failed to remove directory! Directory is not empty.\n");
        free(directoryToDelete);
        freeDirectory(ppi.parent);
        return -1;
    }

    // Special case to determine how to clear sentinel value
    if (sizeOfDir == 1) {
        // Clearing the sentinel value
        FAT[locationOfDir] = vcb->firstFreeBlock;
        vcb->firstFreeBlock = locationOfDir;
    } else  if (sizeOfDir > 1) {
        // Traverse the FAT until it reaches the sentinel value
        int endOfDirIndex = seekBlock(sizeOfDir - 1, locationOfDir);
        if (endOfDirIndex < 0) {
            free(directoryToDelete);
            freeDirectory(ppi.parent);
            return -1;
        }
        // Clearing the sentinel value
        FAT[endOfDirIndex] = vcb->firstFreeBlock;
        vcb->firstFreeBlock = locationOfDir;
    }
    
    // Write updated FAT to disk
    if (writeBlock(FAT, vcb->freeSpaceSize, vcb->freeSpaceLocation) == -1) {
        free(directoryToDelete);
        freeDirectory(ppi.parent);
        return -1;
    }

    // Mark the deleted directory as unused
    ppi.parent[index].location = -1;

    // Write the new parent directory with deleted directory into disk
    int sizeOfDirectory = (ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppi.parent, sizeOfDirectory, ppi.parent[0].location) == -1) {
        free(directoryToDelete);
        freeDirectory(ppi.parent);
        return -1;
    }

    free(directoryToDelete);
    freeDirectory(ppi.parent);
    return 0;
}

/**
 * Move a file or directory from source directory to destination directory.
 * 
 * @param srcPathName the path of the source file/directory.
 * @param destPathName the path of where to move the file/directory.
 * @return 0 on success or -1 for error.
 */
int fs_move(char *srcPathName, char* destPathName) {
    time_t currentTime = time(NULL);
    ppInfo ppiSrc;
    ppInfo ppiDest;

    // Make a mutable copy of the pathname (discards const warning issue)
    char* srcPath = strdup(srcPathName);
    if (srcPath == NULL) {
        return -1;
    }
    char* destPath = strdup(destPathName);
    if (destPath == NULL) {
        free(srcPath);
        return -1;
    }

    // Get PPI of both SRC and DEST paths
    if (parsePath(srcPath, &ppiSrc) == -1) {
        free(srcPath);
        free(destPath);
        return -1;
    }
    free(srcPath);

    if (parsePath(destPath, &ppiDest) == -1) {
        free(srcPath);
        free(destPath);
        freeDirectory(ppiSrc.parent);
        return -1;
    }
    free(destPath);

    // Get a free entry in DEST directory
    int vacantDE = findUnusedDE(ppiDest.parent);
    if (vacantDE == -1) {
        // Acquire parent directory
        char* result = secondToLastElement(destPathName);
        int location = findNameInDir(ppiDest.previousDir, result);

		ppiDest.parent = expandDirectory(ppiDest.parent, ppiDest.previousDir, &ppiDest.previousDir[location]);
        if (ppiDest.parent == NULL) {
            freeDirectory(ppiSrc.parent);
            return -1;
        }
		vacantDE = findUnusedDE(ppiDest.parent);
	}

    // Store corresponding index to access PPI
    int srcElementIndex = ppiSrc.lastElementIndex;
    int destElementIndex = vacantDE;

    // Copying SRC entry into DEST entry
    char name[sizeof(((DirectoryEntry*)0)->name)];
    memcpy(name, ppiDest.lastElement, sizeof(name));
    memcpy(&ppiDest.parent[vacantDE], &ppiSrc.parent[srcElementIndex], sizeof(DirectoryEntry));
    memcpy(&ppiDest.parent[vacantDE].name, name, sizeof(name));
    ppiDest.parent[srcElementIndex].dateModified = currentTime;

    // Deleting entry in SRC entry
    ppiSrc.parent[srcElementIndex].size = 0;
    ppiSrc.parent[srcElementIndex].location = -1;

    int sizeOfDestDirectory = (ppiDest.parent->size + vcb->blockSize - 1) / vcb->blockSize;
    int sizeOfSrcDirectory = (ppiSrc.parent->size + vcb->blockSize - 1) / vcb->blockSize;

    // When moving in the same directory
    if (ppiDest.parent->location == ppiSrc.parent->location) {

        // Set DEST directory of SRC file
        ppiDest.parent[srcElementIndex].size = 0;
        ppiDest.parent[srcElementIndex].location = -1;
        if (writeBlock(ppiDest.parent, sizeOfDestDirectory, ppiDest.parent->location) == -1) {
            freeDirectory(ppiSrc.parent);
            freeDirectory(ppiDest.parent);
            return -1;
        }
    } else { // Normal operation when moving in different directories
        if (writeBlock(ppiDest.parent, sizeOfDestDirectory, ppiDest.parent->location) == -1) {
            freeDirectory(ppiSrc.parent);
            freeDirectory(ppiDest.parent);
            return -1;
        }

        if (writeBlock(ppiSrc.parent, sizeOfSrcDirectory, ppiSrc.parent->location) == -1) {
            freeDirectory(ppiSrc.parent);
            freeDirectory(ppiDest.parent);
            return -1;
        }
    }   
    updateWorkingDir(ppiDest);
    updateWorkingDir(ppiSrc);

    freeDirectory(ppiSrc.parent);
    freeDirectory(ppiDest.parent);

    return 0;
}