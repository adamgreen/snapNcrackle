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

// Include headers from C modules under test.
extern "C"
{
    #include "DiskImage.h"
    #include "MallocFailureInject.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

static const char* g_imageFilename = "DiskImageTest.nib";


TEST_GROUP(DiskImage)
{
    DiskImage* m_pDiskImage;
    
    void setup()
    {
        m_pDiskImage = NULL;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        DiskImage_Free(m_pDiskImage);
    }
    
    void validateAllZeroes(const unsigned char* pBuffer, size_t bufferSize)
    {
        for (size_t i = 0 ; i < bufferSize ; i++)
        {
            LONGS_EQUAL(0, *pBuffer++);
        }
    }
};


TEST(DiskImage, FailAllocationInCreate)
{
    MallocFailureInject_FailAllocation(1);
    m_pDiskImage = DiskImage_Create(g_imageFilename);
    POINTERS_EQUAL(NULL, m_pDiskImage);
}

TEST(DiskImage, Create)
{
    m_pDiskImage = DiskImage_Create(g_imageFilename);
    CHECK_TRUE(NULL != m_pDiskImage);
    
    const unsigned char* pImage = DiskImage_GetImagePointer(m_pDiskImage);
    CHECK_TRUE(NULL != pImage);
    LONGS_EQUAL(DISK_IMAGE_SIZE, DiskImage_GetImageSize(m_pDiskImage));
    validateAllZeroes(pImage, DISK_IMAGE_SIZE);
}
