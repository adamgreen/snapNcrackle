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
#include "NibbleDiskImage.h"
#include "DiskImagePriv.h"
#include "DiskImageTest.h"
#include "BinaryBuffer.h"
#include "TextFile.h"
#include "ParseCSV.h"
#include "util.h"


struct NibbleDiskImage
{
    DiskImage            super;
    unsigned char*       pWrite;
    const unsigned char* pRead;
    const unsigned char* pData;
    unsigned char*       pCurrentTrack;
    unsigned int         side;
    unsigned int         track;
    unsigned int         sector;
    unsigned int         intraTrackOffset;
    unsigned int         bytesLeft;
    unsigned char        checksum;
    unsigned char        lastByte;
    unsigned char        aux[86];
    unsigned char        decode8to6[256];
};


static void freeObject(void* pThis);
static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
struct DiskImageVTable NibbleDiskImageVTable = 
{ 
    freeObject,
    insertData 
};


static void initializeDecode8to6Table(NibbleDiskImage* pThis);
static unsigned char encode6to8(unsigned char byte);
__throws NibbleDiskImage* NibbleDiskImage_Create(void)
{
    NibbleDiskImage* pThis = NULL;
    
    __try
    {
        pThis = allocateAndZero(sizeof(*pThis));
        DiskImage_Init(&pThis->super, &NibbleDiskImageVTable, NIBBLE_DISK_IMAGE_SIZE);
        initializeDecode8to6Table(pThis);
    }
    __catch
    {
        DiskImage_Free(&pThis->super);
        __rethrow;
    }
        
    return pThis;
}

static void initializeDecode8to6Table(NibbleDiskImage* pThis)
{
    unsigned char i = 0;
    memset(pThis->decode8to6, 0xFF, sizeof(pThis->decode8to6));
    for (i = 0 ; i < 64 ; i++)
        pThis->decode8to6[encode6to8(i)] = i;
}

static unsigned char encode6to8(unsigned char byte)
{
    static const unsigned char nibbleArray[64] =
    {
        0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
        0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
        0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
        0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
        0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
        0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

    assert ( byte < 64 );
    return nibbleArray[byte];
}


static void freeObject(void* pThis)
{
}


__throws void NibbleDiskImage_ProcessScriptFile(NibbleDiskImage* pThis, const char* pScriptFilename)
{
    DiskImage_ProcessScriptFile(&pThis->super, pScriptFilename);
}


__throws void NibbleDiskImage_ProcessScript(NibbleDiskImage* pThis, char* pScriptText)
{
    DiskImage_ProcessScript(&pThis->super, pScriptText);
}


__throws void NibbleDiskImage_ReadObjectFile(NibbleDiskImage* pThis, const char* pFilename)
{
    DiskImage_ReadObjectFile(&pThis->super, pFilename);
}


__throws void NibbleDiskImage_InsertObjectFile(NibbleDiskImage* pThis, DiskImageInsert* pInsert)
{
    DiskImage_InsertObjectFile(&pThis->super, pInsert);
}


static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    NibbleDiskImage_InsertData((NibbleDiskImage*)pThis, pData, pInsert);
}

static void insertRWTS16Data(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void prepareForFirstRWTS16Sector(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void advanceToNextSector(NibbleDiskImage* pThis);
static void writeRWTS16Sector(NibbleDiskImage* pThis, int);
static void validateRWTS16TrackAndSector(NibbleDiskImage* pThis, int isCopyProtectionSector);
static void writeSectorLeadInSyncBytes(NibbleDiskImage* pThis);
static void writeSyncBytes(NibbleDiskImage* pThis, size_t syncByteCount);
static void writeRWTS16AddressField(NibbleDiskImage* pThis, unsigned char volume, unsigned char track, unsigned char sector);
static void writeRWTS16AddressFieldProlog(NibbleDiskImage* pThis);
static void initChecksum(NibbleDiskImage* pThis);
static void write4and4Data(NibbleDiskImage* pThis, unsigned char byte);
static void updateChecksum(NibbleDiskImage* pThis, unsigned char byte);
static void writeRWTS16FieldEpilog(NibbleDiskImage* pThis);
static void writeRWTS16DataField(NibbleDiskImage* pThis, const unsigned char* pData);
static void writeRWTS16DataFieldProlog(NibbleDiskImage* pThis);
static void write6and2Data(NibbleDiskImage* pThis, const unsigned char* pData);
static void fillAuxBuffer(NibbleDiskImage* pThis, const unsigned char* pData);
static unsigned char encodeAuxByte(NibbleDiskImage* pThis, size_t i, const unsigned char* pData);
static unsigned char lowBitsByte(size_t i, const unsigned char* pData);
static unsigned char lowBitOffset(size_t i);
static unsigned char midBitsByte(size_t i, const unsigned char* pData);
static unsigned char midBitOffset(size_t i);
static unsigned char highBitsByte(size_t i, const unsigned char* pData);
static unsigned char highBitOffset(size_t i);
static void checksumNibbilizeAndWrite(NibbleDiskImage* pThis, const unsigned char* pData);
static void checksumNibbilizeAndWriteAuxBuffer(NibbleDiskImage* pThis);
static unsigned char nibbilizeByte(NibbleDiskImage* pThis, unsigned char byte);
static void checksumNibbilizeAndWriteDataBuffer(NibbleDiskImage* pThis, const unsigned char* pData);
static void nibbilizeAndWriteChecksum(NibbleDiskImage* pThis);
static void insertRWTS16CPData(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void writeRWTS16CPDataField(NibbleDiskImage* pThis);
static void insertRW18Data(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void prepareForFirstRW18Track(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void writeRW18Track(NibbleDiskImage* pThis);
static void validateRW18TrackAndOffset(NibbleDiskImage* pThis);
static unsigned int initTrackData(NibbleDiskImage* pThis, unsigned char* pTrackData, size_t trackDataSize);
static void readCurrentTrackContentsOrZeroFill(NibbleDiskImage* pThis, unsigned char* pTrackData, size_t trackDataSize);
static void writeEncodedBytes(NibbleDiskImage* pThis, const char* pBytes, size_t byteCount);
static void writeRW18Sector(NibbleDiskImage* pThis, unsigned char sector);
static void writeRW18AddressField(NibbleDiskImage* pThis, unsigned char track, unsigned char sector);
static void writeRW18AddressFieldProlog(NibbleDiskImage* pThis);
static void writeRW18AddressFieldEpilog(NibbleDiskImage* pThis);
static void writeRW18DataField(NibbleDiskImage* pThis, unsigned char sector);
static void writeRW18BundleId(NibbleDiskImage* pThis);
static void writeRW18Data(NibbleDiskImage* pThis, unsigned char sector);
static void writeRW18DataFieldEpilog(NibbleDiskImage* pThis);
static void advanceToNextRW18Track(NibbleDiskImage* pThis, unsigned int bytesUsed);
__throws void NibbleDiskImage_InsertData(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    switch (pInsert->type)
    {
    case DISK_IMAGE_INSERTION_RWTS16:
        insertRWTS16Data(pThis, pData, pInsert);
        break;
    case DISK_IMAGE_INSERTION_RWTS16CP:
        insertRWTS16CPData(pThis, pData, pInsert);
        break;
    case DISK_IMAGE_INSERTION_RW18:
        insertRW18Data(pThis, pData, pInsert);
        break;
    case DISK_IMAGE_INSERTION_BLOCK:
    default:
        __throw(invalidInsertionTypeException);
    }
}

static void insertRWTS16Data(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    prepareForFirstRWTS16Sector(pThis, pData, pInsert);
    while (pThis->bytesLeft > 0)
    {
        writeRWTS16Sector(pThis, 0);
        advanceToNextSector(pThis);
    }
}

static void prepareForFirstRWTS16Sector(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    pThis->track = pInsert->track;
    pThis->sector = pInsert->sector;
    pThis->bytesLeft = pInsert->length;
    pThis->pData = pData + pInsert->sourceOffset;
}

static void advanceToNextSector(NibbleDiskImage* pThis)
{
    pThis->bytesLeft -= DISK_IMAGE_BYTES_PER_SECTOR;
    pThis->pData += DISK_IMAGE_BYTES_PER_SECTOR;
    pThis->sector++;
    if (pThis->sector >= NIBBLE_DISK_IMAGE_RWTS16_SECTORS_PER_TRACK)
    {
        pThis->sector = 0;
        pThis->track++;
    }
}

static void writeRWTS16Sector(NibbleDiskImage* pThis, int isCopyProtectionSector)
{
    static const unsigned char   volume = 0;
    unsigned int                 imageOffset = NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * pThis->track + 
                                               NIBBLE_DISK_IMAGE_RWTS16_GAP1_SYNC_BYTES +
                                               NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR * pThis->sector;
    const unsigned char*         pStart;
    ptrdiff_t leeway;
    
    validateRWTS16TrackAndSector(pThis, isCopyProtectionSector);
        
    pThis->pWrite = DiskImage_GetImagePointer(&pThis->super) + imageOffset;
    pStart = pThis->pWrite;
    
    writeSectorLeadInSyncBytes(pThis);
    writeRWTS16AddressField(pThis, volume, pThis->track, pThis->sector);
    writeSyncBytes(pThis, NIBBLE_DISK_IMAGE_RWTS16_GAP2_SYNC_BYTES);
    if (!isCopyProtectionSector)
        writeRWTS16DataField(pThis, pThis->pData);
    else
        writeRWTS16CPDataField(pThis);
    
    leeway = (NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR - NIBBLE_DISK_IMAGE_RWTS16_GAP3_SYNC_BYTES) - (pThis->pWrite - pStart);
    assert(leeway >= 0);
    if (leeway > 0)
        memset(pThis->pWrite, 0xff, (size_t)leeway);
}

static void validateRWTS16TrackAndSector(NibbleDiskImage* pThis, int isCopyProtectionSector)
{
    if (pThis->sector >= NIBBLE_DISK_IMAGE_RWTS16_SECTORS_PER_TRACK)
        __throw(invalidSectorException);
    if (pThis->track >= DISK_IMAGE_TRACKS_PER_SIDE)
        __throw(invalidTrackException);
    if (pThis->bytesLeft < DISK_IMAGE_BYTES_PER_SECTOR && !isCopyProtectionSector)
        __throw(invalidLengthException);
}

static void writeSectorLeadInSyncBytes(NibbleDiskImage* pThis)
{
    size_t leadInSyncByteCount;
    
    if (pThis->sector == 0)
        leadInSyncByteCount = NIBBLE_DISK_IMAGE_RWTS16_GAP1_SYNC_BYTES;
    else
        leadInSyncByteCount = NIBBLE_DISK_IMAGE_RWTS16_GAP3_SYNC_BYTES;
    pThis->pWrite -= leadInSyncByteCount;
    
    writeSyncBytes(pThis, leadInSyncByteCount);
}

static void writeSyncBytes(NibbleDiskImage* pThis, size_t syncByteCount)
{
    memset(pThis->pWrite, 0xff, syncByteCount);
    pThis->pWrite += syncByteCount;
}

static void writeRWTS16AddressField(NibbleDiskImage* pThis, unsigned char volume, unsigned char track, unsigned char sector)
{
    writeRWTS16AddressFieldProlog(pThis);
    
    initChecksum(pThis);
    write4and4Data(pThis, volume);
    write4and4Data(pThis, track);
    write4and4Data(pThis, sector);
    write4and4Data(pThis, pThis->checksum);
    
    writeRWTS16FieldEpilog(pThis);
}

static void writeRWTS16AddressFieldProlog(NibbleDiskImage* pThis)
{
    memcpy(pThis->pWrite, "\xD5\xAA\x96", 3);
    pThis->pWrite += 3;
}

static void initChecksum(NibbleDiskImage* pThis)
{
    pThis->checksum = 0;
}

static void write4and4Data(NibbleDiskImage* pThis, unsigned char byte)
{
    char oddBits = byte & 0xAA;
    char evenBits = byte & 0x55;
    char encodedOddByte = 0xAA | (oddBits >> 1);
    char encodedEvenByte = 0xAA | evenBits;
    
    updateChecksum(pThis, byte);
    *pThis->pWrite++ = encodedOddByte;
    *pThis->pWrite++ = encodedEvenByte;
}

static void updateChecksum(NibbleDiskImage* pThis, unsigned char byte)
{
    pThis->checksum ^= byte;
}

static void writeRWTS16FieldEpilog(NibbleDiskImage* pThis)
{
    memcpy(pThis->pWrite, "\xDE\xAA\xEB", 3);
    pThis->pWrite += 3;
}

static void writeRWTS16DataField(NibbleDiskImage* pThis, const unsigned char* pData)
{
    writeRWTS16DataFieldProlog(pThis);
    write6and2Data(pThis, pData);
    writeRWTS16FieldEpilog(pThis);
}

static void writeRWTS16DataFieldProlog(NibbleDiskImage* pThis)
{
    memcpy(pThis->pWrite, "\xD5\xAA\xAD", 3);
    pThis->pWrite += 3;
}

static void write6and2Data(NibbleDiskImage* pThis, const unsigned char* pData)
{
    fillAuxBuffer(pThis, pData);
    checksumNibbilizeAndWrite(pThis, pData);
}

static void fillAuxBuffer(NibbleDiskImage* pThis, const unsigned char* pData)
{
    size_t i;
    
    for (i = 0; i < sizeof(pThis->aux) ; i++)
        pThis->aux[i] = encodeAuxByte(pThis, i, pData);
}

static unsigned char encodeAuxByte(NibbleDiskImage* pThis, size_t i, const unsigned char* pData)
{
    unsigned char lowByte = lowBitsByte(i, pData);
    unsigned char midByte = midBitsByte(i, pData);
    unsigned char highByte = highBitsByte(i, pData);
    unsigned char lowBits = ((lowByte & 1) << 1) |
                            ((lowByte & 2) >> 1);
    unsigned char midBits = ((midByte & 1) << 3) |
                            ((midByte & 2) << 1);
    unsigned char highBits = ((highByte & 1) << 5) |
                             ((highByte & 2) << 3);
    
    return highBits | midBits | lowBits;
}

static unsigned char lowBitsByte(size_t i, const unsigned char* pData)
{
    return pData[lowBitOffset(i)];
}

static unsigned char lowBitOffset(size_t i)
{
    return (unsigned char)(0x55 - i);
}

static unsigned char midBitsByte(size_t i, const unsigned char* pData)
{
    return pData[midBitOffset(i)];
}

static unsigned char midBitOffset(size_t i)
{
    return (unsigned char)(0xAB - i);
}

static unsigned char highBitsByte(size_t i, const unsigned char* pData)
{
    return pData[highBitOffset(i)];
}

static unsigned char highBitOffset(size_t i)
{
    return (unsigned char)(0x101 - i);
}

static void checksumNibbilizeAndWrite(NibbleDiskImage* pThis, const unsigned char* pData)
{
    pThis->lastByte = 0;
    initChecksum(pThis);
    
    checksumNibbilizeAndWriteAuxBuffer(pThis);
    checksumNibbilizeAndWriteDataBuffer(pThis, pData);
    nibbilizeAndWriteChecksum(pThis);
}

static void checksumNibbilizeAndWriteAuxBuffer(NibbleDiskImage* pThis)
{
    unsigned char* pCurr = &pThis->aux[sizeof(pThis->aux)-1];
    size_t         i;
    
    for (i = 0 ; i < sizeof(pThis->aux) ; i++)
        *pThis->pWrite++ = nibbilizeByte(pThis, *pCurr--);
    
}

static unsigned char nibbilizeByte(NibbleDiskImage* pThis, unsigned char byte)
{
    unsigned char encodedByte;
    
    encodedByte = byte ^ pThis->lastByte;
    pThis->lastByte = byte;

    return encode6to8(encodedByte);
}

static void checksumNibbilizeAndWriteDataBuffer(NibbleDiskImage* pThis, const unsigned char* pData)
{
    const unsigned char* pCurr = pData;
    size_t         i;
    
    for (i = 0 ; i < DISK_IMAGE_BYTES_PER_SECTOR ; i++)
        *pThis->pWrite++ = nibbilizeByte(pThis, (*pCurr++) >> 2);
    
}

static void nibbilizeAndWriteChecksum(NibbleDiskImage* pThis)
{
    *pThis->pWrite++ = nibbilizeByte(pThis, 0x00);
}

static void insertRWTS16CPData(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    prepareForFirstRWTS16Sector(pThis, pData, pInsert);
    writeRWTS16Sector(pThis, 1);
}

static void writeRWTS16CPDataField(NibbleDiskImage* pThis)
{
    /* If Michael Kelsey's description (textfiles.com/apple/CRACKING/asstcracks1.txt) is anything to go by, this is
       a "bit insertion" copy protection technique.  The precise technique is likely hard to replicate in a nibble
       disk image in a way which is faithful to the original _and_ also works with emulators.  I follow Kelsey's
       solution here: use a nibble sequence which has no hidden timing bits, but still satisfies what the protection
       check routines want.

       -- tkchia 20131015
    */
    static const char magicNibbles[] = "\xe7\xe7\xe7\xe7\xe7\xe7\xaf\xf3\xfc\xee\xe7\xfc\xee\xe7\xfc\xee\xee\xfc";
    size_t i;
    writeRWTS16DataFieldProlog(pThis);
    writeEncodedBytes(pThis, magicNibbles, sizeof magicNibbles - 1);
    for (i = sizeof magicNibbles - 1; i < 343; ++i)
        writeEncodedBytes(pThis, "\xff", 1);
    writeRWTS16FieldEpilog(pThis);
}

static void insertRW18Data(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    prepareForFirstRW18Track(pThis, pData, pInsert);
    while (pThis->bytesLeft > 0)
        writeRW18Track(pThis);
}

static void prepareForFirstRW18Track(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    pThis->side = pInsert->side;
    pThis->track = pInsert->track;
    pThis->intraTrackOffset = pInsert->intraTrackOffset;
    pThis->bytesLeft = pInsert->length;
    pThis->pData = pData + pInsert->sourceOffset;
}

static void writeRW18Track(NibbleDiskImage* pThis)
{
    unsigned int         bytesUsed = 0;
    unsigned int         destOffset = NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * pThis->track;
    unsigned char        sector = 5;
    unsigned char        trackData[DISK_IMAGE_RW18_BYTES_PER_TRACK];
    const unsigned char* pStart;
    
    validateRW18TrackAndOffset(pThis);

    bytesUsed = initTrackData(pThis, trackData, sizeof(trackData));
    pThis->pWrite = DiskImage_GetImagePointer(&pThis->super) + destOffset;
    pThis->pCurrentTrack = trackData;
    pStart = pThis->pWrite;
    
    writeSyncBytes(pThis, 403);
    writeEncodedBytes(pThis, "\xa5\x96\xbf\xff\xfe\xaa\xbb\xaa\xaa\xff\xef\x9a", 12);
    writeRW18Sector(pThis, sector);
    
    do
    {
        writeSyncBytes(pThis, 5);
        writeRW18Sector(pThis, --sector);
    } while (sector);

    assert ( pThis->pWrite - pStart == NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK );
    
    advanceToNextRW18Track(pThis, bytesUsed);
}

static void validateRW18TrackAndOffset(NibbleDiskImage* pThis)
{
    if (pThis->track >= DISK_IMAGE_TRACKS_PER_SIDE)
        __throw(invalidTrackException);
    if (pThis->intraTrackOffset >= DISK_IMAGE_RW18_BYTES_PER_TRACK)
        __throw(invalidIntraTrackOffsetException);
}

static unsigned int initTrackData(NibbleDiskImage* pThis, unsigned char* pTrackData, size_t trackDataSize)
{
    unsigned int         copyBytes = trackDataSize - pThis->intraTrackOffset;
    const unsigned char* pSource = pThis->pData;
    unsigned char*       pDest = pTrackData + pThis->intraTrackOffset;
    
    readCurrentTrackContentsOrZeroFill(pThis, pTrackData, trackDataSize);
    if (copyBytes > pThis->bytesLeft)
        copyBytes = pThis->bytesLeft;
    memcpy(pDest, pSource, copyBytes);
    
    return copyBytes;
}

static void readCurrentTrackContentsOrZeroFill(NibbleDiskImage* pThis, unsigned char* pTrackData, size_t trackDataSize)
{
    __try
    {
        NibbleDiskImage_ReadRW18Track(pThis, pThis->side, pThis->track, pTrackData, trackDataSize);
    }
    __catch
    {
        /* Track didn't already contain RW18 data so initialize it to all zeroes. */
        memset(pTrackData, 0x00, trackDataSize);
        clearExceptionCode();
    }
}

static void writeEncodedBytes(NibbleDiskImage* pThis, const char* pBytes, size_t byteCount)
{
    memcpy(pThis->pWrite, pBytes, byteCount);
    pThis->pWrite += byteCount;
}

static void writeRW18Sector(NibbleDiskImage* pThis, unsigned char sector)
{
    writeRW18AddressField(pThis, pThis->track, sector);
    writeSyncBytes(pThis, 2);
    writeRW18DataField(pThis, sector);
    writeSyncBytes(pThis, 1);
}

static void writeRW18AddressField(NibbleDiskImage* pThis, unsigned char track, unsigned char sector)
{
    writeRW18AddressFieldProlog(pThis);
    
    *pThis->pWrite++ = encode6to8(track);
    *pThis->pWrite++ = encode6to8(sector);
    *pThis->pWrite++ = encode6to8(track ^ sector);
    
    writeRW18AddressFieldEpilog(pThis);
}

static void writeRW18AddressFieldProlog(NibbleDiskImage* pThis)
{
    memcpy(pThis->pWrite, "\xD5\x9D", 2);
    pThis->pWrite += 2;
}

static void writeRW18AddressFieldEpilog(NibbleDiskImage* pThis)
{
    *pThis->pWrite++ = 0xAA;
}

static void writeRW18DataField(NibbleDiskImage* pThis, unsigned char sector)
{
    writeRW18BundleId(pThis);
    writeRW18Data(pThis, sector);
    writeRW18DataFieldEpilog(pThis);
}

static void writeRW18BundleId(NibbleDiskImage* pThis)
{
    *pThis->pWrite++ = pThis->side;
}

static void writeRW18Data(NibbleDiskImage* pThis, unsigned char sector)
{
    unsigned char        checksum = 0;
    const unsigned char* pPage0 = pThis->pCurrentTrack + sector * DISK_IMAGE_PAGE_SIZE;
    const unsigned char* pPage1 = pThis->pCurrentTrack + (sector + 6) * DISK_IMAGE_PAGE_SIZE;
    const unsigned char* pPage2 = pThis->pCurrentTrack + (sector + 12) * DISK_IMAGE_PAGE_SIZE;
    int                  i = 0;
    
    for ( i = 0 ; i < DISK_IMAGE_PAGE_SIZE ; i++)
    {
        unsigned char byte0 = *pPage0++;
        unsigned char byte1 = *pPage1++;
        unsigned char byte2 = *pPage2++;
        unsigned char auxByte = ((byte0 & 0xC0) >> 2) | ((byte1 & 0xC0) >> 4) | ((byte2 & 0xC0) >> 6);
        
        *pThis->pWrite++ = encode6to8(auxByte);
        *pThis->pWrite++ = encode6to8(byte0 & 0x3F);
        *pThis->pWrite++ = encode6to8(byte1 & 0x3F);
        *pThis->pWrite++ = encode6to8(byte2 & 0x3F);
        
        checksum ^= auxByte;
        checksum ^= byte0 & 0x3F;
        checksum ^= byte1 & 0x3F;
        checksum ^= byte2 & 0x3F;
    }
    *pThis->pWrite++ = encode6to8(checksum);
}

static void writeRW18DataFieldEpilog(NibbleDiskImage* pThis)
{
    *pThis->pWrite++ = 0xD4;
}

static void advanceToNextRW18Track(NibbleDiskImage* pThis, unsigned int bytesUsed)
{
    pThis->bytesLeft -= bytesUsed;
    pThis->pData += bytesUsed;
    pThis->intraTrackOffset = 0;
    pThis->track++;
}


__throws void NibbleDiskImage_WriteImage(NibbleDiskImage* pThis, const char* pImageFilename)
{
    DiskImage_WriteImage(&pThis->super, pImageFilename);
}


const unsigned char* NibbleDiskImage_GetImagePointer(NibbleDiskImage* pThis)
{
    return DiskImage_GetImagePointer(&pThis->super);
}


size_t NibbleDiskImage_GetImageSize(NibbleDiskImage* pThis)
{
    return DiskImage_GetImageSize(&pThis->super);
}


static void validateReadRWTrackArguments(unsigned int track, size_t trackDataSize);
static void validateSyncBytes(NibbleDiskImage* pThis, unsigned int expectedSyncBytes);
static void validateByte(NibbleDiskImage* pThis, unsigned char expectedByte);
static void validateBytes(NibbleDiskImage* pThis, const char* pExpectedBytes, size_t byteCount);
static void extractRW18Sector(NibbleDiskImage* pThis, unsigned int sector);
static void validateDecodedByte(NibbleDiskImage* pThis, unsigned char expectedByte);
__throws void NibbleDiskImage_ReadRW18Track(NibbleDiskImage* pThis,
                                            unsigned int side,
                                            unsigned int track,
                                            unsigned char* pTrackData,
                                            size_t trackDataSize)
{
    unsigned int sector = 5;
    
    validateReadRWTrackArguments(track, trackDataSize);
    
    pThis->pRead = DiskImage_GetImagePointer(&pThis->super) +  NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * track;
    pThis->pCurrentTrack = pTrackData;
    pThis->track = track;
    pThis->side = side;

    validateSyncBytes(pThis, 403);
    validateBytes(pThis, "\xa5\x96\xbf\xff\xfe\xaa\xbb\xaa\xaa\xff\xef\x9a", 12);
    extractRW18Sector(pThis, sector);
    
    do
    {
        validateSyncBytes(pThis, 5);
        extractRW18Sector(pThis, --sector);
    } while (sector > 0);
}

static void validateReadRWTrackArguments(unsigned int track, size_t trackDataSize)
{
    if (track >= DISK_IMAGE_TRACKS_PER_SIDE)
        __throw(invalidTrackException);
    if (trackDataSize != DISK_IMAGE_RW18_BYTES_PER_TRACK)
        __throw(invalidArgumentException);
}

static void validateSyncBytes(NibbleDiskImage* pThis, unsigned int expectedSyncBytes)
{
    unsigned int i;
    
    for (i = 0 ; i < expectedSyncBytes ; i++)
        validateByte(pThis, 0xFF);
}

static void validateByte(NibbleDiskImage* pThis, unsigned char expectedByte)
{
    if (*pThis->pRead++ != expectedByte)
        __throw(badTrackException);
}

static void validateBytes(NibbleDiskImage* pThis, const char* pExpectedBytes, size_t byteCount)
{
    if (0 != memcmp(pThis->pRead, pExpectedBytes, byteCount))
        __throw(badTrackException);
    pThis->pRead += byteCount;
}

static void extractRW18Sector(NibbleDiskImage* pThis, unsigned int sector)
{
    unsigned char  checksum = pThis->track ^ sector;
    unsigned char* pPage0 = pThis->pCurrentTrack + sector * DISK_IMAGE_PAGE_SIZE;
    unsigned char* pPage1 = pThis->pCurrentTrack + (sector + 6) * DISK_IMAGE_PAGE_SIZE;
    unsigned char* pPage2 = pThis->pCurrentTrack + (sector + 12) * DISK_IMAGE_PAGE_SIZE;
    int            i;
    
    validateBytes(pThis, "\xd5\x9d", 2);
    validateDecodedByte(pThis, pThis->track);
    validateDecodedByte(pThis, sector);
    validateDecodedByte(pThis, checksum);
    validateBytes(pThis, "\xaa", 1);
    validateSyncBytes(pThis, 2);
    validateByte(pThis, pThis->side);
    
    checksum = 0;
    for (i = 0 ; i < 256 ; i++)
    {
        unsigned char auxByte = pThis->decode8to6[*pThis->pRead++];
        unsigned char byte0 = pThis->decode8to6[*pThis->pRead++];
        unsigned char byte1 = pThis->decode8to6[*pThis->pRead++];
        unsigned char byte2 = pThis->decode8to6[*pThis->pRead++];
        
        checksum ^= (auxByte ^ byte0 ^ byte1 ^ byte2);
        
        auxByte <<= 2;
        *pPage0++ = (auxByte & 0xC0) | byte0;
        auxByte <<= 2;
        *pPage1++ = (auxByte & 0xC0) | byte1;
        auxByte <<= 2;
        *pPage2++ = (auxByte & 0xC0) | byte2;
    }
    validateDecodedByte(pThis, checksum);
    
    validateByte(pThis, 0xD4);
    validateSyncBytes(pThis, 1);
}

static void validateDecodedByte(NibbleDiskImage* pThis, unsigned char expectedByte)
{
    unsigned char decodedByte = pThis->decode8to6[*pThis->pRead++];
    if (decodedByte != expectedByte)
        __throw(badTrackException);
}
