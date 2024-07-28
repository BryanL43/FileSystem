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

    // Loads the directory into memory
    DirectoryEntry* temp = loadDir(&(ppi.parent[ppi.lastElementIndex]));
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

    return 0;
}

char* fs_getcwd(char *pathname, size_t size) {
    strncpy(pathname, cwdPathName, size);
    return cwdPathName;
}

int fs_mkdir(const char *pathname, mode_t mode) {
    ppInfo ppi;

    // Make a mutable copy of the pathname (discards const for warning issue)
    char* mutablePath = strdup(pathname);
    if (mutablePath == NULL) {
        return -1;
    }

    if (parsePath(mutablePath, &ppi) == -1) { // Error case
        free(mutablePath);
        return -1;
    }
    
    int vacantDE = findUnusedDE(ppi.parent);
    if (vacantDE == -1) {
        ppi.parent = expandDirectory(ppi.parent);
        vacantDE = findUnusedDE(ppi.parent);
    }

    if (vacantDE < 0) {
        free(mutablePath);
        freeDirectory(ppi.parent);
        return -1;
    }  

    if (ppi.lastElementIndex != -1) { // directory already exists
        free(mutablePath);
        freeDirectory(ppi.parent);
        return -2;
    }

    DirectoryEntry* newDir = initDirectory(DEFAULT_DIR_SIZE, ppi.parent);
    if (newDir == NULL) {
        free(mutablePath);
        freeDirectory(ppi.parent);
        return -1;
    }
    
    memcpy(&(ppi.parent[vacantDE]), newDir, sizeof(DirectoryEntry));
    strncpy(ppi.parent[vacantDE].name, ppi.lastElement, sizeof(ppi.parent->name));

    writeBlock(ppi.parent, (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize, ppi.parent->location);

    updateWorkingDir(ppi);

    freeDirectory(ppi.parent);
    freeDirectory(newDir);
    free(mutablePath);

    return 0;
}
/**
 *  fs_stat() attempts assign buffe's variable size, blocksize,
 *  blocks, creatime, and modtime 
 * 
 * @param path the path of the file in question
 * @param buf 
 * @return 0 on success. On error, -1 is returned.
 */
int fs_stat(const char *path, struct fs_stat *buf) {
    ppInfo ppi;

    // Make a mutable copy of the pathname (discards const for warning issue)
    char* mutablePath = strdup(path);
    if (mutablePath == NULL) {
        return -1;
    }
    
    int ret = parsePath(mutablePath, &ppi);
    free(mutablePath);
    if (ret == -1) {
        return -1;
    }

    buf->st_size = ppi.parent[ppi.lastElementIndex].size;
    buf->st_blksize = vcb->blockSize;
    buf->st_blocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
    buf->st_createtime = ppi.parent->dateCreated;
    buf->st_modtime = ppi.parent->dateModified;

    freeDirectory(ppi.parent);
    return 0;
}
/**
 * check if the given path is a directory
 * 
 * @param path the path of the file in question
 * 
 * @return 1 if Directory. On error or not Directory, 0 is returned. 
 */
int fs_isDir(char* path) 
{
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

/**
 * check if the given path is a file
 * 
 * @param path the path of the file in question
 * 
 * @return 1 if the direactory entry at the given path is a file 0 otherwise
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
 * check if the given path is a directory
 * 
 * @param path the path of the directory in question
 * 
 * @return 1 if the direactory entry at the given path is a directory 0 otherwise
 */
fdDir * fs_opendir(const char *pathname){

    ppInfo ppi;
    char* path = strdup(pathname);
    if (path == NULL) {
        return NULL;
    }

    if (parsePath(path, &ppi) != 0) {
        freeDirectory(ppi.parent);
        return NULL;
    }

    fdDir *dirp = malloc(sizeof(dirp));
    if (dirp == NULL) {
        freeDirectory(ppi.parent);
        return NULL;
    }

    if (ppi.lastElementIndex == ROOT)
    {
        ppi.lastElementIndex = 0;
    }
    
    DirectoryEntry * thisDir = loadDir(&(ppi.parent[ppi.lastElementIndex]));
    dirp->directory = thisDir;
    dirp->dirEntryPosition = 0;
    dirp->d_reclen = 0;
    for(int i = 0; i < thisDir->size / sizeof(DirectoryEntry); i++)
        if(dirp->directory[i].location != -1)
            dirp->d_reclen++;
    struct fs_diriteminfo * di = malloc(sizeof(di));
    if (di == NULL) {
        return NULL;
    }

    freeDirectory(ppi.parent);

    dirp->di = di;
    return dirp;

}

int fs_closedir(fdDir *dirp)
{
    freeDirectory(dirp->directory);
    free(dirp->di);
	free(dirp);

    dirp = NULL;
    return 1;
}

/**
 * Get the information about a directory entry 
 * 
 * @param dirp A sort of private file descriptor to keep track of what directory entries are in the current directory
 * 
 * @return fs_diriteminfo with metadata about each file in the directory
 */
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    if (dirp == NULL) 
        return NULL;

    // Past the end of the number of directories
    if (dirp->dirEntryPosition >= dirp->directory->size / sizeof(DirectoryEntry)) {
        return NULL;
    }

    unsigned short pos = dirp->dirEntryPosition;

    // Find the next valid directory entry
    while (dirp->directory[pos].location < 0 && dirp->directory[pos].location != -2) {
        pos++;
    }

    // Populate the fs_diriteminfo structure
    dirp->di->d_reclen = (dirp->directory[pos].size + sizeof(DirectoryEntry) - 1) 
                        / sizeof(DirectoryEntry);
    
    dirp->di->fileType = dirp->directory[pos].isDirectory;
    strcpy(dirp->di->d_name, dirp->directory[pos].name);

    // Move to the next directory entry
    dirp->dirEntryPosition = 1 + pos;


    // Past the end of the number of directories
    if (dirp->dirEntryPosition >= dirp->directory->size / sizeof(DirectoryEntry)) {
        return NULL;
    }
    return dirp->di;
}

/**
 * Delete the given file
 * 
 * @param filename the path of the file to delete
 * @return 0 on success or -1 for error
 */
int fs_delete(char* filename) {
    ppInfo ppi;
    // Make a mutable copy of the pathname (discards const for warning issue)
    char* path = strdup(filename);
    if (path == NULL) {
        return -1;
    }
    // Populate ppInfo
    if (parsePath(path, &ppi) == -1) {
        free(path);
        return -1;
    }

    // Deleting the file without touching directory
    if(deleteBlob(ppi) == -1)
    {
        free(path);
        return -1;
    }
    
    // Loading ppInfo into variables
    int index = ppi.lastElementIndex;

    // Mark the deleted file as unused
    ppi.parent[index].location = -1;

    // Write the new parent directory with deleted file into disk
    int sizeOfParentDir = (ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppi.parent, sizeOfParentDir, ppi.parent[0].location) == -1) {
        free(path);
        return -1;
    }

    // Freeing buffers
    freeDirectory(ppi.parent);
    free(path);
    return 0;
}


/**
 * Delete the given directory
 * 
 * @param pathname the path of the directory to delete
 * @return 0 on success or -1 for error
 */
int fs_rmdir(const char *pathname) {
    ppInfo ppi;

    // Make a mutable copy of the pathname (discards const for warning issue)
    char* path = strdup(pathname);
    if (path == NULL) {
        return -1;
    }

    // Populate ppInfo
    if (parsePath(path, &ppi) == -1) {
        free(path);
        return -1;
    }

    // Loading ppInfo into variables
    int index = ppi.lastElementIndex;
    int locationOfDir = ppi.parent[index].location;
    int sizeOfDir = (ppi.parent[index].size + vcb->blockSize - 1) / vcb->blockSize;
    if (index < 0 || locationOfDir < 0 || sizeOfDir < 0) {
        free(path);
        return -1;
    }

    // Load directory for empty check
    DirectoryEntry* directoryToDelete = loadDir(&(ppi.parent[ppi.lastElementIndex]));
    if(isDirEmpty(directoryToDelete) == -1) {
        // When the directory is not empty, return error
        free(path);
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
            free(path);
            return -1;
        }
        // Clearing the sentinel value
        FAT[endOfDirIndex] = vcb->firstFreeBlock;
        vcb->firstFreeBlock = locationOfDir;
    }
    
    // Write updated FAT to disk
    if (writeBlock(FAT, vcb->freeSpaceSize, vcb->freeSpaceLocation) == -1) {
        free(path);
        return -1;
    }

    // Mark the deleted directory as unused
    ppi.parent[index].location = -1;

    // Write the new parent directory with deleted directory into disk
    int sizeOfDirectory = (ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppi.parent, sizeOfDirectory, ppi.parent[0].location) == -1) {
        free(path);
        return -1;
    }

    // Freeing buffers
    freeDirectory(ppi.parent);
    free(path);
    return 0;
}

/**
 * Move a file or directory from source directory to destination directory
 * 
 * @param srcPathName the path of the source file/directory
 * @param destPathName the path of where to move the file/directory
 * @return 0 on success or -1 for error
 */
int fs_move(char *srcPathName, char* destPathName) {
    time_t currentTime = time(NULL);
    ppInfo ppiSrc;
    ppInfo ppiDest;

    // Make a mutable copy of the pathname (discards const for warning issue)
    char* srcPath = strdup(srcPathName);
    if (srcPath == NULL) {
        return -1;
    }
    char* destPath = strdup(destPathName);
    if (destPath == NULL) {
        return -1;
    }

    // Get PPI of both SRC and DEST paths
    if (parsePath(srcPath, &ppiSrc) == -1) {
        return -1;
    }

    if (parsePath(destPath, &ppiDest) == -1) {
        return -1;
    }

    // Get a free entry in DEST directory
    int vacantDE = findUnusedDE(ppiDest.parent);
    if (vacantDE == -1) {
		ppiDest.parent = expandDirectory(ppiDest.parent);
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

    if (ppiDest.parent->location == ppiSrc.parent->location) { // When moving in the sam directory

        // Set DEST directory of SRC file
        ppiDest.parent[srcElementIndex].size = 0;
        ppiDest.parent[srcElementIndex].location = -1;
        if (writeBlock(ppiDest.parent, sizeOfDestDirectory, ppiDest.parent->location) == -1) {
            return -1;
        }
    } else { // Normal operation when moving in different directories
        if (writeBlock(ppiDest.parent, sizeOfDestDirectory, ppiDest.parent->location) == -1) {
            return -1;
        }

        if (writeBlock(ppiSrc.parent, sizeOfSrcDirectory, ppiSrc.parent->location) == -1) {
            return -1;
        }
    }   
    updateWorkingDir(ppiDest);
    updateWorkingDir(ppiSrc);

    return 0;
}