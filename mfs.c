#include "mfs.h"
#include "fsDesign.h"
#include "directory.h"

DirectoryEntry* loadDir(DirectoryEntry* directory, int index) {
    // int location = directory[index].location;
    // DirectoryEntry* temp = malloc(sizeof(DirectoryEntry));
    // if (temp == NULL) {
    //     perror("Failed to instantiate a temp directory for loadDir!\n");
    //     return NULL;
    // }

    return NULL;
}

int parsePath(const char* path, ppInfo* ppi) {
    // if (path == NULL || ppi == NULL) {
    //     perror("Error: parse path recieved invalid pointers!\n");
    //     return -1;
    // }

    // DirectoryEntry* start;
    // if (path[0] == '/') {
    //     start = root;
    // } else {
    //     // start = 
    // }

    return 0;
}

int fs_setcwd(char *pathname) {
    // ppInfo* ppi = malloc(sizeof(ppInfo));
    // ppi->parent = malloc(sizeof(DirectoryEntry));

    // int ret = parsePath(pathname, ppi);
    // ret = -1;
    
    // // Error check parsePath
    // if (ret == -1 || ppi->lastElementIndex == -1) {
    //     perror("Error: parse path failed in fs_setcwd!\n");
    //     free(ppi->parent);
    //     free(ppi);
    //     return -1;
    // }

    // // Special sentinel for root
    // if (ppi->lastElementIndex == -2) {
        
    // }

    return 0;
}

char* fs_getcwd(char *pathname, size_t size) {
    strncpy(pathname, cwdPathName, size);
    return cwdPathName;
}

int fs_mkdir(const char *pathname, mode_t mode) {
    // ppInfo ppi;
    // parsePath(pathname, &ppi);
    return 0;
}