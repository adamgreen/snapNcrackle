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
#include "util.h"

typedef struct
{
    char*  pFilename;
    size_t filenameLength;
    char   fullFilename[PATH_LENGTH];
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


static void initFilenameParts(FilenameParts* pParts, const char* pFilename);
static FilenameInfo determineFilenameInfo(const char* pFilename);
static FilenamePointers findLastSlashAndEndOfFilename(const char* pFilename);
static void throwIfLengthTooLong(int length, int limit);
static const char* findFilenameCaseInsensitive(FilenameParts* pThis);
__throws FILE* FileOpen(const char* pFilename, const char* pMode)
{
    FilenameParts filenameParts;
    __try
    {
        initFilenameParts(&filenameParts, pFilename);
    }
    __catch
    {
        clearExceptionCode();
        return NULL;
    }
    return fopen(findFilenameCaseInsensitive(&filenameParts), pMode);
}

static void initFilenameParts(FilenameParts* pParts, const char* pFilename)
{
    FilenameInfo filenameInfo = determineFilenameInfo(pFilename);
    memset(pParts, 0, sizeof(*pParts));
    
    throwIfLengthTooLong(filenameInfo.directoryNameLength + 1 + filenameInfo.filenameLength + 1, 
                         sizeof(pParts->fullFilename));
    
    memcpy(pParts->fullFilename, filenameInfo.pDirectoryName, filenameInfo.directoryNameLength);
    pParts->fullFilename[filenameInfo.directoryNameLength] = PATH_SEPARATOR;
    pParts->pFilename = pParts->fullFilename + filenameInfo.directoryNameLength + 1;
    memcpy(pParts->pFilename, filenameInfo.pFilename, filenameInfo.filenameLength + 1);
    pParts->filenameLength = filenameInfo.filenameLength;
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
        if (*pFilename == PATH_SEPARATOR)
            filenamePointers.pLastSlash = pFilename;
        pFilename++;
    }
    filenamePointers.pEndOfFilename = pFilename;
    return filenamePointers;
}

static void throwIfLengthTooLong(int length, int limit)
{
    if (length >= limit)
        __throw(outOfMemoryException);
}

static const char* findFilenameCaseInsensitive(FilenameParts* pThis)
{
    size_t         filenameLength = pThis->filenameLength;
    struct dirent* pNextEntry;
    DIR*           pDir;
    
    pThis->pFilename[-1] = '\0';
    pDir = opendir(pThis->fullFilename);
    pThis->pFilename[-1] = PATH_SEPARATOR;
    if (!pDir)
        return pThis->fullFilename;

    while (NULL != (pNextEntry = readdir(pDir)))
    {
        if (filenameLength == pNextEntry->d_namlen && 0 == strcasecmp(pNextEntry->d_name, pThis->pFilename))
        {
            memcpy(pThis->pFilename, pNextEntry->d_name, pNextEntry->d_namlen+1);
            break;
        }
    }
    closedir(pDir);
    
    return pThis->fullFilename;
}
