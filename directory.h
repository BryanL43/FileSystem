#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include <stdlib.h>
#include <string.h>
#include "fsDesign.h"
#include "freeSpace.h"

DirectoryEntry *initDirectory(int minEntries, DirectoryEntry *parent);

#endif