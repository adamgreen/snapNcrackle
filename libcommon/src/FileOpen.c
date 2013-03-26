/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#include <dirent.h>
#include "FileOpen.h"
#include "FileOpenTest.h"
#include "string.h"

typedef struct
{
    char*  pDirectoryName;
    char*  pFilename;
    char*  pActualFilename;
    size_t filenameLength;
} FilenameParts;

typedef struct
{
    const char* pLastSlash;
    const char* pEndOfFilename;
} FilenamePointers;

typedef struct
{
    const char* pDirectoryName;
    const char* pFilename;
    size_t      directoryNameLength;
    size_t      filenameLength;
} FilenameInfo;


static FilenameParts initFilenameParts(const char* pFilename);
static FilenameInfo determineFilenameInfo(const char* pFilename);
static FilenamePointers findLastSlashAndEndOfFilename(const char* pFilename);
static void* throwingMalloc(size_t size);
static void freeFilenameParts(FilenameParts* pThis);
static const char* findFilenameCaseInsensitive(FilenameParts* pThis);
__throws FILE* FileOpen(const char* pFilename, const char* pMode)
{
    FilenameParts filenameParts = initFilenameParts(pFilename);
    FILE* pReturn = fopen(findFilenameCaseInsensitive(&filenameParts), pMode);
    freeFilenameParts(&filenameParts);

    return pReturn;
}

static FilenameParts initFilenameParts(const char* pFilename)
{
    FilenameParts    filenameParts = { NULL, NULL, NULL, 0 };
    
    __try
    {
        FilenameInfo filenameInfo = determineFilenameInfo(pFilename);
        
        filenameParts.pDirectoryName = throwingMalloc(filenameInfo.directoryNameLength + 1);
        filenameParts.pFilename = throwingMalloc(filenameInfo.filenameLength + 1);
        filenameParts.pActualFilename = throwingMalloc(filenameInfo.filenameLength + 1);
    
        memcpy(filenameParts.pDirectoryName, filenameInfo.pDirectoryName, filenameInfo.directoryNameLength);
        filenameParts.pDirectoryName[filenameInfo.directoryNameLength] = '\0';
        memcpy(filenameParts.pFilename, filenameInfo.pFilename, filenameInfo.filenameLength + 1);
        filenameParts.filenameLength = filenameInfo.filenameLength;
        memcpy(filenameParts.pActualFilename, filenameInfo.pFilename, filenameInfo.filenameLength + 1);
    }
    __catch
    {
        freeFilenameParts(&filenameParts);
        __rethrow;
    }
    return filenameParts;
}

static FilenameInfo determineFilenameInfo(const char* pFilename)
{
    static const char currentDirectory[] = ".";
    int               filenameLength;
    int               directoryNameLength;
    FilenameInfo      filenameInfo;
    FilenamePointers  filenamePointers = findLastSlashAndEndOfFilename(pFilename);

    filenameLength = filenamePointers.pEndOfFilename - filenamePointers.pLastSlash - 1;
    directoryNameLength = filenamePointers.pLastSlash - pFilename;
    if (directoryNameLength < 0)
    {
        /* There was no directory name portion so just set it to "." instead. */
        directoryNameLength = 1;
        pFilename = currentDirectory;
    }
    filenameInfo.pDirectoryName = pFilename;
    filenameInfo.pFilename = filenamePointers.pLastSlash + 1;
    filenameInfo.directoryNameLength = (size_t)directoryNameLength;
    filenameInfo.filenameLength = (size_t)filenameLength;
    return filenameInfo;
}

static FilenamePointers findLastSlashAndEndOfFilename(const char* pFilename)
{
    FilenamePointers filenamePointers;
    
    filenamePointers.pLastSlash = pFilename - 1;
    while (*pFilename)
    {
        if (*pFilename == '/')
            filenamePointers.pLastSlash = pFilename;
        pFilename++;
    }
    filenamePointers.pEndOfFilename = pFilename;
    return filenamePointers;
}

static void* throwingMalloc(size_t size)
{
    void* pReturn = malloc(size);
    if (!pReturn)
        __throw(outOfMemoryException);
    return pReturn;
}

static const char* findFilenameCaseInsensitive(FilenameParts* pThis)
{
    size_t         filenameLength = pThis->filenameLength;
    struct dirent* pNextEntry;
    
    DIR* pDir = opendir(pThis->pDirectoryName);
    if (!pDir)
        return pThis->pActualFilename;

    while (NULL != (pNextEntry = readdir(pDir)))
    {
        if (filenameLength == pNextEntry->d_namlen && 0 == strcasecmp(pNextEntry->d_name, pThis->pFilename))
        {
            memcpy(pThis->pActualFilename, pNextEntry->d_name, pNextEntry->d_namlen+1);
            break;
        }
    }
    closedir(pDir);
    
    return pThis->pActualFilename;
}

static void freeFilenameParts(FilenameParts* pThis)
{
    free(pThis->pDirectoryName);
    free(pThis->pFilename);
    free(pThis->pActualFilename);
}
