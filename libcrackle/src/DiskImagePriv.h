/*  Copyright (C) 2012  Adam Green (https://github.com/adamgreen)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#ifndef _DISK_IMAGE_PRIV_H_
#define _DISK_IMAGE_PRIV_H_

#include "DiskImage.h"
#include "TextFile.h"
#include "ParseCSV.h"
#include "ByteBuffer.h"


typedef struct DiskImage DiskImage;


typedef struct DiskImageVTable
{
    void (*insertData)(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert);

} DiskImageVTable;


typedef struct DiskImageScriptEngine
{
    DiskImage*      pDiskImage;
    TextFile*       pTextFile;
    ParseCSV*       pParser;
    const char*     pScriptFilename;
    DiskImageInsert insert;
    unsigned int    lineNumber;
} DiskImageScriptEngine;


struct DiskImage
{
    DiskImageVTable*      pVTable;
    ByteBuffer            image;
    ByteBuffer            object;
    DiskImageScriptEngine script;
};


__throws DiskImage DiskImage_Init(DiskImageVTable* pVTable, unsigned int imageSize);
         void      DiskImage_Free(DiskImage* pThis);

__throws void      DiskImageScriptEngine_ProcessScriptFile(DiskImageScriptEngine* pThis, 
                                                           DiskImage*             pDiskImage, 
                                                           const char*            pScriptFilename);
__throws void      DiskImageScriptEngine_ProcessScript(DiskImageScriptEngine* pThis, 
                                                       DiskImage*             pDiskImage,
                                                       char*                  pScriptText);

__throws void      DiskImage_ReadObjectFile(DiskImage* pThis, const char* pFilename);
__throws void      DiskImage_InsertObjectFile(DiskImage* pThis, DiskImageInsert* pInsert);

__throws void      DiskImage_WriteImage(DiskImage* pThis, const char* pImageFilename);

         unsigned char* DiskImage_GetImagePointer(DiskImage* pThis);
         size_t         DiskImage_GetImageSize(DiskImage* pThis);

#endif /* _DISK_IMAGE_PRIV_H_ */
