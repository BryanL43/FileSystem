#include "mfs.h"
#include "fsDesign.h"
#include "directory.h"
#include "fsUtils.h"

int fs_setcwd(char *pathname) {
    ppInfo* ppi = malloc(sizeof(ppInfo));
    ppi->parent = malloc(sizeof(DirectoryEntry));

    int ret = parsePath(pathname, ppi);
    if (ret == -1 || ppi->lastElementIndex == -1) { // parsePath return error
        free(ppi);
        return -1;
    }
    
    if (ppi->lastElementIndex == -2) { // Special "root" case
        cwd = loadDir(&root[0]);
        strcpy(cwdPathName, "/");
        free(ppi);
        return 0;
    }

    //This is for non root directory [TO-DO]
    DirectoryEntry* temp = loadDir(&(ppi->parent[ppi->lastElementIndex]));
    if (temp == NULL) {
        free(ppi);
        return -1;
    }

    memcpy(cwd, temp, temp->size);
    free(temp);
    if( pathname[0] == '/' ) {
        cwdPathName = strdup(pathname);
    }
    else {
        strcat(cwdPathName, pathname);
    }

    cwdPathName = strdup(pathname);
    printf("CwdPathName: %s\n", cwdPathName);

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

    int ret = parsePath(mutablePath, &ppi);
    if (ret < 0) { // Error case
        free(mutablePath);
        return ret;
    }

    if (ppi.lastElementIndex != -1) { // directory already exist
        free(mutablePath);
        free(ppi.parent);
        return -2;
    }
    
    DirectoryEntry* newDir = initDirectory(DEFAULT_DIR_SIZE, ppi.parent);
    if (newDir == NULL) {
        free(mutablePath);
        free(ppi.parent);
        return -1;
    }

    int vacantDE = findUnusedDE(ppi.parent);
    if (vacantDE == -1) {
        free(ppi.parent);
        free(newDir);
        free(mutablePath);
        return -1;
    }

    memcpy(&(ppi.parent[vacantDE]), newDir, sizeof(DirectoryEntry));
    strncpy(ppi.parent[vacantDE].name, ppi.lastElement, sizeof(ppi.parent->name));

    writeBlock(ppi.parent, ppi.parent->size / vcb->blockSize, ppi.parent->location);

    free(ppi.parent);
    free(newDir);
    free(mutablePath);

    return 0;
}

// int fs_stat(const char *path, struct fs_stat *buf) {
//     ppInfo* ppi = malloc(sizeof(ppInfo));
//     ppi->parent = malloc(sizeof(DirectoryEntry));
    
//     int ret = parsePath(path, ppi);
//     if (ret == -1) {
//         return -1;
//     }


// }

int fs_isDir(char* path) 
{
    ppInfo* ppi = malloc(sizeof(ppInfo));
    if (ppi == NULL) {
        return -1;
    }

    if (parsePath(path, ppi) != 0) {
        free(ppi->parent);
        free(ppi);        
        return 0;
    }

    if(ppi->parent[ppi->lastElementIndex].isDirectory == 'd') {
        free(ppi->parent);
        free(ppi);
        return 1;
    }

    free(ppi->parent);
    free(ppi);

    return 0;
}

int fs_isFile(char* path) {
    return !fs_isDir;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    if(dirp == NULL) 
        return NULL;

    // Past the end of the number of directories
    if(dirp->dirEntryPosition > dirp->d_reclen) {
        return NULL;
    }
    
    unsigned short pos = dirp->dirEntryPosition;

    // Get how many directory entries there are in the directory
    dirp->di->d_reclen = 
        (dirp->directory[pos].size + sizeof(DirectoryEntry) - 1) 
        / sizeof(DirectoryEntry);

    // Determine the file type of the directory entry in the directory
    dirp->di->fileType = dirp->directory[pos].isDirectory;

    // copy name to the 
    strcpy(dirp->di->d_name, dirp->directory[pos].name);
    
    // giving another fs_diriteminfo so go to the next one
    dirp->dirEntryPosition += 1;
    return dirp->di;
}