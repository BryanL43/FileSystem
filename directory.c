DirectoryEntry *initDirectory(int minEntries, DirectoryEntry *parent)
{
    int bytesNeeded = minEntries * sizeof(DirectoryEntry);
    int blocksNeeded = (bytesNeeded + BLOCKSIZE - 1) / BLOCKSIZE;
    int bytesToALloc = bytesNeeded * BLOCKSIZE;
    DirectoryEntry *DEs = malloc(bytesToALloc);

    if (DEs == NULL)
    {
        // Handle memory allocation failure
        return NULL;
    }

    int actualEntries = bytesToALloc / sizeof(DirectoryEntry);
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

    // Initialize . and ..
    int newLoc = 100; // TODO: Replace with fsalloc(blocksNeeded)
    time_t currentTime = time(NULL);

    // Initialize "."
    DEs[0].location = newLoc;
    DEs[0].size = actualEntries;
    strcpy(DEs[0].name, ".");
    DEs[0].isDirectory = 'd';
    DEs[0].dateCreated = currentTime;
    DEs[0].dateModified = currentTime;
    DEs[0].permissions = 0;

    DirectoryEntry *dotdot = parent;
    if (dotdot == NULL)
    {
        dotdot = &DEs[0];
    }

    memcpy(&DEs[1], &DEs[0], sizeof(DirectoryEntry));
    strcpy(DEs[0].name, "..");

    return DEs;
}