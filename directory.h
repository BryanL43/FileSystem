#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include <stdlib.h>
#include <string.h>
#include "fsDesign.h"
#include "freeSpace.h"

typedef struct DirectoryEntry
{
   time_t dateCreated;     // seconds since epoch
   time_t dateModified;    // seconds since epoch

   char name[51];          // directory entry name
   char isDirectory;       // is directory entry a directory

   int size;               // directory entry size
   int location;           // starting block number for the file data
   int permissions;        // file modes converted to decimal format

   // total size = 80, divisible by 16, and no internal paddings
} DirectoryEntry;

DirectoryEntry *initDirectory(int minEntries, DirectoryEntry *parent);

extern DirectoryEntry* root;
#endif