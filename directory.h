/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: directory.h
*
* Description:: Interface for the directory creation method
*
**************************************************************/

#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include <stdlib.h>
#include <string.h>
#include "fsDesign.h"
#include "freeSpace.h"

// Default directory size is 64 directory entries
#define DEFAULT_DIR_SIZE 3

typedef struct DirectoryEntry
{
   time_t dateCreated;     // seconds since epoch
   time_t dateModified;    // seconds since epoch

   char name[55];          // directory entry name
   char isDirectory;       // is directory entry a directory

   int size;               // directory entry size
   int location;           // starting block number for the file data

   // total size = 80, divisible by 16, and no internal paddings
} DirectoryEntry;

DirectoryEntry *initDirectory(int minEntries, DirectoryEntry *parent);

extern DirectoryEntry* root;
extern DirectoryEntry* cwd;
extern char* cwdPathName;

#endif