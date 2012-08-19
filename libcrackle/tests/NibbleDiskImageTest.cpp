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
    #include "NibbleDiskImage.h"
    #include "BinaryBuffer.h"
    #include "MallocFailureInject.h"
    #include "FileFailureInject.h"
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

static const char* g_imageFilename = "NibbleDiskImageTest.nib";
static const char* g_savFilenameAllZeroes = "NibbleDiskImageTestAllZeroes.sav";
static const char* g_savFilenameAllOnes = "NibbleDiskImageAllOnes.sav";
static const char* g_scriptFilename = "NibbleDiskImageTest.script";


TEST_GROUP(NibbleDiskImage)
{
    NibbleDiskImage*     m_pNibbleDiskImage;
    const unsigned char* m_pCurr;
    FILE*                m_pFile;
    unsigned char*       m_pImageOnDisk;
    unsigned char        m_checksum;
    char                 m_buffer[256];
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(512);
        m_pNibbleDiskImage = NULL;
        m_pFile = NULL;
        m_pCurr = NULL;
        m_pImageOnDisk = NULL;
    }

    void teardown()
    {
        LONGS_EQUAL(noException, getExceptionCode());
        MallocFailureInject_Restore();
        printfSpy_Unhook();
        DiskImage_Free((DiskImage*)m_pNibbleDiskImage);
        if (m_pFile)
            fclose(m_pFile);
        free(m_pImageOnDisk);
        remove(g_imageFilename);
        remove(g_savFilenameAllZeroes);
        remove(g_savFilenameAllOnes);
        remove(g_scriptFilename);
    }
    
    char* copy(const char* pStringToCopy)
    {
        CHECK(strlen(pStringToCopy) < sizeof(m_buffer) - 1);
        strcpy(m_buffer, pStringToCopy);
        return m_buffer;
    }
    
    void validateAllZeroes(const unsigned char* pBuffer, size_t bufferSize)
    {
        for (size_t i = 0 ; i < bufferSize ; i++)
            LONGS_EQUAL(0, *pBuffer++);
    }
    
    void validateRWTS16SectorsAreClear(const unsigned char* pImage, 
                                       unsigned int         startTrack, 
                                       unsigned int         startSector,
                                       unsigned int         endTrack,
                                       unsigned int         endSector)
    {
        unsigned int sourceOffset = NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * startTrack + 
                                   NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR * startSector;
        unsigned int endOffset = NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * endTrack + 
                                 NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR * (endSector + 1);
        unsigned int length = endOffset - sourceOffset;
        
        validateAllZeroes(pImage + sourceOffset, length);
    }

    void validateRWTS16SectorContainsZeroData(const unsigned char* pImage, unsigned int track, unsigned int sector)
    {
        unsigned char expectedEncodedData[343];
        memset(expectedEncodedData, 0x96, sizeof(expectedEncodedData));
        validateRWTS16SectorContainsNibbles(pImage, expectedEncodedData, track, sector);
    }
    
    void validateRWTS16SectorContainsNibbles(const unsigned char* pImage,
                                             const unsigned char* pExpectedContent, 
                                             unsigned int         track, 
                                             unsigned int         sector)
    {
        static const unsigned char volume = 0;
        unsigned int               sourceOffset = NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * track + 
                                                 NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR * sector;
        m_pCurr = pImage + sourceOffset;
        
        validateSyncBytes(21);
        validateAddressField(volume, track, sector);
        validateSyncBytes(10);
        validateDataField(pExpectedContent);
        validateSyncBytes(22);
    }
    
    void validateSyncBytes(size_t syncByteCount)
    {
        for (size_t i = 0 ; i < syncByteCount ; i++)
            LONGS_EQUAL(0xFF, *m_pCurr++);
    }
    
    void validateAddressField(unsigned char volume, unsigned char track, unsigned char sector)
    {
        validateAddressFieldProlog();
        
        initChecksum();
        validate4by4Data(volume);
        validate4by4Data(track);
        validate4by4Data(sector);
        validateChecksum();
        
        validateFieldEpilog();
    }
    
    void validateAddressFieldProlog()
    {
        CHECK_TRUE(0 == memcmp(m_pCurr, "\xD5\xAA\x96", 3));
        m_pCurr += 3;
    }
    
    void initChecksum()
    {
        m_checksum = 0;
    }
    
    void validate4by4Data(unsigned char byte)
    {
        unsigned char oddBits = byte & 0xAA;
        unsigned char evenBits = byte & 0x55;
        unsigned char encodedOddByte = 0xAA | (oddBits >> 1);
        unsigned char encodedEvenByte = 0xAA | evenBits;
        
        updateChecksum(byte);
        LONGS_EQUAL(encodedOddByte, *m_pCurr++);
        LONGS_EQUAL(encodedEvenByte, *m_pCurr++);
    }
    
    void updateChecksum(unsigned char byte)
    {
        m_checksum ^= byte;
    }
    
    void validateChecksum()
    {
        validate4by4Data(m_checksum);
    }
    
    void validateFieldEpilog()
    {
        CHECK_TRUE(0 == memcmp(m_pCurr, "\xDE\xAA\xEB", 3));
        m_pCurr += 3;
    }
    
    void validateDataField(const unsigned char* pExpectedContent)
    {
        validateDataFieldProlog();
        
        validateData(pExpectedContent);
        
        validateFieldEpilog();
    }
    
    void validateDataFieldProlog()
    {
        CHECK_TRUE(0 == memcmp(m_pCurr, "\xD5\xAA\xAD", 3));
        m_pCurr += 3;
    }
    
    void validateData(const unsigned char* pExpectedContent)
    {
        CHECK_TRUE(0 == memcmp(m_pCurr, pExpectedContent, 343));
        m_pCurr += 343;
    }
    
    void writeZeroSectors(unsigned int startTrack, unsigned int startSector, unsigned int sectorCount)
    {
        unsigned int   totalSectorSize = sectorCount * DISK_IMAGE_BYTES_PER_SECTOR;
        unsigned char* pSectorData = (unsigned char*)malloc(totalSectorSize);
        CHECK_TRUE(pSectorData != NULL);
        memset(pSectorData, 0, totalSectorSize);
        
        DiskImageInsert insert;
        insert.type = DISK_IMAGE_INSERTION_RWTS16;
        insert.sourceOffset = 0;
        insert.length = totalSectorSize;
        insert.track = startTrack;
        insert.sector = startSector;

        NibbleDiskImage_InsertData(m_pNibbleDiskImage, pSectorData, &insert);
        free(pSectorData);
    }
    
    const unsigned char* readNibbleDiskImageIntoMemory(void)
    {
        m_pFile = fopen(g_imageFilename, "r");
        CHECK(m_pFile != NULL);
        LONGS_EQUAL(NIBBLE_DISK_IMAGE_SIZE, getFileSize(m_pFile));
        
        m_pImageOnDisk = (unsigned char*)malloc(NIBBLE_DISK_IMAGE_SIZE);
        CHECK(m_pImageOnDisk != NULL);
        LONGS_EQUAL(NIBBLE_DISK_IMAGE_SIZE, fread(m_pImageOnDisk, 1, NIBBLE_DISK_IMAGE_SIZE, m_pFile));

        fclose(m_pFile);
        m_pFile = NULL;
        
        return m_pImageOnDisk;
    }
    
    long getFileSize(FILE* pFile)
    {
        fseek(pFile, 0, SEEK_END);
        long size = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);
        return size;
    }
    
    void validateOutOfMemoryExceptionThrown()
    {
        validateExceptionThrown(outOfMemoryException);
    }
    
    void validateInvalidArgumentExceptionThrown()
    {
        validateExceptionThrown(invalidArgumentException);
    }
    
    void validateFileExceptionThrown()
    {
        validateExceptionThrown(fileException);
    }
    
    void validateExceptionThrown(int expectedExceptionCode)
    {
        LONGS_EQUAL(expectedExceptionCode, getExceptionCode());
        clearExceptionCode();
    }
    
    void createZeroSectorObjectFile()
    {
        unsigned char sectorData[DISK_IMAGE_BYTES_PER_SECTOR];
        memset(sectorData, 0, sizeof(sectorData));
        createSectorObjectFile(g_savFilenameAllZeroes, sectorData, sizeof(sectorData));
    }
    
    void createOnesSectorObjectFile()
    {
        unsigned char sectorData[DISK_IMAGE_BYTES_PER_SECTOR];
        memset(sectorData, 0xff, sizeof(sectorData));
        createSectorObjectFile(g_savFilenameAllOnes, sectorData, sizeof(sectorData));
    }
    
    void createSectorObjectFile(const char* pFilename, unsigned char* pSectorData, size_t sectorDataSize)
    {
        SavFileHeader header;
    
        memcpy(header.signature, BINARY_BUFFER_SAV_SIGNATURE, sizeof(header.signature));
        header.address = 0;
        header.length = sectorDataSize;
    
        FILE* pFile = fopen(pFilename, "w");
        fwrite(&header, 1, sizeof(header), pFile);
        fwrite(pSectorData, 1, sectorDataSize, pFile);
        fclose(pFile);
    }

    void createTextFile(const char* pFilename, const char* pText)
    {
        FILE* pFile = fopen(pFilename, "w");
        fwrite(pText, 1, strlen(pText), pFile);
        fclose(pFile);
    }
};


TEST(NibbleDiskImage, FailAllAllocationsInCreate)
{
    static const int allocationsToFail = 3;
    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        m_pNibbleDiskImage = NibbleDiskImage_Create();
        POINTERS_EQUAL(NULL, m_pNibbleDiskImage);
        validateOutOfMemoryExceptionThrown();
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    CHECK_TRUE(m_pNibbleDiskImage != NULL);
}

TEST(NibbleDiskImage, VerifyCreateStartsWithZeroesInImage)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    CHECK_TRUE(NULL != m_pNibbleDiskImage);
    
    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    CHECK_TRUE(NULL != pImage);
    LONGS_EQUAL(NIBBLE_DISK_IMAGE_SIZE, NibbleDiskImage_GetImageSize(m_pNibbleDiskImage));
    validateAllZeroes(pImage, NIBBLE_DISK_IMAGE_SIZE);
}

TEST(NibbleDiskImage, InsertZeroSectorAt0_0AsRWTS16)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(0, 0, 1);

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
}

TEST(NibbleDiskImage, InsertZeroSectorAt34_15AsRWTS16)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(34, 15, 1);

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 0, 34, 14);
    validateRWTS16SectorContainsZeroData(pImage, 34, 15);
}

TEST(NibbleDiskImage, InsertTwoZeroSectorsAt0_15AsRWTS16)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(0, 15, 2);
 
    unsigned char expectedEncodedData[343];
    memset(expectedEncodedData, 0x96, sizeof(expectedEncodedData));
    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 0, 0, 14);
    validateRWTS16SectorContainsZeroData(pImage, 0, 15);
    validateRWTS16SectorContainsZeroData(pImage, 1, 0);
    validateRWTS16SectorsAreClear(pImage, 1, 1, 34, 15);
}

TEST(NibbleDiskImage, FailToInsertSector16AsRWTS16)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(0, 16, 1);
    validateExceptionThrown(invalidSectorException);
}

TEST(NibbleDiskImage, FailToInsertTrack35AsRWTS16)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(35, 0, 1);
    validateExceptionThrown(invalidTrackException);
}

TEST(NibbleDiskImage, FailToInsertSecondSectorAsRWTS16)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(34, 15, 2);
    validateExceptionThrown(invalidTrackException);
}

TEST(NibbleDiskImage, InsertTestSectorAt0_0AsRWTS16)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();

    unsigned char sectorData[256] =
    {
        0x00, 0x00, 0x03, 0x00, 0xfa, 0x55, 0x53, 0x45,
        0x52, 0x53, 0x2e, 0x44, 0x49, 0x53, 0x4b, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xc3, 0x27, 0x0d, 0x09, 0x00, 0x06,
        0x00, 0x18, 0x01, 0x26, 0x50, 0x52, 0x4f, 0x44,
        0x4f, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x08, 0x00, 0x1f, 0x00,
        0x00, 0x3c, 0x00, 0x21, 0xa8, 0x00, 0x00, 0x00,
        0x00, 0x21, 0x00, 0x20, 0x21, 0xa8, 0x00, 0x00,
        0x02, 0x00, 0x2c, 0x42, 0x41, 0x53, 0x49, 0x43,
        0x2e, 0x53, 0x59, 0x53, 0x54, 0x45, 0x4d, 0x00,
        0x00, 0x00, 0xff, 0x27, 0x00, 0x15, 0x00, 0x00,
        0x28, 0x00, 0x6f, 0xa7, 0x00, 0x00, 0x00, 0x00,
        0x21, 0x00, 0x20, 0x6f, 0xa7, 0x00, 0x00, 0x02,
        0x00, 0x25, 0x46, 0x49, 0x4c, 0x45, 0x52, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x3c, 0x00, 0x33, 0x00, 0x00, 0x64,
        0x00, 0x21, 0xa8, 0x00, 0x00, 0x00, 0x00, 0x21,
        0x6e, 0x01, 0x21, 0xa8, 0x00, 0x00, 0x02, 0x00,
        0x27, 0x43, 0x4f, 0x4e, 0x56, 0x45, 0x52, 0x54,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x6f, 0x00, 0x2a, 0x00, 0x01, 0x50, 0x00,
        0x61, 0xa7, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00,
        0x20, 0x61, 0xa7, 0x00, 0x00, 0x02, 0x00, 0x27,
        0x53, 0x54, 0x41, 0x52, 0x54, 0x55, 0x50, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
        0x99, 0x00, 0x18, 0x00, 0xc9, 0x2c, 0x00, 0x4f,
        0xa7, 0x00, 0x00, 0x00, 0x00, 0x21, 0x01, 0x08,
        0x4f, 0xa7, 0x00, 0x00, 0x02, 0x00, 0x25, 0x4d,
        0x4f, 0x49, 0x52, 0x45, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xb1
    };
    DiskImageInsert insert;
    insert.type = DISK_IMAGE_INSERTION_RWTS16;
    insert.sourceOffset = 0;
    insert.length = 256;
    insert.track = 0;
    insert.sector = 0;
    NibbleDiskImage_InsertData(m_pNibbleDiskImage, sectorData, &insert);

    unsigned char expectedEncodedData[343] =
    {
        0xa7, 0x9d, 0xad, 0xad, 0xf4, 0xa6, 0xfd, 0xbe,
        0xb7, 0xe6, 0xd9, 0x97, 0xeb, 0xb5, 0xfc, 0xad,
        0xa7, 0x96, 0xd6, 0xd6, 0xae, 0xd6, 0xcd, 0xed,
        0x96, 0xb4, 0xbd, 0xf7, 0x96, 0xfc, 0xd6, 0xfc,
        0xb4, 0xdb, 0xde, 0xa7, 0xaf, 0xae, 0xac, 0xab,
        0xaf, 0x9d, 0x9a, 0x9b, 0xd7, 0xd7, 0x9a, 0x9b,
        0xda, 0xd6, 0x9b, 0xfc, 0xae, 0xed, 0xae, 0xae,
        0x96, 0xd6, 0x96, 0xe7, 0xfb, 0x96, 0xf2, 0x9b,
        0xb4, 0xbd, 0xe9, 0xb2, 0xb6, 0xbd, 0xed, 0xed,
        0xdb, 0x9f, 0xb2, 0x96, 0x9a, 0xac, 0x96, 0xae,
        0xaf, 0x9e, 0x96, 0xd7, 0xda, 0x97,
        0x9b, 0x96, 0x96, 0x96, 0xfe, 0xe7, 0x97, 0x9e,
        0x9e, 0x96, 0xd3, 0xbf, 0x9b, 0x9f, 0x9f, 0xb6,
        0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96,
        0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96,
        0x96, 0x96, 0xed, 0xf9, 0xac, 0x97, 0x9a, 0x97,
        0x97, 0x9f, 0x9f, 0xab, 0xce, 0x96, 0xa6, 0x9a,
        0x9a, 0xa6, 0xb9, 0x96, 0x96, 0x96, 0x96, 0x96,
        0x96, 0x96, 0x96, 0xff, 0xfd, 0x9a, 0xa6, 0xa6,
        0x96, 0xb3, 0xb3, 0xa7, 0xd9, 0xe6, 0x96, 0x96,
        0x96, 0xa7, 0xa7, 0xa7, 0x96, 0xd9, 0xe6, 0x96,
        0x96, 0x96, 0xad, 0xcb, 0x96, 0x9d, 0x9f, 0x9a,
        0xcb, 0xd3, 0x9a, 0x9a, 0x97, 0x9d, 0x9a, 0xb7,
        0x96, 0x96, 0xff, 0xf5, 0xab, 0x9e, 0x9e, 0x96,
        0xac, 0xac, 0xcb, 0xef, 0xe5, 0x96, 0x96, 0x96,
        0xa7, 0xa7, 0xa7, 0xb7, 0xef, 0xe5, 0x96, 0x96,
        0x96, 0xab, 0xbd, 0x9b, 0x97, 0x9a, 0x9e, 0xb9,
        0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96,
        0x96, 0xff, 0xed, 0xb3, 0xae, 0xae, 0x96, 0xbe,
        0xbe, 0xa7, 0xd9, 0xe6, 0x96, 0x96, 0x96, 0xa7,
        0xb7, 0xcb, 0xa7, 0xd9, 0xe6, 0x96, 0x96, 0x96,
        0xab, 0xbe, 0x9b, 0x96, 0x9f, 0x9d, 0x9e, 0x97,
        0xba, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96,
        0xff, 0xdb, 0xcb, 0xac, 0xac, 0x96, 0xb9, 0xb9,
        0xbd, 0xee, 0xe5, 0x96, 0x96, 0x96, 0xa7, 0xa7,
        0xa7, 0xb4, 0xee, 0xe5, 0x96, 0x96, 0x96, 0xab,
        0xce, 0x97, 0x9e, 0x9d, 0x97, 0x96, 0x97, 0xb9,
        0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0xff,
        0xbe, 0xdd, 0x9f, 0x9f, 0xef, 0xf9, 0xad, 0xb7,
        0xfa, 0xe5, 0x96, 0x96, 0x96, 0xa7, 0xa7, 0x9a,
        0xb5, 0xfa, 0xe5, 0x96, 0x96, 0x96, 0xab, 0xbf,
        0x96, 0x97, 0x9f, 0x9e, 0xb5, 0x96, 0x96, 0x96,
        0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0xff, 0xb7,
        0xe9
    };
    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsNibbles(pImage, expectedEncodedData, 0, 0);
}

TEST(NibbleDiskImage, WriteImage)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(0, 0, 1);
    NibbleDiskImage_WriteImage(m_pNibbleDiskImage, g_imageFilename);
    
    const unsigned char* pImage = readNibbleDiskImageIntoMemory();
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
}

TEST(NibbleDiskImage, FailFOpenInWriteImage)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(0, 0, 1);
    fopenFail(NULL);
        NibbleDiskImage_WriteImage(m_pNibbleDiskImage, g_imageFilename);
    fopenRestore();
    validateFileExceptionThrown();
}

TEST(NibbleDiskImage, FailFWriteInWriteImage)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    writeZeroSectors(0, 0, 1);
    fwriteFail(0);
        NibbleDiskImage_WriteImage(m_pNibbleDiskImage, g_imageFilename);
    fwriteRestore();
    validateFileExceptionThrown();
}

TEST(NibbleDiskImage, ReadObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);
}

TEST(NibbleDiskImage, FailFOpenInReadObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);
    validateFileExceptionThrown();
}

TEST(NibbleDiskImage, FailHeaderReadInReadObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    
    freadFail(0);
        NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);
    freadRestore();
    validateFileExceptionThrown();
}

TEST(NibbleDiskImage, FailAllocationInReadObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    
    MallocFailureInject_FailAllocation(1);
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);
    validateOutOfMemoryExceptionThrown();
}

TEST(NibbleDiskImage, FailDataReadInReadObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    
    freadFail(0);
    freadToFail(2);
        NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);
    freadRestore();
    validateFileExceptionThrown();
}

TEST(NibbleDiskImage, ReadObjectFileAndWriteToImage)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);

    DiskImageInsert insert;
    insert.type = DISK_IMAGE_INSERTION_RWTS16;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BYTES_PER_SECTOR;
    insert.track = 0;
    insert.sector = 0;
    NibbleDiskImage_InsertObjectFile(m_pNibbleDiskImage, &insert);
    
    NibbleDiskImage_WriteImage(m_pNibbleDiskImage, g_imageFilename);
    const unsigned char* pImage = readNibbleDiskImageIntoMemory();
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
}

TEST(NibbleDiskImage, OutOfBoundsStartingOffsetForInsertObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);

    DiskImageInsert insert;
    insert.type = DISK_IMAGE_INSERTION_RWTS16;
    insert.sourceOffset = DISK_IMAGE_BYTES_PER_SECTOR;
    insert.length = DISK_IMAGE_BYTES_PER_SECTOR;
    insert.track = 0;
    insert.sector = 0;
    NibbleDiskImage_InsertObjectFile(m_pNibbleDiskImage, &insert);
    validateExceptionThrown(invalidSourceOffsetException);
}

TEST(NibbleDiskImage, OutOfBoundsEndingOffsetForInsertObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);

    DiskImageInsert insert;
    insert.type = DISK_IMAGE_INSERTION_RWTS16;
    insert.sourceOffset = 1;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.track = 0;
    insert.sector = 0;
    NibbleDiskImage_InsertObjectFile(m_pNibbleDiskImage, &insert);
    validateExceptionThrown(invalidLengthException);
}

TEST(NibbleDiskImage, VerifyRoundUpToPageForInsertObjectFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    unsigned char zeroSectorData[DISK_IMAGE_PAGE_SIZE + 1];
    memset(zeroSectorData, 0, sizeof(zeroSectorData));
    createSectorObjectFile(g_savFilenameAllZeroes, zeroSectorData, sizeof(zeroSectorData));
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);

    DiskImageInsert insert;
    insert.type = DISK_IMAGE_INSERTION_RWTS16;
    insert.sourceOffset = DISK_IMAGE_PAGE_SIZE;
    insert.length = DISK_IMAGE_BYTES_PER_SECTOR;
    insert.track = 0;
    insert.sector = 0;
    NibbleDiskImage_InsertObjectFile(m_pNibbleDiskImage, &insert);

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
}

TEST(NibbleDiskImage, ReadTwoObjectFilesAndOnlyWriteSecondToImage)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    createOnesSectorObjectFile();
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllOnes);
    NibbleDiskImage_ReadObjectFile(m_pNibbleDiskImage, g_savFilenameAllZeroes);

    DiskImageInsert insert;
    insert.type = DISK_IMAGE_INSERTION_RWTS16;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BYTES_PER_SECTOR;
    insert.track = 0;
    insert.sector = 0;
    NibbleDiskImage_InsertObjectFile(m_pNibbleDiskImage, &insert);
    
    NibbleDiskImage_WriteImage(m_pNibbleDiskImage, g_imageFilename);
    const unsigned char* pImage = readNibbleDiskImageIntoMemory();
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
}

TEST(NibbleDiskImage, ProcessOneLineTextScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,0\n"));

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
}

TEST(NibbleDiskImage, ProcessOneLineTextScriptWithNoNewLineAtEnd)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,0"));

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
}

TEST(NibbleDiskImage, ProcessOneLineTextScriptWithComment)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("#RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,0"));

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 15);
    STRCMP_EQUAL("", printfSpy_GetLastErrorOutput());
}

TEST(NibbleDiskImage, ProcessTwoLineTextScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,0\n"
                                                           "RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,34,15\n"));

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 14);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
    validateRWTS16SectorContainsZeroData(pImage, 34, 15);
}

TEST(NibbleDiskImage, FailTextFileCreateInProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    MallocFailureInject_FailAllocation(1);
    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,0\n"));
    validateOutOfMemoryExceptionThrown();
}

TEST(NibbleDiskImage, PassInvalidEmptyLineToProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("\n"));
    STRCMP_EQUAL("<null>:1: error: Script line cannot be blank.\n",
                 printfSpy_GetLastErrorOutput());
}
TEST(NibbleDiskImage, PassInvalidScriptInsertionTypeToProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("foo.bar"));
    STRCMP_EQUAL("<null>:1: error: foo.bar isn't a recognized image insertion type of BLOCK or RWTS16.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(NibbleDiskImage, PassUnsupportedScriptInsertionTypeToProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("BLOCK,NibbleDiskImageTestAllZeroes.sav,0,256,0"));
    STRCMP_EQUAL("<null>:1: error: BLOCK insertion type isn't supported for this output image type.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(NibbleDiskImage, PassTooFewParametersToProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0"));
    STRCMP_EQUAL("<null>:1: error: Line doesn't contain correct fields: RWTS16,objectFilename,objectStartOffset,insertionLength,track,sector\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(NibbleDiskImage, PassInvalidFilenameToProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,InvalidFilename.sav,0,256,0,0\n"));
    STRCMP_EQUAL("<null>:1: error: Failed to read 'InvalidFilename.sav' object file.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(NibbleDiskImage, PassInvalidSectorToProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,16\n"));
    STRCMP_EQUAL("<null>:1: error: 16 specifies an invalid sector.  Must be 0 - 15.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(NibbleDiskImage, PassInvalidTrackToProcessScript)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();

    NibbleDiskImage_ProcessScript(m_pNibbleDiskImage, copy("RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,35,0\n"));
    STRCMP_EQUAL("<null>:1: error: Write starting at track/sector 35/0 won't fit in output image file.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(NibbleDiskImage, ProcessTwoLineScriptFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    createTextFile(g_scriptFilename, "RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,0\n"
                                     "RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,34,15\n");

    NibbleDiskImage_ProcessScriptFile(m_pNibbleDiskImage, g_scriptFilename);

    const unsigned char* pImage = NibbleDiskImage_GetImagePointer(m_pNibbleDiskImage);
    validateRWTS16SectorsAreClear(pImage, 0, 1, 34, 14);
    validateRWTS16SectorContainsZeroData(pImage, 0, 0);
    validateRWTS16SectorContainsZeroData(pImage, 34, 15);
}

TEST(NibbleDiskImage, FailToAllocateTextFileInProcessScriptFile)
{
    m_pNibbleDiskImage = NibbleDiskImage_Create();
    createZeroSectorObjectFile();
    createTextFile(g_scriptFilename, "RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,0,0\n"
                                     "RWTS16,NibbleDiskImageTestAllZeroes.sav,0,256,34,15\n");

    MallocFailureInject_FailAllocation(1);
    NibbleDiskImage_ProcessScriptFile(m_pNibbleDiskImage, g_scriptFilename);
    validateOutOfMemoryExceptionThrown();
}

