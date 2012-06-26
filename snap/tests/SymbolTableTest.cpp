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
    #include "SymbolTable.h"
    #include "MallocFailureInject.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

static const char* pKey1 = "foo1";
static const char* pData1 = "bar1";
static const char* pKey2 = "foo2";
static const char* pData2 = "bar2";


TEST_GROUP(SymbolTable)
{
    SymbolTable* m_pSymbolTable;
    Symbol*      m_pSymbol1;
    Symbol*      m_pSymbol2;
    
    void setup()
    {
        m_pSymbolTable = NULL;
        m_pSymbol1 = NULL;
        m_pSymbol2 = NULL;
        clearExceptionCode();
    }

    void teardown()
    {
        LONGS_EQUAL(0, getExceptionCode());
        SymbolTable_Free(m_pSymbolTable);
        m_pSymbolTable = NULL;
    }
    
    void makeFailingInitCall(void)
    {
        int exceptionThrown = FALSE;

        __try
            m_pSymbolTable = SymbolTable_Create(1);
        __catch
            exceptionThrown = TRUE;
        
        CHECK_TRUE(exceptionThrown);
        LONGS_EQUAL(outOfMemoryException, getExceptionCode());
        clearExceptionCode();
    }
    
    void createTwoSymbols(void)
    {
        m_pSymbol1 = SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
        m_pSymbol2 = SymbolTable_Add(m_pSymbolTable, pKey2, pData2);
        LONGS_EQUAL(2, SymbolTable_GetSymbolCount(m_pSymbolTable));
    
        CHECK(NULL != m_pSymbol1);
        CHECK(NULL != m_pSymbol2);
        CHECK(m_pSymbol1 != m_pSymbol2);
    }
};


TEST(SymbolTable, InitAndFailFirstAlloc)
{
    MallocFailureInject_Construct(1);
    makeFailingInitCall();
    MallocFailureInject_Destruct();
}

TEST(SymbolTable, InitAndFailSecondAlloc)
{
    MallocFailureInject_Construct(2);
    makeFailingInitCall();
    MallocFailureInject_Destruct();
}

TEST(SymbolTable, EmptySymbolTable)
{
    m_pSymbolTable = SymbolTable_Create(1);
    LONGS_EQUAL(0, SymbolTable_GetSymbolCount(m_pSymbolTable));
}

TEST(SymbolTable, FailSymbolTableEntry)
{
    const Symbol* pElem = NULL;
    int   exceptionThrown = FALSE;
    
    m_pSymbolTable = SymbolTable_Create(1);
    MallocFailureInject_Construct(1);
    __try
        pElem = SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
    __catch
        exceptionThrown = TRUE;
    MallocFailureInject_Destruct();
    
    CHECK_TRUE(exceptionThrown);
    CHECK(NULL == pElem);
    LONGS_EQUAL(0, SymbolTable_GetSymbolCount(m_pSymbolTable));
    clearExceptionCode();
}

TEST(SymbolTable, OneItemInSymbolTable)
{
    const Symbol* pElem = NULL;
    
    m_pSymbolTable = SymbolTable_Create(2);
    pElem = SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
    
    CHECK(NULL != pElem);
    POINTERS_EQUAL(pKey1, pElem->pKey);
    POINTERS_EQUAL(pData1, pElem->pData);
    LONGS_EQUAL(1, SymbolTable_GetSymbolCount(m_pSymbolTable));
}

TEST(SymbolTable, TwoItemsInSymbolTable)
{
    m_pSymbolTable = SymbolTable_Create(2);
    createTwoSymbols();
    POINTERS_EQUAL(pKey1, m_pSymbol1->pKey);
    POINTERS_EQUAL(pData1, m_pSymbol1->pData);
    POINTERS_EQUAL(pKey2, m_pSymbol2->pKey);
    POINTERS_EQUAL(pData2, m_pSymbol2->pData);
}

TEST(SymbolTable, AttemptToFindNonExistantItem)
{
    const Symbol* pElem = NULL;
    
    m_pSymbolTable = SymbolTable_Create(2);
    SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
    pElem = SymbolTable_Find(m_pSymbolTable, "foobar");
    POINTERS_EQUAL(NULL, pElem);
}

TEST(SymbolTable, FindItemWhenMultipleBuckets)
{
    const Symbol* pElem = NULL;
    
    m_pSymbolTable = SymbolTable_Create(5);
    createTwoSymbols();
    pElem = SymbolTable_Find(m_pSymbolTable, pKey1);
    CHECK(NULL != pElem);
    POINTERS_EQUAL(pKey1, pElem->pKey);
    POINTERS_EQUAL(pData1, pElem->pData);
}

TEST(SymbolTable, FindFirstItemInBucket)
{
    const Symbol* pElem = NULL;
    
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();

    pElem = SymbolTable_Find(m_pSymbolTable, pKey1);
    CHECK(NULL != pElem);
    POINTERS_EQUAL(pKey1, pElem->pKey);
    POINTERS_EQUAL(pData1, pElem->pData);
}

TEST(SymbolTable, FindSecondItemInBucket)
{
    const Symbol* pElem = NULL;
    
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();

    pElem = SymbolTable_Find(m_pSymbolTable, pKey2);
    CHECK(NULL != pElem);
    POINTERS_EQUAL(pKey2, pElem->pKey);
    POINTERS_EQUAL(pData2, pElem->pData);
}
