#include "mfs.h"
#include "fsDesign.h"
#include "directory.h"
#include "fsUtils.h"

int fs_setcwd(char *pathname) {
    ppInfo* ppi = malloc(sizeof(ppInfo));
    ppi->parent = malloc(DIR_SIZE);

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
    
    memcpy(&(ppi.parent[vacantDE]), newDir, sizeof(DirectoryEntry));
    strcpy(ppi.parent[vacantDE].name, ppi.lastElement);

    free(newDir);
    //Need Free if not working dir here!!! [TO-DO]

    return 0;
}

int fs_isDir(char* path) 
{
    ppInfo* ppi = malloc(sizeof(ppInfo));
    if (ppi == NULL) 
    {
        return 0; 
    }
    if (parsePath(path, ppi) != 0) 
    {
        free(ppi);        
        return 0;
    }

    // if(ppi->parent[ppi->lastElementIndex].isDirectory == 'd') 
    if(0)
    {
        free(ppi);
        return 1;
    }
    free(ppi);
    return 0;
}

int fs_isFile(char* path) {
    return !fs_isDir;
}