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
#include <assert.h>
// Include headers from C modules under test.
extern "C"
{
    #include "BlockDiskImage.h"
    #include "BinaryBuffer.h"
    #include "MallocFailureInject.h"
    #include "FileFailureInject.h"
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

static const char* g_imageFilename = "BlockDiskImageTest.2mg";
static const char* g_savFilenameAllZeroes = "BlockDiskImageTestZeroes.sav";
static const char* g_savFilenameAllOnes = "BlockDiskImageTestOnes.sav";
static const char* g_usrFilenameAllOnes = "BlockDiskImageTestOnes.usr";
static const char* g_scriptFilename = "BlockDiskImageTest.script";


TEST_GROUP(BlockDiskImage)
{
    BlockDiskImage*      m_pDiskImage;
    FILE*                m_pFile;
    unsigned char*       m_pImageOnDisk;
    char                 m_buffer[256];
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(512);
        m_pDiskImage = NULL;
        m_pFile = NULL;
        m_pImageOnDisk = NULL;
    }

    void teardown()
    {
        LONGS_EQUAL(noException, getExceptionCode());
        MallocFailureInject_Restore();
        printfSpy_Unhook();
        DiskImage_Free((DiskImage*)m_pDiskImage);
        if (m_pFile)
            fclose(m_pFile);
        free(m_pImageOnDisk);
        remove(g_imageFilename);
        remove(g_savFilenameAllOnes);
        remove(g_savFilenameAllZeroes);
        remove(g_usrFilenameAllOnes);
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
    
    void validateAllOnes(const unsigned char* pBuffer, size_t bufferSize)
    {
        for (size_t i = 0 ; i < bufferSize ; i++)
            LONGS_EQUAL(0xff, *pBuffer++);
    }
    
    void validateBlocksAreZeroes(const unsigned char* pImage, unsigned int startBlock, unsigned int endBlock)
    {
        unsigned char expectedBlock[DISK_IMAGE_BLOCK_SIZE];
        memset(expectedBlock, 0x00, sizeof(expectedBlock));
        validateBlocks(pImage, startBlock, endBlock, expectedBlock);
    }
    
    void validateBlocksAreOnes(const unsigned char* pImage, unsigned int startBlock, unsigned int endBlock)
    {
        unsigned char expectedBlock[DISK_IMAGE_BLOCK_SIZE];
        memset(expectedBlock, 0xff, sizeof(expectedBlock));
        validateBlocks(pImage, startBlock, endBlock, expectedBlock);
    }
    
    void validateBlocks(const unsigned char* pImage, 
                        unsigned int         startBlock, 
                        unsigned int         endBlock, 
                        unsigned char*       pExpectedBlockContent)
    {
        unsigned int  blockCount = endBlock - startBlock + 1;

        pImage += startBlock * DISK_IMAGE_BLOCK_SIZE;
        for (unsigned int i = 0 ; i < blockCount ; i++)
        {
            CHECK_TRUE( 0 == memcmp(pImage, pExpectedBlockContent, DISK_IMAGE_BLOCK_SIZE) );
            pImage += DISK_IMAGE_BLOCK_SIZE;
        }
    }
    
    void validateRW18SectorsAreZeroes(const unsigned char* pImage, 
                                      unsigned short startSide, unsigned short startTrack, unsigned short startSector,
                                      unsigned short endSide, unsigned short endTrack, unsigned short endSector)
    {
        unsigned char expectedSector[DISK_IMAGE_BYTES_PER_SECTOR];
        memset(expectedSector, 0x00, sizeof(expectedSector));
        validateRW18Sectors(pImage, startSide, startTrack, startSector, endSide, endTrack, endSector, expectedSector);
    }
    
    void validateRW18SectorsAreOnes(const unsigned char* pImage, 
                                    unsigned short startSide, unsigned short startTrack, unsigned short startSector,
                                    unsigned short endSide, unsigned short endTrack, unsigned short endSector)
    {
        unsigned char expectedSector[DISK_IMAGE_BYTES_PER_SECTOR];
        memset(expectedSector, 0xff, sizeof(expectedSector));
        validateRW18Sectors(pImage, startSide, startTrack, startSector, endSide, endTrack, endSector, expectedSector);
    }
    
    void validateRW18Sectors(const unsigned char* pImage, 
                             unsigned short       startSide, unsigned short startTrack, unsigned short startSector,
                             unsigned short       endSide, unsigned short endTrack, unsigned short endSector,
                             unsigned char*       pExpectedSectorContent)
    {
        startSide = rw18SideToIndex(startSide);
        endSide = rw18SideToIndex(endSide);
        
        for (unsigned short side = startSide ; side <= endSide ; side++)
        {
            const unsigned char* pStartOfSide = pImage + startOffsetForSide(side);
            unsigned short firstTrackForSide = (side == startSide) ? startTrack : 0;
            unsigned short lastTrackForSide = (side == endSide) ? endTrack : DISK_IMAGE_TRACKS_PER_SIDE - 1;
            
            for (unsigned short track = firstTrackForSide ; track <= lastTrackForSide ; track++)
            {
                const unsigned char* pStartOfTrack = pStartOfSide + track * DISK_IMAGE_RW18_BYTES_PER_TRACK;
                unsigned short startSectorForTrack = (side == startSide && track == firstTrackForSide) ? startSector : 0;
                unsigned short endSectorForTrack = (side == endSide && track == lastTrackForSide) ? endSector : 17;
                
                for (unsigned short sector = startSectorForTrack ; sector <= endSectorForTrack ; sector++)
                {
                    CHECK( 0 == memcmp(pStartOfTrack + sector * DISK_IMAGE_BYTES_PER_SECTOR, 
                                       pExpectedSectorContent, 
                                       DISK_IMAGE_BYTES_PER_SECTOR) );
                }
            }
        }
    }
    
    unsigned short rw18SideToIndex(unsigned short side)
    {
        switch (side)
        {
        case 0xa9:
            return 0;
        case 0xad:
            return 1;
        case 0x79:
            return 2;
        }
        assert ( 0 );
        return 0xFFFF;
    }
    
    size_t startOffsetForSide(unsigned short side)
    {
        switch (side)
        {
        case 0:
            return 16 * DISK_IMAGE_BLOCK_SIZE;
        case 1:
            return (16 + 315 + 1) * DISK_IMAGE_BLOCK_SIZE;
        case 2:
            return (16 + 315 + 1 + 315) * DISK_IMAGE_BLOCK_SIZE;
        default:
            return 0;
        }
    }
    
    void writeOnesBlocks(unsigned int startBlock, unsigned int blockCount)
    {
        unsigned int   totalSize = blockCount * DISK_IMAGE_BLOCK_SIZE;
        unsigned char* pBlockData = (unsigned char*)malloc(totalSize);
        CHECK_TRUE(pBlockData != NULL);
        memset(pBlockData, 0xff, totalSize);
        
        DiskImageInsert insert;
        insert.sourceOffset = 0;
        insert.length = totalSize;
        insert.type = DISK_IMAGE_INSERTION_BLOCK;
        insert.block = startBlock;
        insert.intraBlockOffset = 0;

        __try_and_catch( BlockDiskImage_InsertData(m_pDiskImage, pBlockData, &insert) );
        free(pBlockData);
    }
    
    void writeOnesRW18Sectors(unsigned short side, unsigned short startTrack, unsigned short startSector, unsigned short sectorCount)
    {
        unsigned int   totalSize = sectorCount * DISK_IMAGE_BYTES_PER_SECTOR;
        unsigned char* pBlockData = (unsigned char*)malloc(totalSize);
        memset(pBlockData, 0xff, totalSize);
        
        DiskImageInsert insert;
        insert.sourceOffset = 0;
        insert.length = totalSize;
        insert.type = DISK_IMAGE_INSERTION_RWTS18;
        insert.side = side;
        insert.track = startTrack;
        insert.sector = startSector;
        insert.intraSectorOffset = 0;

        __try_and_catch( BlockDiskImage_InsertData(m_pDiskImage, pBlockData, &insert) );
        free(pBlockData);
    }
    
    const unsigned char* readDiskImageIntoMemory(void)
    {
        m_pFile = fopen(g_imageFilename, "r");
        CHECK(m_pFile != NULL);
        LONGS_EQUAL(BLOCK_DISK_IMAGE_3_5_DISK_SIZE, getFileSize(m_pFile));
        
        m_pImageOnDisk = (unsigned char*)malloc(BLOCK_DISK_IMAGE_3_5_DISK_SIZE);
        CHECK(m_pImageOnDisk != NULL);
        LONGS_EQUAL(BLOCK_DISK_IMAGE_3_5_DISK_SIZE, fread(m_pImageOnDisk, 1, BLOCK_DISK_IMAGE_3_5_DISK_SIZE, m_pFile));

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
    
    void createOnesBlockObjectFile()
    {
        unsigned char blockData[DISK_IMAGE_BLOCK_SIZE];
        memset(blockData, 0xff, sizeof(blockData));
        createBlockObjectFile(g_savFilenameAllOnes, blockData, sizeof(blockData));
    }
    
    void createZeroesBlockObjectFile()
    {
        unsigned char blockData[DISK_IMAGE_BLOCK_SIZE];
        memset(blockData, 0x00, sizeof(blockData));
        createBlockObjectFile(g_savFilenameAllZeroes, blockData, sizeof(blockData));
    }

    void createBlockObjectFile(const char* pFilename, unsigned char* pBlockData, size_t blockDataSize)
    {
        SavFileHeader header;
    
        memcpy(header.signature, BINARY_BUFFER_SAV_SIGNATURE, sizeof(header.signature));
        header.address = 0;
        header.length = blockDataSize;
    
        FILE* pFile = fopen(pFilename, "w");
        fwrite(&header, 1, sizeof(header), pFile);
        fwrite(pBlockData, 1, blockDataSize, pFile);
        fclose(pFile);
    }
    
    void createOnesSectorUSRObjectFile(unsigned short side, 
                                       unsigned short track, 
                                       unsigned short sector, 
                                       unsigned short offset)
    {
        unsigned char sectorData[DISK_IMAGE_BYTES_PER_SECTOR];
        memset(sectorData, 0xff, sizeof(sectorData));
        createSectorUSRObjectFile(g_usrFilenameAllOnes, sectorData, sizeof(sectorData),
                                  side, track, sector, offset);
    }
    
    void createSectorUSRObjectFile(const char*    pFilename, 
                                   unsigned char* pSectorData, 
                                   size_t         sectorDataSize,
                                   unsigned short side,
                                   unsigned short track,
                                   unsigned short sector,
                                   unsigned short offset)
    {
        RW18SavFileHeader header;
    
        memcpy(header.signature, BINARY_BUFFER_RW18SAV_SIGNATURE, sizeof(header.signature));
        header.side = side;
        header.track = track;
        header.offset = DISK_IMAGE_BYTES_PER_SECTOR * sector + offset;
        header.length = sectorDataSize;
    
        FILE* pFile = fopen(pFilename, "w");
        fwrite(&header, 1, sizeof(header), pFile);
        fwrite(pSectorData, 1, sectorDataSize, pFile);
        fclose(pFile);
    }
    
    void createOnesBlockRawObjectFile()
    {
        unsigned char blockData[DISK_IMAGE_BLOCK_SIZE];
        memset(blockData, 0xff, sizeof(blockData));
        createBlockRawObjectFile(g_savFilenameAllOnes, blockData, sizeof(blockData));
    }

    void createBlockRawObjectFile(const char* pFilename, unsigned char* pBlockData, size_t blockDataSize)
    {
        FILE* pFile = fopen(pFilename, "w");
        fwrite(pBlockData, 1, blockDataSize, pFile);
        fclose(pFile);
    }

    void createTextFile(const char* pFilename, const char* pText)
    {
        FILE* pFile = fopen(pFilename, "w");
        fwrite(pText, 1, strlen(pText), pFile);
        fclose(pFile);
    }
};


TEST(BlockDiskImage, FailAllAllocationInCreate)
{
    static const int allocationsToFail = 3;
    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT) );
        validateOutOfMemoryExceptionThrown();
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    CHECK_TRUE(m_pDiskImage != NULL);
}

TEST(BlockDiskImage, VerifyCreateStartsWithZeroesInImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    CHECK_TRUE(NULL != m_pDiskImage);
    
    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    CHECK_TRUE(NULL != pImage);
    LONGS_EQUAL(BLOCK_DISK_IMAGE_3_5_DISK_SIZE, BlockDiskImage_GetImageSize(m_pDiskImage));
    validateAllZeroes(pImage, BLOCK_DISK_IMAGE_3_5_DISK_SIZE);
}

TEST(BlockDiskImage, InsertOnesBlockInFirstBlock)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesBlocks(0, 1);

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, InsertOnesBlockInLastBlock)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesBlocks(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1, 1);

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 0, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 2);
    validateBlocksAreOnes(pImage, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
}

TEST(BlockDiskImage, FailToInsertBlockJustPastEndOfImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesBlocks(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT, 1);
    validateExceptionThrown(blockExceedsImageBoundsException);
}

TEST(BlockDiskImage, FailToInsertBlocksWhichExtendPastEndOfImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesBlocks(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT-1, 2);
    validateExceptionThrown(blockExceedsImageBoundsException);
}

TEST(BlockDiskImage, InsertOnesSectorInFirstRW18Sector)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesRW18Sectors(DISK_IMAGE_RW18_SIDE_0, 0, 0, 1);

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateRW18SectorsAreZeroes(pImage, DISK_IMAGE_RW18_SIDE_0, 0, 1, 
                                         DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17);
    validateRW18SectorsAreOnes(pImage, DISK_IMAGE_RW18_SIDE_0, 0, 0, DISK_IMAGE_RW18_SIDE_0, 0, 0);
}

TEST(BlockDiskImage, InsertOnesSectorInFirstRW18SectorOfSide1)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesRW18Sectors(DISK_IMAGE_RW18_SIDE_1, 0, 0, 1);

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateRW18SectorsAreZeroes(pImage, DISK_IMAGE_RW18_SIDE_0, 0, 0, 
                                         DISK_IMAGE_RW18_SIDE_0, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17);
    validateRW18SectorsAreOnes(pImage, DISK_IMAGE_RW18_SIDE_1, 0, 0, DISK_IMAGE_RW18_SIDE_1, 0, 0);
    validateRW18SectorsAreZeroes(pImage, DISK_IMAGE_RW18_SIDE_1, 0, 1, 
                                         DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17);
}

TEST(BlockDiskImage, InsertOnesSectorInLastRW18Sector)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesRW18Sectors(DISK_IMAGE_RW18_SIDE_2, 34, 17, 1);

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateRW18SectorsAreZeroes(pImage, DISK_IMAGE_RW18_SIDE_0, 0, 0, 
                                         DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 16);
    validateRW18SectorsAreOnes(pImage, DISK_IMAGE_RW18_SIDE_2, 34, 17, DISK_IMAGE_RW18_SIDE_2, 34, 17);
}

TEST(BlockDiskImage, FailToInsertRW18SectorToInvalidSide)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesRW18Sectors(0xff, 0, 0, 1);
    validateExceptionThrown(invalidSideException);
}

TEST(BlockDiskImage, FailToInsertRW18SectorToInvalidTrack)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesRW18Sectors(DISK_IMAGE_RW18_SIDE_0, 35, 0, 1);
    validateExceptionThrown(invalidTrackException);
}

TEST(BlockDiskImage, FailToInsertRW18SectorToInvalidSector)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesRW18Sectors(DISK_IMAGE_RW18_SIDE_0, 0, 18, 1);
    validateExceptionThrown(invalidSectorException);
}

TEST(BlockDiskImage, WriteImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesBlocks(0, 1);
    BlockDiskImage_WriteImage(m_pDiskImage, g_imageFilename);
    
    const unsigned char* pImage = readDiskImageIntoMemory();
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, FailFOpenInWriteImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesBlocks(0, 1);
    fopenFail(NULL);
        __try_and_catch( BlockDiskImage_WriteImage(m_pDiskImage, g_imageFilename) );
    fopenRestore();
    validateFileExceptionThrown();
}

TEST(BlockDiskImage, FailFWriteInWriteImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    writeOnesBlocks(0, 1);
    fwriteFail(0);
        __try_and_catch( BlockDiskImage_WriteImage(m_pDiskImage, g_imageFilename) );
    fwriteRestore();
    validateFileExceptionThrown();
}

TEST(BlockDiskImage, ReadSAVObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);
}

TEST(BlockDiskImage, ReadRawObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockRawObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);
}

TEST(BlockDiskImage, ReadUSRObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_0, 0, 0, 0);
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_usrFilenameAllOnes);
}

TEST(BlockDiskImage, FailSecondHeaderReadInReadUSRObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_0, 0, 0, 0);
    
    freadFail(0);
    freadToFail(2);
        __try_and_catch( BlockDiskImage_ReadObjectFile(m_pDiskImage, g_usrFilenameAllOnes) );
    freadRestore();
    validateFileExceptionThrown();
}

TEST(BlockDiskImage, FailFOpenForNoExistingFileInReadObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    __try_and_catch( BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes) );
    validateFileExceptionThrown();
}

TEST(BlockDiskImage, FailHeaderReadInReadObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    
    freadFail(0);
        __try_and_catch( BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes) );
    freadRestore();
    validateFileExceptionThrown();
}

TEST(BlockDiskImage, FailAllocationInReadObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    
    MallocFailureInject_FailAllocation(1);
    __try_and_catch( BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes) );
    validateOutOfMemoryExceptionThrown();
}

TEST(BlockDiskImage, FailDataReadInReadObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    
    freadFail(0);
    freadToFail(2);
        __try_and_catch( BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes) );
    freadRestore();
    validateFileExceptionThrown();
}

TEST(BlockDiskImage, ReadObjectFileAndWriteToImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = 0;
    insert.intraBlockOffset = 0;
    BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert);
    
    BlockDiskImage_WriteImage(m_pDiskImage, g_imageFilename);
    const unsigned char* pImage = readDiskImageIntoMemory();
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, ReadRawObjectFileAndWriteToImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockRawObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = 0;
    insert.intraBlockOffset = 0;
    BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert);
    
    BlockDiskImage_WriteImage(m_pDiskImage, g_imageFilename);
    const unsigned char* pImage = readDiskImageIntoMemory();
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, InvalidTypeForInsertObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_RWTS16;
    insert.block = 0;
    insert.intraBlockOffset = 0;
    __try_and_catch( BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert) );
    validateExceptionThrown(invalidInsertionTypeException);
}

TEST(BlockDiskImage, OutOfBoundsStartingOffsetForInsertObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = DISK_IMAGE_BLOCK_SIZE;
    insert.length = 1;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = 0;
    insert.intraBlockOffset = 0;
    __try_and_catch( BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert) );
    validateExceptionThrown(invalidSourceOffsetException);
}

TEST(BlockDiskImage, OutOfBoundsEndingOffsetOnInputObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = 1;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = 0;
    insert.intraBlockOffset = 0;
    __try_and_catch( BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert) );
    validateExceptionThrown(invalidLengthException);
}

TEST(BlockDiskImage, VerifyRoundUpToBlockForInsertObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    unsigned char onesSectorData[DISK_IMAGE_BLOCK_SIZE + 1];
    memset(onesSectorData, 0xff, sizeof(onesSectorData));
    onesSectorData[sizeof(onesSectorData) - 1] = 0x00;
    createBlockObjectFile(g_savFilenameAllOnes, onesSectorData, sizeof(onesSectorData));
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = DISK_IMAGE_BLOCK_SIZE;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = 0;
    insert.intraBlockOffset = 0;
    BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert);

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 0, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
}

TEST(BlockDiskImage, OutOfBoundsBlockForInsertObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT;
    insert.intraBlockOffset = 0;
    __try_and_catch( BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert) );
    validateExceptionThrown(blockExceedsImageBoundsException);
}

TEST(BlockDiskImage, OutOfBoundIntraBlockOffsetForInsertObjectFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT;
    insert.intraBlockOffset = DISK_IMAGE_BLOCK_SIZE;
    __try_and_catch( BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert) );
    validateExceptionThrown(invalidIntraBlockOffsetException);
}

TEST(BlockDiskImage, ReadTwoObjectFilesAndOnlyWriteSecondToImage)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createZeroesBlockObjectFile();
    createOnesBlockObjectFile();
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllZeroes);
    BlockDiskImage_ReadObjectFile(m_pDiskImage, g_savFilenameAllOnes);

    DiskImageInsert insert;
    insert.sourceOffset = 0;
    insert.length = DISK_IMAGE_BLOCK_SIZE;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = 0;
    insert.intraBlockOffset = 0;
    BlockDiskImage_InsertObjectFile(m_pDiskImage, &insert);
    
    BlockDiskImage_WriteImage(m_pDiskImage, g_imageFilename);
    const unsigned char* pImage = readDiskImageIntoMemory();
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, ProcessOneLineTextScriptWithAsteriskForLength)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,*,0\n"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, ProcessOneLineTextScriptWithNoNewLineAtEnd)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,512,0"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, ProcessOneLineTextScriptWithComment)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("#BLOCK,BlockDiskImageTestOnes.sav,0,512,0"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 0, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    STRCMP_EQUAL("", printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, ProcessTextScriptWithOptionalBlockOffset)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,256,0,256"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateAllZeroes(pImage, 256);
    validateAllZeroes(pImage + 256 + 256, BlockDiskImage_GetImageSize(m_pDiskImage) - 512);
    validateAllOnes(pImage + 256, 256);
}

TEST(BlockDiskImage, ProcessTextScriptWithMaximumOptionalBlockOffset)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,1,0,511"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateAllZeroes(pImage, 511);
    validateAllZeroes(pImage + 512, BlockDiskImage_GetImageSize(m_pDiskImage) - 512);
    validateAllOnes(pImage + 511, 1);
}

TEST(BlockDiskImage, ProcessTwoLineTextScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,512,0\n"
                                                    "BLOCK,BlockDiskImageTestOnes.sav,0,512,1599\n"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 2);
    validateBlocksAreOnes(pImage, 0, 0);
    validateBlocksAreOnes(pImage, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
}

TEST(BlockDiskImage, ProcessTwoLineTextScriptUsingAsteriskToStartFromLastInsertionOr0OnFirstLine)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,255,*\n"
                                                    "BLOCK,BlockDiskImageTestOnes.sav,0,257,*\n"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
    validateBlocksAreOnes(pImage, 0, 0);
}

TEST(BlockDiskImage, ProcessOneRW18LineTextScriptWithAsteriskForAllFieldsSoThatFileHeaderFieldsAreUsed)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 0);

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS18,BlockDiskImageTestOnes.usr,0,*,*,*,*,*\n"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateRW18SectorsAreZeroes(pImage, DISK_IMAGE_RW18_SIDE_0, 0, 0, 
                                         DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 16);
    validateRW18SectorsAreOnes(pImage, DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 
                                       DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17);
}

TEST(BlockDiskImage, ProcessOneRW18LineTextScriptWithOverridesForAllFileHeaderFields)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 0);

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS18,BlockDiskImageTestOnes.usr,0,*,0xa9,0,0,0\n"));

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateRW18SectorsAreOnes(pImage, DISK_IMAGE_RW18_SIDE_0, 0, 0, DISK_IMAGE_RW18_SIDE_0, 0, 0);
    validateRW18SectorsAreZeroes(pImage, DISK_IMAGE_RW18_SIDE_0, 0, 1, 
                                         DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17);
}

TEST(BlockDiskImage, FailTextFileCreateInProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    MallocFailureInject_FailAllocation(1);
    __try_and_catch( BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,512,0\n")) );
    validateOutOfMemoryExceptionThrown();
}

TEST(BlockDiskImage, PassInvalidBlankLineToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("\n"));
    STRCMP_EQUAL("<null>:1: error: Script line cannot be blank.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidScriptInsertionTokenToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("foo.bar"));
    STRCMP_EQUAL("<null>:1: error: foo.bar isn't a recognized image insertion type of BLOCK or RWTS16.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassTooFewBlockTokensToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,512\n"));
    STRCMP_EQUAL("<null>:1: error: Line doesn't contain correct fields: BLOCK,objectFilename,objectStartOffset,insertionLength,block[,intraBlockOffset]\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassTooManyTokensToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,512,0,0,0\n"));
    STRCMP_EQUAL("<null>:1: error: Line doesn't contain correct fields: BLOCK,objectFilename,objectStartOffset,insertionLength,block[,intraBlockOffset]\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidFilenameToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,InvalidFilename.sav,0,512,0\n"));
    STRCMP_EQUAL("<null>:1: error: Failed to read 'InvalidFilename.sav' object file.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidBlockToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,512,1600\n"));
    STRCMP_EQUAL("<null>:1: error: Write starting at block 1600 offset 0 won't fit in output image file.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidIntraBlockOffsetToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("BLOCK,BlockDiskImageTestOnes.sav,0,512,0,512\n"));
    STRCMP_EQUAL("<null>:1: error: 512 specifies an invalid intra block offset.  Must be 0 - 511.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidInsertionTypeToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS16,BlockDiskImageTestOnes.sav,0,512,0,0\n"));
    STRCMP_EQUAL("<null>:1: error: RWTS16 insertion type isn't supported for this output image type.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassTooFewRWTS18TokensToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 0);

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS18,BlockDiskImageTestOnes.usr,0,*,0xa9,0,0\n"));
    STRCMP_EQUAL("<null>:1: error: Line doesn't contain correct fields: RWTS18,objectFilename,objectStartOffset,insertionLength,side,track,sector,offset\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassTooManyRWTS18TokensToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 0);

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS18,BlockDiskImageTestOnes.usr,0,*,0xa9,0,0,0,0\n"));
    STRCMP_EQUAL("<null>:1: error: Line doesn't contain correct fields: RWTS18,objectFilename,objectStartOffset,insertionLength,side,track,sector,offset\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidRWTS18FilenameToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 0);

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS18,InvalidFilename.usr,0,*,0xa9,0,0,0\n"));
    STRCMP_EQUAL("<null>:1: error: Failed to read 'InvalidFilename.usr' object file.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidRWTS18SectorOffsetToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 0);

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS18,BlockDiskImageTestOnes.usr,0,*,0xa9,0,0,256\n"));
    STRCMP_EQUAL("<null>:1: error: 256 specifies an invalid intra sector offset.  Must be 0 - 255.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, PassInvalidRWTS18SideToProcessScript)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesSectorUSRObjectFile(DISK_IMAGE_RW18_SIDE_2, DISK_IMAGE_TRACKS_PER_SIDE - 1, 17, 0);

    BlockDiskImage_ProcessScript(m_pDiskImage, copy("RWTS18,BlockDiskImageTestOnes.usr,0,*,0,0,0,0\n"));
    STRCMP_EQUAL("<null>:1: error: 0x0 specifies an invalid side.  Must be 0xa9, 0xad, 0x79.\n",
                 printfSpy_GetLastErrorOutput());
}

TEST(BlockDiskImage, ProcessTwoLineScriptFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    createTextFile(g_scriptFilename, "BLOCK,BlockDiskImageTestOnes.sav,0,512,0\n"
                                     "BLOCK,BlockDiskImageTestOnes.sav,0,512,1599\n");

    BlockDiskImage_ProcessScriptFile(m_pDiskImage, g_scriptFilename);

    const unsigned char* pImage = BlockDiskImage_GetImagePointer(m_pDiskImage);
    validateBlocksAreZeroes(pImage, 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 2);
    validateBlocksAreOnes(pImage, 0, 0);
    validateBlocksAreOnes(pImage, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1, BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT - 1);
}

TEST(BlockDiskImage, FailToAllocateTextFileInProcessScriptFile)
{
    m_pDiskImage = BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    createOnesBlockObjectFile();
    createTextFile(g_scriptFilename, "BLOCK,BlockDiskImageTestOnes.sav,0,512,0\n"
                                     "BLOCK,BlockDiskImageTestOnes.sav,0,512,1599\n");

    MallocFailureInject_FailAllocation(1);
    __try_and_catch( BlockDiskImage_ProcessScriptFile(m_pDiskImage, g_scriptFilename) );
    validateOutOfMemoryExceptionThrown();
}
