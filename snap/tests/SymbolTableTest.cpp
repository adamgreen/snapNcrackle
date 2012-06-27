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
    
    void createOneSymbol(void)
    {
        m_pSymbol1 = SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
        LONGS_EQUAL(1, SymbolTable_GetSymbolCount(m_pSymbolTable));
    
        CHECK(NULL != m_pSymbol1);
        LONGS_EQUAL(noException, getExceptionCode());
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
    
    void nextEnumAttemptShouldFail(void)
    {
        Symbol* pSymbol = SymbolTable_EnumNext(m_pSymbolTable);
        POINTERS_EQUAL(NULL, pSymbol);
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
    const Symbol* pSymbol = NULL;
    int   exceptionThrown = FALSE;
    
    m_pSymbolTable = SymbolTable_Create(1);
    MallocFailureInject_Construct(1);
    __try
        pSymbol = SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
    __catch
        exceptionThrown = TRUE;
    MallocFailureInject_Destruct();
    
    CHECK_TRUE(exceptionThrown);
    CHECK(NULL == pSymbol);
    LONGS_EQUAL(0, SymbolTable_GetSymbolCount(m_pSymbolTable));
    clearExceptionCode();
}

TEST(SymbolTable, OneItemInSymbolTable)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(2);
    pSymbol = SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
    
    CHECK(NULL != pSymbol);
    POINTERS_EQUAL(pKey1, pSymbol->pKey);
    POINTERS_EQUAL(pData1, pSymbol->pData);
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
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(2);
    SymbolTable_Add(m_pSymbolTable, pKey1, pData1);
    pSymbol = SymbolTable_Find(m_pSymbolTable, "foobar");
    POINTERS_EQUAL(NULL, pSymbol);
}

TEST(SymbolTable, FindItemWhenMultipleBuckets)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(5);
    createTwoSymbols();
    pSymbol = SymbolTable_Find(m_pSymbolTable, pKey1);
    CHECK(NULL != pSymbol);
    POINTERS_EQUAL(pKey1, pSymbol->pKey);
    POINTERS_EQUAL(pData1, pSymbol->pData);
}

TEST(SymbolTable, FindFirstItemInBucket)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();

    pSymbol = SymbolTable_Find(m_pSymbolTable, pKey1);
    CHECK(NULL != pSymbol);
    POINTERS_EQUAL(pKey1, pSymbol->pKey);
    POINTERS_EQUAL(pData1, pSymbol->pData);
}

TEST(SymbolTable, FindSecondItemInBucket)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();

    pSymbol = SymbolTable_Find(m_pSymbolTable, pKey2);
    CHECK(NULL != pSymbol);
    POINTERS_EQUAL(pKey2, pSymbol->pKey);
    POINTERS_EQUAL(pData2, pSymbol->pData);
}

TEST(SymbolTable, EnumerateEmptyList)
{
    m_pSymbolTable = SymbolTable_Create(1);
    SymbolTable_EnumStart(m_pSymbolTable);
    nextEnumAttemptShouldFail();
}

TEST(SymbolTable, EnumerateSingleItemList)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();
    
    SymbolTable_EnumStart(m_pSymbolTable);
    Symbol* pSymbol = SymbolTable_EnumNext(m_pSymbolTable);
    CHECK(pSymbol != NULL);
    LONGS_EQUAL(noException, getExceptionCode());
    POINTERS_EQUAL(pKey1, pSymbol->pKey);

    nextEnumAttemptShouldFail();
}

TEST(SymbolTable, EnumerateTwoItemsInOneBucket)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();
    
    SymbolTable_EnumStart(m_pSymbolTable);

    Symbol* pSymbol1 = SymbolTable_EnumNext(m_pSymbolTable);
    CHECK(pSymbol1 != NULL);
    LONGS_EQUAL(noException, getExceptionCode());

    Symbol* pSymbol2 = SymbolTable_EnumNext(m_pSymbolTable);
    CHECK(pSymbol2 != NULL);
    LONGS_EQUAL(noException, getExceptionCode());
    CHECK(pSymbol1 != pSymbol2);

    nextEnumAttemptShouldFail();
}

TEST(SymbolTable, EnumerateOneItemNotInFirstBucket)
{
    m_pSymbolTable = SymbolTable_Create(111);
    createOneSymbol();
    
    SymbolTable_EnumStart(m_pSymbolTable);

    Symbol* pSymbol = SymbolTable_EnumNext(m_pSymbolTable);
    CHECK(pSymbol != NULL);
    POINTERS_EQUAL(pKey1, pSymbol->pKey);
    LONGS_EQUAL(noException, getExceptionCode());

    nextEnumAttemptShouldFail();
}

TEST(SymbolTable, EnumerateTwoItemsInDifferentBuckets)
{
    m_pSymbolTable = SymbolTable_Create(111);
    createTwoSymbols();
    
    SymbolTable_EnumStart(m_pSymbolTable);

    Symbol* pSymbol1 = SymbolTable_EnumNext(m_pSymbolTable);
    CHECK(pSymbol1 != NULL);
    LONGS_EQUAL(noException, getExceptionCode());

    Symbol* pSymbol2 = SymbolTable_EnumNext(m_pSymbolTable);
    CHECK(pSymbol2 != NULL);
    LONGS_EQUAL(noException, getExceptionCode());
    CHECK(pSymbol1 != pSymbol2);

    nextEnumAttemptShouldFail();
}
