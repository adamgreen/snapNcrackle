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
#include <assert.h>
#include <string.h>
#include "DiskImagePriv.h"
#include "DiskImageTest.h"
#include "BinaryBuffer.h"
#include "util.h"


#define IMAGE_TABLE_DEFAULT_ADDRESS 0x6000

static void DiskImageScriptEngine_Init(DiskImageScriptEngine* pThis);
__throws void DiskImage_Init(DiskImage* pThis, DiskImageVTable* pVTable, unsigned int imageSize)
{
    memset(pThis, 0, sizeof(*pThis));
    pThis->pVTable = pVTable;
    ByteBuffer_Allocate(&pThis->image, imageSize);
    DiskImageScriptEngine_Init(&pThis->script);
}

static void DiskImageScriptEngine_Init(DiskImageScriptEngine* pThis)
{
    pThis->pParser = ParseCSV_Create();
}


static void DiskImageScriptEngine_Free(DiskImageScriptEngine* pThis);
static void closeTextFile(DiskImageScriptEngine* pThis);
void DiskImage_Free(DiskImage* pThis)
{
    if (!pThis)
        return;
    
    if (pThis->pVTable)
        pThis->pVTable->freeObject(pThis);
    ByteBuffer_Free(&pThis->object);
    ByteBuffer_Free(&pThis->image);
    DiskImageScriptEngine_Free(&pThis->script);
    free(pThis);
}

static void DiskImageScriptEngine_Free(DiskImageScriptEngine* pThis)
{
    ParseCSV_Free(pThis->pParser);
    closeTextFile(pThis);
}

static void closeTextFile(DiskImageScriptEngine* pThis)
{
    TextFile_Free(pThis->pTextFile);
    pThis->pTextFile = NULL;
}


#define LOG_ERROR(pTHIS, FORMAT, ...) fprintf(stderr, \
                                       "%s:%d: error: " FORMAT LINE_ENDING, \
                                       pTHIS->pScriptFilename, \
                                       pTHIS->lineNumber, \
                                       __VA_ARGS__)

static void DiskImageScriptEngine_ProcessScriptFile(DiskImageScriptEngine* pThis, 
                                                    DiskImage*              pDiskImage, 
                                                    const char*             pScriptFilename);
static void processScriptFromTextFile(DiskImageScriptEngine* pThis);
static int isLineAComment(const SizedString* pLine);
static void processNextScriptLine(DiskImageScriptEngine* pThis, const SizedString* pScriptLine);
static void processBlockScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields);
static void readObjectFile(DiskImage* pDiskImage, const SizedString* pFilenameString);
static unsigned int parseLengthField(DiskImageScriptEngine* pThis, const SizedString* pLengthField);
static unsigned int parseFieldWhichSupportsAsteriskForDefaulValue(DiskImageScriptEngine* pThis, 
                                                                  const SizedString*     pField, 
                                                                  unsigned int           defaultValue);
static void parseBlockRelatedFieldsAndSetInsertFields(DiskImageScriptEngine* pThis, 
                                                      size_t fieldCount, 
                                                      const SizedString* pFields);
static int isAsterisk(const SizedString* pString);
static void setBlockInsertFieldsBasedOnLastInsertion(DiskImageScriptEngine* pThis);
static void setBlockInsertFieldsBaseOnScriptFields(DiskImageScriptEngine* pThis, 
                                                   size_t fieldCount, 
                                                   const SizedString* pFields);
static void rememberLastInsertionInformation(DiskImageScriptEngine* pThis);
static void processRWTS16ScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields);
static void processRWTS16CPScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields);
static void processRW18ScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields);
static void processImageTableUpdates(DiskImageScriptEngine* pThis, unsigned short newImageTableAddress);
static unsigned short getImageTableObjectSize(DiskImage* pDiskImage, unsigned short startImageTableAddress);
static void reportScriptLineException(DiskImageScriptEngine* pThis);
__throws void DiskImage_ProcessScriptFile(DiskImage* pThis, const char* pScriptFilename)
{
    DiskImageScriptEngine_ProcessScriptFile(&pThis->script, pThis, pScriptFilename);
}

static void DiskImageScriptEngine_ProcessScriptFile(DiskImageScriptEngine* pThis, 
                                                    DiskImage*             pDiskImage, 
                                                    const char*            pScriptFilename)
{
    __try
    {
        SizedString scriptFilename = SizedString_InitFromString(pScriptFilename);
        pThis->pScriptFilename = pScriptFilename;
        pThis->pDiskImage = pDiskImage;
        pThis->pTextFile = TextFile_CreateFromFile(NULL, &scriptFilename, NULL);
    }
    __catch
    {
        LOG_ERROR(pThis, "Failed to open %s for parsing.", pScriptFilename);
        __rethrow;
    }
    
    processScriptFromTextFile(pThis);
}

static void processScriptFromTextFile(DiskImageScriptEngine* pThis)
{
    pThis->lineNumber = 1;
    while (!TextFile_IsEndOfFile(pThis->pTextFile))
    {
        SizedString nextLine = TextFile_GetNextLine(pThis->pTextFile);
        if (!isLineAComment(&nextLine))
            processNextScriptLine(pThis, &nextLine);
        pThis->lineNumber++;
    }
    closeTextFile(pThis);
}

static int isLineAComment(const SizedString* pLine)
{
    return pLine->pString[0] == '#';
}

static void processNextScriptLine(DiskImageScriptEngine* pThis, const SizedString* pScriptLine)
{
    size_t             fieldCount;
    const SizedString* pFields;
    
    ParseCSV_Parse(pThis->pParser, pScriptLine);
    fieldCount = ParseCSV_FieldCount(pThis->pParser);
    pFields = ParseCSV_FieldPointers(pThis->pParser);
    if (fieldCount < 1 || SizedString_strlen(&pFields[0]) == 0)
    {
        LOG_ERROR(pThis, "%s cannot be blank.", "Script line");
        return;
    }
    
    __try
    {
        if (0 == SizedString_strcasecmp(&pFields[0], "block"))
            processBlockScriptLine(pThis, fieldCount, pFields);
        else if (0 == SizedString_strcasecmp(&pFields[0], "rwts16"))
            processRWTS16ScriptLine(pThis, fieldCount, pFields);
        else if (0 == SizedString_strcasecmp(&pFields[0], "rwts16cp"))
            processRWTS16CPScriptLine(pThis, fieldCount, pFields);
        else if (0 == SizedString_strcasecmp(&pFields[0], "rw18"))
            processRW18ScriptLine(pThis, fieldCount, pFields);
        else
            LOG_ERROR(pThis, "%.*s isn't a recognized image insertion type of BLOCK or RWTS16.", 
                      pFields[0].stringLength, pFields[0].pString);
    }
    __catch
    {
        reportScriptLineException(pThis);
        __nothrow;
    }
}

static void processBlockScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields)
{
    if (fieldCount < 5 || fieldCount > 6)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: BLOCK,objectFilename,objectStartOffset,insertionLength,block[,intraBlockOffset]",
                  "Line");
        __throw(invalidArgumentException);
    }
    
    readObjectFile(pThis->pDiskImage, &pFields[1]);
    pThis->insert.sourceOffset = SizedString_strtoul(&pFields[2], NULL, 0);
    pThis->insert.length = parseLengthField(pThis, &pFields[3]);
    pThis->insert.type = DISK_IMAGE_INSERTION_BLOCK;
    parseBlockRelatedFieldsAndSetInsertFields(pThis, fieldCount, pFields);
    rememberLastInsertionInformation(pThis);
    DiskImage_InsertObjectFile(pThis->pDiskImage, &pThis->insert);
}

static void readObjectFile(DiskImage* pDiskImage, const SizedString* pFilenameString)
{
    char* pFilename = NULL;
    
    __try
    {
        pFilename = SizedString_strdup(pFilenameString);
        DiskImage_ReadObjectFile(pDiskImage, pFilename);
    }
    __catch
    {
        free(pFilename);
        __rethrow;
    }
    free(pFilename);
}

static unsigned int parseLengthField(DiskImageScriptEngine* pThis, const SizedString* pLengthField)
{
    return parseFieldWhichSupportsAsteriskForDefaulValue(pThis, pLengthField, pThis->pDiskImage->objectFileLength);
}

static unsigned int parseFieldWhichSupportsAsteriskForDefaulValue(DiskImageScriptEngine* pThis, 
                                                                  const SizedString*     pField, 
                                                                  unsigned int           defaultValue)
{
    if (isAsterisk(pField))
        return defaultValue;
    else
        return SizedString_strtoul(pField, NULL, 0);
}

static int isAsterisk(const SizedString* pString)
{
    return 0 == SizedString_strcmp(pString, "*");
}

static void parseBlockRelatedFieldsAndSetInsertFields(DiskImageScriptEngine* pThis, 
                                                      size_t fieldCount, 
                                                      const SizedString* pFields)
{
    const SizedString* pBlockField = &pFields[4];

    if (isAsterisk(pBlockField))
        setBlockInsertFieldsBasedOnLastInsertion(pThis);
    else
        setBlockInsertFieldsBaseOnScriptFields(pThis, fieldCount, pFields);
}

static void setBlockInsertFieldsBasedOnLastInsertion(DiskImageScriptEngine* pThis)
{
    unsigned int lastOffset = pThis->lastBlock * DISK_IMAGE_BLOCK_SIZE + pThis->lastLength;
    pThis->insert.block = lastOffset / DISK_IMAGE_BLOCK_SIZE;
    pThis->insert.intraBlockOffset = lastOffset % DISK_IMAGE_BLOCK_SIZE;
}

static void setBlockInsertFieldsBaseOnScriptFields(DiskImageScriptEngine* pThis, 
                                                   size_t fieldCount, 
                                                   const SizedString* pFields)
{
    pThis->insert.block = SizedString_strtoul(&pFields[4], NULL, 0);
    if (fieldCount > 5)
        pThis->insert.intraBlockOffset = SizedString_strtoul(&pFields[5], NULL, 0);
    else
        pThis->insert.intraBlockOffset = 0;
}

static void rememberLastInsertionInformation(DiskImageScriptEngine* pThis)
{
    pThis->lastBlock = pThis->insert.block;
    pThis->lastLength = pThis->insert.length;
}

static void processRWTS16ScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields)
{
    if (fieldCount != 6)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: RWTS16,objectFilename,objectStartOffset,insertionLength,track,sector",
                  "Line");
        __throw(invalidArgumentException);
    }
    
    readObjectFile(pThis->pDiskImage, &pFields[1]);
    pThis->insert.sourceOffset = SizedString_strtoul(&pFields[2], NULL, 0);
    pThis->insert.length = parseLengthField(pThis, &pFields[3]);
    pThis->insert.type = DISK_IMAGE_INSERTION_RWTS16;
    pThis->insert.track = SizedString_strtoul(&pFields[4], NULL, 0);
    pThis->insert.sector = SizedString_strtoul(&pFields[5], NULL, 0);
    DiskImage_InsertObjectFile(pThis->pDiskImage, &pThis->insert);
}

static void processRWTS16CPScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields)
{
    if (fieldCount != 3)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: RWTS16CP,track,sector",
                  "Line");
        __throw(invalidArgumentException);
    }

    pThis->pDiskImage->objectFileLength = 0;
    pThis->insert.sourceOffset = 0;
    pThis->insert.length = 0;
    pThis->insert.type = DISK_IMAGE_INSERTION_RWTS16CP;
    pThis->insert.track = SizedString_strtoul(&pFields[1], NULL, 0);
    pThis->insert.sector = SizedString_strtoul(&pFields[2], NULL, 0);
    DiskImage_InsertObjectFile(pThis->pDiskImage, &pThis->insert);
}

static void processRW18ScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const SizedString* pFields)
{
    if (fieldCount < 7 || fieldCount > 8)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: "
                    "RW18,objectFilename,objectStartOffset,insertionLength,side,track,offset[,imageTableAddress]",
                  "Line");
        __throw(invalidArgumentException);
    }
    
    readObjectFile(pThis->pDiskImage, &pFields[1]);
    pThis->insert.type = DISK_IMAGE_INSERTION_RW18;
    pThis->insert.sourceOffset = SizedString_strtoul(&pFields[2], NULL, 0);
    pThis->insert.length = parseLengthField(pThis, &pFields[3]);
    pThis->insert.side = parseFieldWhichSupportsAsteriskForDefaulValue(pThis, &pFields[4], 
                                                                       pThis->pDiskImage->insert.side);
    pThis->insert.track = parseFieldWhichSupportsAsteriskForDefaulValue(pThis, &pFields[5], 
                                                                        pThis->pDiskImage->insert.track);
    pThis->insert.intraTrackOffset = parseFieldWhichSupportsAsteriskForDefaulValue(pThis, &pFields[6], 
                                                                                   pThis->pDiskImage->insert.intraTrackOffset);
    if (fieldCount > 7)
        processImageTableUpdates(pThis, SizedString_strtoul(&pFields[7], NULL, 0));

    DiskImage_InsertObjectFile(pThis->pDiskImage, &pThis->insert);
}

static void processImageTableUpdates(DiskImageScriptEngine* pThis, unsigned short newImageTableAddress)
{
    unsigned short imageTableSize;

    DiskImage_UpdateImageTableFile(pThis->pDiskImage, newImageTableAddress);
    imageTableSize = getImageTableObjectSize(pThis->pDiskImage, newImageTableAddress);
    if (pThis->insert.length > imageTableSize)
        pThis->insert.length = imageTableSize;
}

static unsigned short getImageTableObjectSize(DiskImage* pDiskImage, unsigned short startImageTableAddress)
{
    unsigned char* pObject = pDiskImage->object.pBuffer;
    unsigned char  imageCount;
    unsigned short lastImageTableAddress;
    
    imageCount = *pObject++;
    pObject += (imageCount * 2);
    lastImageTableAddress = (unsigned short)*pObject++;
    lastImageTableAddress |= ((unsigned short)*pObject++) << 8;
    
    return (lastImageTableAddress - startImageTableAddress);
}

static void reportScriptLineException(DiskImageScriptEngine* pThis)
{
    const SizedString* pFields = ParseCSV_FieldPointers(pThis->pParser);
    int                exceptionCode = getExceptionCode();
    
    assert ( exceptionCode == fileOpenException ||
             exceptionCode == fileException || 
             exceptionCode == invalidArgumentException ||
             exceptionCode == blockExceedsImageBoundsException ||
             exceptionCode == invalidInsertionTypeException ||
             exceptionCode == invalidSideException ||
             exceptionCode == invalidSectorException ||
             exceptionCode == invalidTrackException ||
             exceptionCode == invalidIntraBlockOffsetException ||
             exceptionCode == invalidIntraTrackOffsetException ||
             exceptionCode == invalidSourceOffsetException ||
             exceptionCode == invalidLengthException );

    /* Note: invalidArgumentException prints error text before throwing. */
    if (exceptionCode == fileOpenException)
        LOG_ERROR(pThis, "Failed to open '%.*s' object file.", 
                  pFields[1].stringLength, pFields[1].pString);
    else if (exceptionCode == fileException)
        LOG_ERROR(pThis, "Failed to process '%.*s' object file.", 
                  pFields[1].stringLength, pFields[1].pString);
    else if (exceptionCode == blockExceedsImageBoundsException)
        LOG_ERROR(pThis, "Write starting at block %u offset %u won't fit in output image file.", 
                  pThis->insert.block, pThis->insert.intraBlockOffset);
    else if (exceptionCode == invalidInsertionTypeException)
        LOG_ERROR(pThis, "%.*s insertion type isn't supported for this output image type.", 
                  pFields[0].stringLength, pFields[0].pString);
    else if (exceptionCode == invalidSideException)
        LOG_ERROR(pThis, "0x%x specifies an invalid side.  Must be 0xa9, 0xad, 0x79.", pThis->insert.side);
    else if (exceptionCode == invalidSectorException)
        LOG_ERROR(pThis, "%u specifies an invalid sector.  Must be 0 - 15.", pThis->insert.sector);
    else if (exceptionCode == invalidTrackException)
        LOG_ERROR(pThis, "Write starting at track/sector %u/%u won't fit in output image file.", 
                  pThis->insert.track, pThis->insert.sector);
    else if (exceptionCode == invalidIntraBlockOffsetException)
        LOG_ERROR(pThis, "%u specifies an invalid intra block offset.  Must be 0 - 511.", 
                  pThis->insert.intraBlockOffset);
    else if (exceptionCode == invalidIntraTrackOffsetException)
        LOG_ERROR(pThis, "%u specifies an invalid intra track offset.  Must be 0 - 4607.", 
                  pThis->insert.intraTrackOffset);
    else if (exceptionCode == invalidSourceOffsetException)
        LOG_ERROR(pThis, "%u specifies an invalid source data offset.  Should be less than %u.", 
                  pThis->insert.sourceOffset,
                  pThis->pDiskImage->objectFileLength);
    else if (exceptionCode == invalidLengthException)
        LOG_ERROR(pThis, "%u specifies an invalid length.", 
                  pThis->insert.length);
}


static void DiskImageScriptEngine_ProcessScript(DiskImageScriptEngine* pThis, 
                                                DiskImage*             pDiskImage,
                                                char*                  pScriptText);
__throws void DiskImage_ProcessScript(DiskImage* pThis, char* pScriptText)
{
    DiskImageScriptEngine_ProcessScript(&pThis->script, pThis, pScriptText);
}

static void DiskImageScriptEngine_ProcessScript(DiskImageScriptEngine* pThis, 
                                                DiskImage*             pDiskImage,
                                                char*                  pScriptText)
{
    pThis->pDiskImage = pDiskImage;
    pThis->pScriptFilename = "<null>";
    pThis->pTextFile = TextFile_CreateFromString(pScriptText);

    processScriptFromTextFile(pThis);
}


static FILE* openFile(const char* pFilename, const char* pMode);
static void determineObjectSizeFromFileHeader(DiskImage* pThis, FILE* pFile);
static int wasSAVedFromAssembler(const char* pSignature);
static int wasRW18SAVedFromAssembler(const char* pSignature);
static void readInRW18SavHeaderToSetDefaultInsertOptions(DiskImage* pThis, FILE* pFile, void* pvPartialHeader);
static RW18SavFileHeader readInRestOfRW18FileHeader(DiskImage* pThis, FILE* pFile, void* pPartialHeader);
static long getFileSize(FILE* pFile);
static unsigned int roundUpLengthToBlockSize(unsigned int length);
__throws void DiskImage_ReadObjectFile(DiskImage* pThis, const char* pFilename)
{
    FILE*         pFile = NULL;
    unsigned int  roundedObjectSize;
    
    __try
    {
        pFile = openFile(pFilename, "rb");
        memset(&pThis->insert, 0, sizeof(pThis->insert));
        determineObjectSizeFromFileHeader(pThis, pFile);
        roundedObjectSize = roundUpLengthToBlockSize(pThis->objectFileLength);
        ByteBuffer_Allocate(&pThis->object, roundedObjectSize);
        ByteBuffer_ReadPartialFromFile(&pThis->object, pThis->objectFileLength, pFile);
    }
    __catch
    {
        if (pFile)
            fclose(pFile);
        __rethrow;
    }

    fclose(pFile);    
}

static FILE* openFile(const char* pFilename, const char* pMode)
{
    FILE* pFile = fopen(pFilename, pMode);
    if (!pFile)
        __throw(fileOpenException);
    return pFile;
}

static void determineObjectSizeFromFileHeader(DiskImage* pThis, FILE* pFile)
{
    SavFileHeader     header;
    size_t            bytesRead;
    
    bytesRead = fread(&header, 1, sizeof(header), pFile);
    if (bytesRead == sizeof(header) && wasSAVedFromAssembler(header.signature))
    {
        pThis->objectFileLength = header.length;
    }
    else if (bytesRead == sizeof(header) && wasRW18SAVedFromAssembler(header.signature))
    {
        readInRW18SavHeaderToSetDefaultInsertOptions(pThis, pFile, &header);
    }
    else
    {
        pThis->objectFileLength = getFileSize(pFile);
        fseek(pFile, 0, SEEK_SET);
    }
}

static int wasSAVedFromAssembler(const char* pSignature)
{
    return 0 == memcmp(pSignature, BINARY_BUFFER_SAV_SIGNATURE, 4);
}

static int wasRW18SAVedFromAssembler(const char* pSignature)
{
    return 0 == memcmp(pSignature, BINARY_BUFFER_RW18SAV_SIGNATURE, 4);
}

static void readInRW18SavHeaderToSetDefaultInsertOptions(DiskImage* pThis, FILE* pFile, void* pPartialHeader)
{
    RW18SavFileHeader rw18Header = readInRestOfRW18FileHeader(pThis, pFile, pPartialHeader);

    pThis->objectFileLength = rw18Header.length;
    pThis->insert.type = DISK_IMAGE_INSERTION_RW18;
    pThis->insert.length = rw18Header.length;
    pThis->insert.side = rw18Header.side;
    pThis->insert.track = rw18Header.track;
    pThis->insert.intraTrackOffset = rw18Header.offset;
}

static RW18SavFileHeader readInRestOfRW18FileHeader(DiskImage* pThis, FILE* pFile, void* pPartialHeader)
{
    RW18SavFileHeader rw18Header;
    size_t            sizeDiffBetweenHeaders = sizeof(rw18Header) - sizeof(SavFileHeader);
    size_t            bytesRead;

    assert ( sizeof(rw18Header) >= sizeof(SavFileHeader) );
    memcpy(&rw18Header, pPartialHeader, sizeof(SavFileHeader));
    bytesRead = fread((char*)&rw18Header + sizeof(SavFileHeader), 1, sizeDiffBetweenHeaders, pFile);
    if (bytesRead != sizeDiffBetweenHeaders)
        __throw(fileException);
        
    return rw18Header;
}

static long getFileSize(FILE* pFile)
{
    fseek(pFile, 0, SEEK_END);
    long size = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    return size;
}

static unsigned int roundUpLengthToBlockSize(unsigned int length)
{
    return (length + (DISK_IMAGE_BLOCK_SIZE - 1)) & ~(DISK_IMAGE_BLOCK_SIZE - 1);
}


static void validateObjectFileHasValidImageTableHeader(DiskImage* pThis);
static void updateImageTableAddresses(DiskImage* pThis, unsigned short newImageTableAddress);
__throws void DiskImage_UpdateImageTableFile(DiskImage* pThis, unsigned short newImageTableAddress)
{
    validateObjectFileHasValidImageTableHeader(pThis);
    updateImageTableAddresses(pThis, newImageTableAddress);
}

static void validateObjectFileHasValidImageTableHeader(DiskImage* pThis)
{
    unsigned char* pObject = pThis->object.pBuffer;
    unsigned char  imageCount;
    unsigned short expectedStartAddress;
    unsigned short actualStartAddress;
    
    if (pThis->objectFileLength < 3)
        __throw(fileException);
    
    imageCount = *pObject++;
    actualStartAddress = (unsigned short)*pObject++;
    actualStartAddress |= ((unsigned short)*pObject++) << 8;
    expectedStartAddress = IMAGE_TABLE_DEFAULT_ADDRESS + 1 + (imageCount + 1) * 2;
    if (actualStartAddress != expectedStartAddress)
        __throw(fileException);
}

static void updateImageTableAddresses(DiskImage* pThis, unsigned short newImageTableAddress)
{
    unsigned char* pObject = pThis->object.pBuffer;
    unsigned int   bytesLeft = pThis->objectFileLength;
    unsigned char  imageCount;
    unsigned char  i;
    
    imageCount = *pObject++;
    bytesLeft--;
    
    for (i = 0 ; i < imageCount + 1 ; i++)
    {
        unsigned short currAddress;
        
        if (bytesLeft < 2)
            __throw(fileException);
        
        currAddress = (unsigned short)pObject[0];
        currAddress |= ((unsigned short)pObject[1]) << 8;
        currAddress -= IMAGE_TABLE_DEFAULT_ADDRESS;
        currAddress += newImageTableAddress;
        pObject[0] = currAddress & 0xFF;
        pObject[1] = currAddress >> 8;
        
        pObject += 2;
        bytesLeft -= 2;
    }
}


static void validateSourceObjectParameters(DiskImage* pThis, DiskImageInsert* pInsert);
__throws void DiskImage_InsertObjectFile(DiskImage* pThis, DiskImageInsert* pInsert)
{
    validateSourceObjectParameters(pThis, pInsert);
    pThis->pVTable->insertData(pThis, pThis->object.pBuffer, pInsert);
}

static void validateSourceObjectParameters(DiskImage* pThis, DiskImageInsert* pInsert)
{
    if (pInsert->type == DISK_IMAGE_INSERTION_RWTS16CP)
        return;
    if (pInsert->sourceOffset >= pThis->objectFileLength)
        __throw(invalidSourceOffsetException);
    if (pInsert->sourceOffset + pInsert->length > pThis->object.bufferSize)
        __throw(invalidLengthException);
}


__throws void DiskImage_WriteImage(DiskImage* pThis, const char* pImageFilename)
{
    FILE* pFile = NULL;

    __try
    {
        pFile = openFile(pImageFilename, "wb");
        ByteBuffer_WriteToFile(&pThis->image, pFile);
    }
    __catch
    {
        if (pFile)
            fclose(pFile);
        __rethrow;
    }
    
    fclose(pFile);
}


unsigned char* DiskImage_GetImagePointer(DiskImage* pThis)
{
    return pThis->image.pBuffer;
}


size_t DiskImage_GetImageSize(DiskImage* pThis)
{
    return pThis->image.bufferSize;
}
