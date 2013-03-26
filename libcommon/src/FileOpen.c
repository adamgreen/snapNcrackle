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
    size_t filenameLength;
    char directoryName[PATH_LENGTH];
    char filename[PATH_LENGTH];
    char actualFilename[PATH_LENGTH];
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
static void throwIfLengthTooLong(int length, int limit);
static const char* findFilenameCaseInsensitive(FilenameParts* pThis);
__throws FILE* FileOpen(const char* pFilename, const char* pMode)
{
    FilenameParts filenameParts;
    __try
    {
        filenameParts = initFilenameParts(pFilename);
    }
    __catch
    {
        clearExceptionCode();
        return NULL;
    }
    return fopen(findFilenameCaseInsensitive(&filenameParts), pMode);
}

static FilenameParts initFilenameParts(const char* pFilename)
{
    FilenameParts    filenameParts = { 0, "", "", "" };
    
    FilenameInfo filenameInfo = determineFilenameInfo(pFilename);
    
    throwIfLengthTooLong(filenameInfo.filenameLength, sizeof(filenameParts.filename));
    throwIfLengthTooLong(filenameInfo.directoryNameLength, sizeof(filenameParts.directoryName));
    
    memcpy(filenameParts.directoryName, filenameInfo.pDirectoryName, filenameInfo.directoryNameLength);
    filenameParts.directoryName[filenameInfo.directoryNameLength] = '\0';
    memcpy(filenameParts.filename, filenameInfo.pFilename, filenameInfo.filenameLength + 1);
    filenameParts.filenameLength = filenameInfo.filenameLength;
    memcpy(filenameParts.actualFilename, filenameInfo.pFilename, filenameInfo.filenameLength + 1);

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

static void throwIfLengthTooLong(int length, int limit)
{
    if (length >= limit)
        __throw(outOfMemoryException);
}

static const char* findFilenameCaseInsensitive(FilenameParts* pThis)
{
    size_t         filenameLength = pThis->filenameLength;
    struct dirent* pNextEntry;
    
    DIR* pDir = opendir(pThis->directoryName);
    if (!pDir)
        return pThis->actualFilename;

    while (NULL != (pNextEntry = readdir(pDir)))
    {
        if (filenameLength == pNextEntry->d_namlen && 0 == strcasecmp(pNextEntry->d_name, pThis->filename))
        {
            memcpy(pThis->actualFilename, pNextEntry->d_name, pNextEntry->d_namlen+1);
            break;
        }
    }
    closedir(pDir);
    
    return pThis->actualFilename;
}
