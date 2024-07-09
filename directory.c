#include "directory.h"
#include <time.h>

DirectoryEntry *initDirectory(int minEntries, DirectoryEntry *parent)
{
    int bytesNeeded = minEntries * sizeof(DirectoryEntry);
    int blocksNeeded = (bytesNeeded + vcb->blockSize - 1) / vcb->blockSize;
    int bytesToAlloc = bytesNeeded * vcb->blockSize;

    DirectoryEntry *DEs = malloc(bytesToAlloc);
    if (DEs == NULL) {
        return NULL;
    }

    int actualEntries = bytesToAlloc / sizeof(DirectoryEntry);
    
    // TODO: init all the actual entries
    for(int i = 2; i < actualEntries; i++) {
        time_t currentTime = time(NULL);
        DEs[i].dateCreated = currentTime;
        DEs[i].dateModified = currentTime;
        strcpy(DEs[i].name, "");
        DEs[i].isDirectory = ' ';
        DEs[i].size = 0;
        DEs[i].location = -1;
        DEs[i].permissions = 0;
    }

    // Get Free Space
    int newLoc = getFreeBlocks(blocksNeeded);
    if (newLoc == -1) {
        printf("Error: unable to get free block for directory!\n");
        return NULL;
    }

    time_t currentTime = time(NULL);

    printf("Seconds since January 1, 1970 = %d\n", currentTime);

    // Initialize "."
    DEs[0].location = newLoc;
    DEs[0].size = actualEntries;
    strcpy(DEs[0].name, ".");
    DEs[0].isDirectory = 'd';
    DEs[0].dateCreated = currentTime;
    DEs[0].dateModified = currentTime;
    DEs[0].permissions = 0;

    // Initialize ".."
    DirectoryEntry *dotdot = parent;
    if (dotdot == NULL)
    {
        dotdot = &DEs[0];
    }

    memcpy(&DEs[1], dotdot, sizeof(DirectoryEntry));
    strcpy(DEs[0].name, "..");

    if (writeBlock(blocksNeeded, DEs, newLoc) == -1) {
        printf("Error: Failed to write block for directory!\n");
        return NULL;
    }

    return DEs;
}