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
#include <string.h>
#include "SymbolTable.h"
#include "SymbolTableTest.h"


static SymbolTable* allocateSymbolTable(void);
static void allocateBuckets(SymbolTable* pThis, size_t bucketCount);
__throws SymbolTable* SymbolTable_Create(size_t bucketCount)
{
    SymbolTable* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocateSymbolTable() );
        __throwing_func( allocateBuckets(pThis, bucketCount) );
    }
    __catch
    {
        SymbolTable_Free(pThis);
        __rethrow_and_return(NULL);
    }

    return pThis;
}

static SymbolTable* allocateSymbolTable(void)
{
    SymbolTable* pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    memset(pThis, 0, sizeof(*pThis));
    return pThis;
}

static void allocateBuckets(SymbolTable* pThis, size_t bucketCount)
{
    size_t allocationSize = bucketCount * sizeof(*pThis->ppBuckets);
    pThis->ppBuckets = malloc(allocationSize);
    if (!pThis->ppBuckets)
        __throw(outOfMemoryException);
    memset(pThis->ppBuckets, 0, allocationSize);
    pThis->bucketCount = bucketCount;
}


static void freeBuckets(SymbolTable* pThis);
static void freeBucketList(SymbolTableElement* pEntry);
void SymbolTable_Free(SymbolTable* pThis)
{
    if (!pThis)
        return;
    
    freeBuckets(pThis);
    free(pThis);
}

static void freeBuckets(SymbolTable* pThis)
{
    size_t i;
    
    if (!pThis->ppBuckets)
        return;
        
    for (i = 0 ; i < pThis->bucketCount ; i++)
        freeBucketList(pThis->ppBuckets[i]);
    free(pThis->ppBuckets);
}

static void freeBucketList(SymbolTableElement* pEntry)
{
    while (pEntry)
    {
        SymbolTableElement* pNext = pEntry->pNext;
        free(pEntry);
        pEntry = pNext;;
    }
}


size_t SymbolTable_GetSymbolCount(SymbolTable* pThis)
{
    return pThis->symbolCount;
}


static SymbolTableElement* allocateElement(const char* pKey, const void* pData);
static size_t hashString(const char* pString);
SymbolTableElement* SymbolTable_Add(SymbolTable* pThis, const char* pKey, const void* pData)
{
    SymbolTableElement*  pElement;
    SymbolTableElement** ppBucket;    
    
    __try
        pElement = allocateElement(pKey, pData);
    __catch
        __rethrow_and_return(NULL);

    ppBucket = &pThis->ppBuckets[hashString(pKey) % pThis->bucketCount];
    pElement->pNext = *ppBucket;
    *ppBucket = pElement;
    pThis->symbolCount++;
    
    return pElement;
}

static SymbolTableElement* allocateElement(const char* pKey, const void* pData)
{
    SymbolTableElement* pElement = NULL;
    
    pElement = malloc(sizeof(*pElement));
    if (!pElement)
        __throw_and_return(outOfMemoryException, NULL);
        
    pElement->pKey = pKey;
    pElement->pData = pData;
    pElement->pNext = NULL;
    
    return pElement;
}

static size_t hashString(const char* pString)
{
    static const size_t hashMultiplier = 31;
    size_t              hash = 0;
    const char*         pCurr;
    
    for (pCurr = pString ; *pCurr ; pCurr++)
        hash = hash * hashMultiplier + (size_t)*pCurr;
        
    return hash;
}


static SymbolTableElement* searchBucket(SymbolTableElement* pBucketHead, const char* pKey);
SymbolTableElement* SymbolTable_Find(SymbolTable* pThis, const char* pKey)
{
    size_t i;
    
    for (i = 0 ; i < pThis->bucketCount ; i++)
    {
        SymbolTableElement* pSearchResult = searchBucket(pThis->ppBuckets[i], pKey);
        if (pSearchResult)
            return pSearchResult;
    }
    return NULL;
}

static SymbolTableElement* searchBucket(SymbolTableElement* pBucketHead, const char* pKey)
{
    SymbolTableElement* pCurr = pBucketHead;
    
    if (!pBucketHead)
        return NULL;
        
    while (pCurr)
    {
        if (0 == strcmp(pKey, pCurr->pKey))
            return pCurr;
            
        pCurr = pCurr->pNext;
    }
    return NULL;
}
