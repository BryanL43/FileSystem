#include "mfs.h"
#include "fsDesign.h"
#include "directory.h"
#include "fsUtils.h"

#define ROOT -2

int fs_setcwd(char *pathname) {
    ppInfo ppi;

    int ret = parsePath(pathname, &ppi);
    if (ret == -1 || ppi.lastElementIndex == -1) { // parsePath returned error
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

    DirectoryEntry* temp = loadDir(&(ppi.parent[ppi.lastElementIndex]));
    if (temp == NULL) {
        return -1;
    }
    memcpy(cwd, temp, temp->size);
    // freeDirectory(ppi.parent)
    
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

    writeBlock(cwd, (cwd->size + vcb->blockSize - 1) / vcb->blockSize, cwd->location);
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

    DirectoryEntry* temp = loadDir(ppi.parent);
    ppi.parent = temp;
    
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

    int index = findNameInDir(ppi.parent, ppi.lastElement);
    if (index == -1) {
        return -1;
    }

    buf->st_size = ppi.parent[index].size;
    buf->st_blksize = vcb->blockSize;
    buf->st_blocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
    buf->st_createtime = ppi.parent->dateCreated;
    buf->st_modtime = ppi.parent->dateModified;


    freeDirectory(ppi.parent);
    return 0;
}

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

    // Creating an empty buffer to overwrite contents in disk
    char *emptyBlocks = malloc(sizeOfDir * vcb->blockSize);
    if (emptyBlocks == NULL) {
        free(emptyBlocks);
        free(path);
        return -1;
    }

    // Load directory for empty check
    DirectoryEntry* directoryToDelete = loadDir(&(ppi.parent[ppi.lastElementIndex]));
    if(isDirEmpty(directoryToDelete) == -1) {
        // When the directory is not empty, return error
        free(emptyBlocks);
        free(path);
        return -1;
    }

    // Deleting dir in disk
    if (writeBlock(emptyBlocks, sizeOfDir, locationOfDir) == -1) {
        free(emptyBlocks);
        free(path);
        return -1;
    }

    // Special case to determine how to clear sentinel value
    if (sizeOfDir == 1) {
        // Clearing the sentinel value
        FAT[locationOfDir] = vcb->firstFreeBlock;
        vcb->firstFreeBlock = locationOfDir;
    } else {
        // Traverse the FAT until it reaches the sentinel value
        int endOfDirIndex = seekBlock(sizeOfDir, locationOfDir);
        if (endOfDirIndex < 0) {
            free(emptyBlocks);
            free(path);
            return -1;
        }
        // Clearing the sentinel value
        FAT[endOfDirIndex] = vcb->firstFreeBlock;
        vcb->firstFreeBlock = locationOfDir;
    }
    
    // Write updated FAT to disk
    if (writeBlock(FAT, vcb->freeSpaceSize, vcb->freeSpaceLocation) == -1) {
        free(emptyBlocks);
        free(path);
        return -1;
    }

    // Mark the deleted directory as unused
    ppi.parent[index].location = -1;
    
    // Clearing the name and isDirectory in the parent directory
    // for hexdump to easily show empty directories
    memcpy(ppi.parent[index].name, emptyBlocks, sizeof(ppi.parent[index].name));
    ppi.parent[index].isDirectory = '\0';

    // Write the new parent directory with deleted directory into disk
    int sizeOfDirectory = (ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppi.parent, sizeOfDirectory, ppi.parent[0].location) == -1) {
        free(emptyBlocks);
        free(path);
        return -1;
    }

    // Freeing buffers
    freeDirectory(ppi.parent);
    free(emptyBlocks);
    free(path);
    return 0;
}

int fs_move(char *srcPathName, char* destPathName) {
    ppInfo ppiSrc;
    ppInfo ppiDest;

    char* srcPath = strdup(srcPathName);
    if (srcPath == NULL) {
        return -1;
    }
    char* destPath = strdup(destPathName);
    if (destPath == NULL) {
        return -1;
    }

    parsePath(srcPath, &ppiSrc);
    if (ppiSrc.lastElementIndex == -1) {
        return -1;
    }

    if (fs_mkdir(destPath, 0777) == -1) {
        return -1;
    }

    if (parsePath(destPath, &ppiDest) == -1) {
        return -1;
    }

    int srcElementIndex = ppiSrc.lastElementIndex;
    int destElementIndex = ppiDest.lastElementIndex;

    ppiDest.parent[destElementIndex].dateCreated = ppiSrc.parent[ppiSrc.lastElementIndex].dateCreated;
    ppiDest.parent[destElementIndex].dateModified = ppiSrc.parent[ppiSrc.lastElementIndex].dateModified;
    ppiDest.parent[destElementIndex].isDirectory = ppiSrc.parent[ppiSrc.lastElementIndex].isDirectory;
    strcpy(ppiDest.parent[destElementIndex].name, ppiSrc.parent[ppiSrc.lastElementIndex].name);
    ppiDest.parent[destElementIndex].size = ppiSrc.parent[ppiSrc.lastElementIndex].size;

    // Write new DE to disk
    int sizeOfDirectory = (ppiDest.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(ppiDest.parent, sizeOfDirectory, ppiDest.parent[0].location) == -1) {
        return -1;
    }

    // Write the actual file to disk
    DirectoryEntry* destDirectory = loadDir(&(ppiDest.parent[ppiDest.lastElementIndex]));
    if (destDirectory == NULL) {
        return -1;
    }

    int location = ppiDest.parent[destElementIndex].location;
    int size = (ppiDest.parent[destElementIndex].size + vcb->blockSize - 1) / vcb->blockSize;
    if (writeBlock(destDirectory, size, location) == -1) {
        return -1;
    }

    // Remove src DE
    if (fs_isDir) {
        if (fs_rmdir(srcPath) == -1) {
            return -1;
        }
    } else if (fs_isFile) {
        if (fs_delete(srcPath) == -1) {
            return -1;
        }
    }
    return 0;
}