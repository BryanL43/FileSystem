/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Kevin Lam, Bryan Lee, Matt Stoffel, Bryan Yao
* Student IDs:: 922350179, 922649673, 923127111, 922709642
* GitHub-Name:: MattRStoffel
* Group-Name:: Robert Bierman
* Project:: Basic File System
*
* File:: fsUtils.h
*
* Description:: Interface for the utility/helper functions.
*
**************************************************************/

#ifndef _FSUTIL_H
#define _FSUTIL_H

#include "directory.h"
#include "fsDesign.h"

DirectoryEntry* loadDir(DirectoryEntry* directory);
int findUnusedDE(DirectoryEntry* directory);
int FindNameInDir(DirectoryEntry* directory, char* name);
int parsePath(char* path, ppInfo* ppi);

#endif