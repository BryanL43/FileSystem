#include "mfs.h"
#include "fsDesign.h"
#include "directory.h"
#include "fsUtils.h"

int fs_setcwd(char *pathname) {
    ppInfo* ppi = malloc(sizeof(ppInfo));
    ppi->parent = malloc(DirectoryEntry);

    int ret = parsePath(pathname, ppi);
    if (ret == -1 || ppi->lastElementIndex == -1) { // parsePath return error
        free(ppi->parent);
        free(ppi);
        return -1;
    }
    
    if (ppi->lastElementIndex == -2) { // Special "root" case
        cwd = loadDir(&root[0]);
        strcpy(cwdPathName, "/");
        free(ppi->parent);
        free(ppi);
        return 0;
    }

    //This is for non root directory [TO-DO]
    //DirectoryEntry* temp = loadDir(&(ppi->parent[ppi->lastElementIndex]));

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
    free(mutablePath);
    if (ret < 0) { // Error case
        return ret;
    }

    if (ppi.lastElementIndex != -1) { // directory already exist
        return -2;
    }
    
    DirectoryEntry* newDir = initDirectory(DEFAULT_DIR_SIZE, ppi.parent);

    int vacantDE = findUnusedDE(ppi.parent);
    if (vacantDE == -1) {
        free(newDir);
        return -1;
    }

    DirectoryEntry* x = loadDir(ppi.parent);
    
    memcpy(&(x[vacantDE]), newDir, sizeof(DirectoryEntry));
    strcpy(x[vacantDE].name, ppi.lastElement);

    writeBlock(x, x->size / vcb->blockSize, x->location);
    free(x);
    free(newDir);
    //Need Free if not working dir here!!! [TO-DO]

    return 0;
}

int fs_isDir(char* path) 
{
    ppInfo* ppi = malloc(sizeof(ppInfo));
    if (ppi == NULL)
        return -1;
    ppi->parent = malloc(sizeof(DirectoryEntry));
    if(ppi->parent == NULL) {
        free(ppi);
        return -1;
    }
    if (parsePath(path, ppi) != 0) 
    {
        free(ppi->parent);
        free(ppi);        
        return 0;
    }

    DirectoryEntry* dir = loadDir(ppi->parent);

    if(dir[ppi->lastElementIndex].isDirectory == 'd') 
    {
        free(dir);
        free(ppi);
        return 1;
    }

    free(dir);
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
    if(dirp->dirEntryPosition > dirp->d_reclen) {
        return NULL;
    }
    
    unsigned short pos = dirp->dirEntryPosition;
    dirp->di->d_reclen = 
        (dirp->directory[pos].size + sizeof(DirectoryEntry) - 1) 
        / sizeof(DirectoryEntry);
    dirp->di->fileType = dirp->directory[pos].isDirectory;
    strcpy(dirp->di->d_name, dirp->directory[pos].name);
    
    dirp->dirEntryPosition += 1;
    return dirp->di;
}