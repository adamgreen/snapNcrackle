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
#include "DiskImage.h"
#include "DiskImageTest.h"
#include "util.h"


struct DiskImage
{
    unsigned char image[DISK_IMAGE_SIZE];
};


__throws DiskImage* DiskImage_Create(const char* pImageFilename)
{
    DiskImage* pThis;
    
    __try
        pThis = allocateAndZero(sizeof(*pThis));
    __catch
        __rethrow_and_return(NULL);
        
    return pThis;
}


void DiskImage_Free(DiskImage* pThis)
{
    if (!pThis)
        return;
    free(pThis);
}


const unsigned char* DiskImage_GetImagePointer(DiskImage* pThis)
{
    return pThis->image;
}


size_t DiskImage_GetImageSize(DiskImage* pThis)
{
    return DISK_IMAGE_SIZE;
}
