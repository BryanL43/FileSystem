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
    free(temp);

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

    DirectoryEntry* test = loadDir(ppi.parent);
    ppi.parent = test;
    
    int vacantDE = findUnusedDE(ppi.parent);
    if (vacantDE == -1) {
        ppi.parent = expandDirectory(ppi.parent);
        vacantDE = findUnusedDE(ppi.parent);
    }

    if (vacantDE < 0) {
        freeDirectory(ppi.parent);
        free(mutablePath);
        return -1;
    }  

    if (ppi.lastElementIndex != -1) { // directory already exist
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
    //readBlock(&(ppi.parent[vacantDE]), 1, newDir->location);
    strncpy(ppi.parent[vacantDE].name, ppi.lastElement, sizeof(ppi.parent->name));

    writeBlock(ppi.parent, (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize, ppi.parent->location);


    if (ppi.parent->location == root->location) {
        root = ppi.parent;
    }
    else if (ppi.parent->location == cwd->location) {
        cwd = ppi.parent;
    }
    else {
        freeDirectory(ppi.parent);
    }

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
    if (ret == -1) {
        free(mutablePath);
        return -1;
    }

    int index = findNameInDir(ppi.parent, ppi.lastElement);
    if (index == -1) {
        free(mutablePath);
        return -1;
    }

    buf->st_size = ppi.parent[index].size;
    buf->st_blksize = vcb->blockSize;
    buf->st_blocks = (ppi.parent->size + vcb->blockSize - 1) / vcb->blockSize;
    buf->st_createtime = ppi.parent->dateCreated;
    buf->st_modtime = ppi.parent->dateModified;

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
    return !fs_isDir;
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
        return NULL;
    }

    if (ppi.lastElementIndex == ROOT)
    {
        ppi.lastElementIndex = 0;
    }
    
//   DirectoryEntry * thisDir = root ;
//   DirectoryEntry * thisDir = loadDir(&(ppi->parent));
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

int fs_closedir(fdDir *dirp){

    // if(dirp == NULL){
    //     return -1;
    // }
    //	free(dirp->directory);
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
    while (dirp->directory[pos].location < 0) {
        pos++;
    }


    // Populate the fs_diriteminfo structure
    dirp->di->d_reclen = (dirp->directory[pos].size + sizeof(DirectoryEntry) - 1) 
                        / sizeof(DirectoryEntry);
    
    dirp->di->fileType = dirp->directory[pos].isDirectory;
    strcpy(dirp->di->d_name, dirp->directory[pos].name);

    // Move to the next directory entry
    dirp->dirEntryPosition = 1 + pos;

    return dirp->di;
}


// TO DO: ERROR CHECKING AND FREE BUFFERS
int fs_delete(char* filename) {
    ppInfo ppi;

    char* path = strdup(filename);
    if (path == NULL) {
        return -1;
    }
    parsePath(path, &ppi);

    // Storing ppi into variables
    int index = ppi.lastElementIndex;
    int locationOfFile = ppi.parent[index].location;
    int sizeOfFile = (ppi.parent[index].size + vcb->blockSize - 1) / vcb->blockSize;
    char* emptyBlocks = malloc(sizeOfFile * vcb->blockSize);

    // Deleting the file in disk
    writeBlock(emptyBlocks, sizeOfFile, locationOfFile);

    // Update the FAT table and write to disk
    FAT[seekBlock(sizeOfFile, locationOfFile)] = vcb->firstFreeBlock;
    vcb->firstFreeBlock = locationOfFile;

    int bytesNeeded = vcb->totalBlocks * sizeof(int);
    int blocksNeeded = (bytesNeeded + vcb->blockSize - 1) / vcb->blockSize;
    LBAwrite(FAT, blocksNeeded, vcb->freeSpaceLocation);

    // Update new directory with file deleted and write to disk
    ppi.parent[index].location = -1; // mark DE as unused
    int sizeOfDirectory = (ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    writeBlock(ppi.parent, sizeOfDirectory, ppi.parent[0].location);
    freeDirectory(ppi.parent);
}

int fs_rmdir(const char *pathname) {
    ppInfo ppi;
    char* path = strdup(pathname);
    if (path == NULL) {
        return -1;
    }
    parsePath(path, &ppi);

    int index = ppi.lastElementIndex;
    int locationOfDir = ppi.parent[index].location;
    int sizeOfDir = (ppi.parent[index].size + vcb->blockSize - 1) / vcb->blockSize;
    char* emptyBlocks = malloc(sizeOfDir * vcb->blockSize);

    DirectoryEntry* directoryToDelete = loadDir(&(ppi.parent[ppi.lastElementIndex]));
    if(isDirEmpty(directoryToDelete) == -1) {
        // When the directory is not empty, return error
        return -1;
    }

    // Deleting file in disk
    writeBlock(emptyBlocks, sizeOfDir, locationOfDir);

    // Update the FAT table and write to disk
    FAT[seekBlock(sizeOfDir, locationOfDir)] = vcb->firstFreeBlock;
    vcb->firstFreeBlock = locationOfDir;

    int bytesNeeded = vcb->totalBlocks * sizeof(int);
    int blocksNeeded = (bytesNeeded + vcb->blockSize - 1) / vcb->blockSize;
    LBAwrite(FAT, blocksNeeded, vcb->freeSpaceLocation);

    ppi.parent[index].location = -1; // mark DE as unused
    int sizeOfDirectory = (ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize;
    writeBlock(ppi.parent, sizeOfDirectory, ppi.parent[0].location);
    freeDirectory(ppi.parent);
}