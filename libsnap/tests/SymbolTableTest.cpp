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
static const char* pKey2 = "foo2";
static const char* pLocal = ":Local";


TEST_GROUP(SymbolTable)
{
    LineInfo     m_lineInfo1;
    LineInfo     m_lineInfo2;
    SizedString  m_Key1;
    SizedString  m_Key2;
    SizedString  m_Local;
    SizedString  m_Empty;
    SymbolTable* m_pSymbolTable;
    Symbol*      m_pSymbol1;
    Symbol*      m_pSymbol2;
    
    void setup()
    {
        memset(&m_lineInfo1, 0, sizeof(m_lineInfo1));
        m_pSymbolTable = NULL;
        m_pSymbol1 = NULL;
        m_pSymbol2 = NULL;
        m_Key1 = SizedString_InitFromString(pKey1);
        m_Key2 = SizedString_InitFromString(pKey2);
        m_Local = SizedString_InitFromString(pLocal);
        m_Empty = SizedString_InitFromString(NULL);
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
        m_pSymbol1 = SymbolTable_Add(m_pSymbolTable, &m_Key1, &m_Empty);
        LONGS_EQUAL(1, SymbolTable_GetSymbolCount(m_pSymbolTable));
    
        CHECK(NULL != m_pSymbol1);
        LONGS_EQUAL(noException, getExceptionCode());
    }

    void createTwoSymbols(void)
    {
        m_pSymbol1 = SymbolTable_Add(m_pSymbolTable, &m_Key1, &m_Empty);
        m_pSymbol2 = SymbolTable_Add(m_pSymbolTable, &m_Key2, &m_Empty);
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
    
    void nextLineEnumAttemptShouldFail(Symbol* pSymbol)
    {
        LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(pSymbol);
        POINTERS_EQUAL(NULL, pLineInfo);
    }
    
    void validateSymbolKeys(const Symbol* pSymbol, SizedString* pExpectedGlobal, SizedString* pExpectedLocal)
    {
        LONGS_EQUAL(0, SizedString_Compare(&pSymbol->globalKey, pExpectedGlobal));
        LONGS_EQUAL(0, SizedString_Compare(&pSymbol->localKey, pExpectedLocal));
    }
};


TEST(SymbolTable, InitAndFailFirstAlloc)
{
    MallocFailureInject_FailAllocation(1);
    makeFailingInitCall();
    MallocFailureInject_Restore();
}

TEST(SymbolTable, InitAndFailSecondAlloc)
{
    MallocFailureInject_FailAllocation(2);
    makeFailingInitCall();
    MallocFailureInject_Restore();
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
    MallocFailureInject_FailAllocation(1);
    __try
        pSymbol = SymbolTable_Add(m_pSymbolTable, &m_Key1, &m_Empty);
    __catch
        exceptionThrown = TRUE;
    MallocFailureInject_Restore();
    
    CHECK_TRUE(exceptionThrown);
    CHECK(NULL == pSymbol);
    LONGS_EQUAL(0, SymbolTable_GetSymbolCount(m_pSymbolTable));
    clearExceptionCode();
}

TEST(SymbolTable, OneGlobalItemInSymbolTable)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(2);
    pSymbol = SymbolTable_Add(m_pSymbolTable, &m_Key1, &m_Empty);
    
    CHECK(NULL != pSymbol);
    validateSymbolKeys(pSymbol, &m_Key1, &m_Empty);
    LONGS_EQUAL(1, SymbolTable_GetSymbolCount(m_pSymbolTable));
}

TEST(SymbolTable, OneLocalItemInSymbolTable)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(2);
    pSymbol = SymbolTable_Add(m_pSymbolTable, &m_Key1, &m_Local);
    
    CHECK(NULL != pSymbol);
    validateSymbolKeys(pSymbol, &m_Key1, &m_Local);
    LONGS_EQUAL(1, SymbolTable_GetSymbolCount(m_pSymbolTable));
}

TEST(SymbolTable, TwoItemsInSymbolTable)
{
    m_pSymbolTable = SymbolTable_Create(2);
    createTwoSymbols();
    validateSymbolKeys(m_pSymbol1, &m_Key1, &m_Empty);
    validateSymbolKeys(m_pSymbol2, &m_Key2, &m_Empty);
}

TEST(SymbolTable, AttemptToFindNonExistantItem)
{
    const Symbol* pSymbol = NULL;
    SizedString fooBar = SizedString_InitFromString("foobar");
    
    m_pSymbolTable = SymbolTable_Create(2);
    SymbolTable_Add(m_pSymbolTable, &m_Key1, &m_Empty);
    pSymbol = SymbolTable_Find(m_pSymbolTable, &fooBar, &m_Empty);
    POINTERS_EQUAL(NULL, pSymbol);
}

TEST(SymbolTable, AttemptToFindStringWhichIsPrefixToExistingKey)
{
    const Symbol* pSymbol = NULL;
    SizedString fullKey = SizedString_InitFromString("JumpTable");
    SizedString keyPrefix = SizedString_InitFromString("Jump");
    
    m_pSymbolTable = SymbolTable_Create(1);
    SymbolTable_Add(m_pSymbolTable, &fullKey, &m_Empty);
    pSymbol = SymbolTable_Find(m_pSymbolTable, &keyPrefix, &m_Empty);
    POINTERS_EQUAL(NULL, pSymbol);
}

TEST(SymbolTable, FindItemWhenMultipleBuckets)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(5);
    createTwoSymbols();
    pSymbol = SymbolTable_Find(m_pSymbolTable, &m_Key1, &m_Empty);
    CHECK(NULL != pSymbol);
    validateSymbolKeys(pSymbol, &m_Key1, &m_Empty);
}

TEST(SymbolTable, FindFirstItemInBucket)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();

    pSymbol = SymbolTable_Find(m_pSymbolTable, &m_Key1, &m_Empty);
    CHECK(NULL != pSymbol);
    validateSymbolKeys(pSymbol, &m_Key1, &m_Empty);
}

TEST(SymbolTable, FindSecondItemInBucket)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();

    pSymbol = SymbolTable_Find(m_pSymbolTable, &m_Key2, &m_Empty);
    CHECK(NULL != pSymbol);
    validateSymbolKeys(pSymbol, &m_Key2, &m_Empty);
}

TEST(SymbolTable, FindBothItemsInBucketWithFind)
{
    const Symbol* pSymbol = NULL;
    
    m_pSymbolTable = SymbolTable_Create(1);
    createTwoSymbols();

    pSymbol = SymbolTable_Find(m_pSymbolTable, &m_Key1, &m_Empty);
    CHECK(NULL != pSymbol);
    validateSymbolKeys(pSymbol, &m_Key1, &m_Empty);

    pSymbol = SymbolTable_Find(m_pSymbolTable, &m_Key2, &m_Empty);
    CHECK(NULL != pSymbol);
    validateSymbolKeys(pSymbol, &m_Key2, &m_Empty);
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
    validateSymbolKeys(pSymbol, &m_Key1, &m_Empty);

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
    validateSymbolKeys(pSymbol, &m_Key1, &m_Empty);
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

TEST(SymbolTable, FailAllocationWhenAddingLineInfoToSymbol)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();
    
    MallocFailureInject_FailAllocation(1);
    __try_and_catch( Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo1) );
    MallocFailureInject_Restore();

    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    POINTERS_EQUAL(NULL, m_pSymbol1->pLineReferences);
    CHECK_FALSE(Symbol_LineReferenceExist(m_pSymbol1, &m_lineInfo1));
    clearExceptionCode();
}

TEST(SymbolTable, AddOneLineInfoToSymbol)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo1);

    Symbol_LineReferenceEnumStart(m_pSymbol1);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(m_pSymbol1);
    POINTERS_EQUAL(&m_lineInfo1, pLineInfo);
    nextLineEnumAttemptShouldFail(m_pSymbol1);
}

TEST(SymbolTable, AddSameLineInfoTwiceToSymbolAndMakeSureThatSecondIsIgnored)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo1);
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo1);

    Symbol_LineReferenceEnumStart(m_pSymbol1);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(m_pSymbol1);
    POINTERS_EQUAL(&m_lineInfo1, pLineInfo);
    nextLineEnumAttemptShouldFail(m_pSymbol1);
}

TEST(SymbolTable, AddTwoLineInfoToSymbolAndVerifyEnumerateInReverseOrder)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo1);
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo2);

    Symbol_LineReferenceEnumStart(m_pSymbol1);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(m_pSymbol1);
    POINTERS_EQUAL(&m_lineInfo2, pLineInfo);
    pLineInfo = Symbol_LineReferenceEnumNext(m_pSymbol1);
    POINTERS_EQUAL(&m_lineInfo1, pLineInfo);
    nextLineEnumAttemptShouldFail(m_pSymbol1);
}

TEST(SymbolTable, EnumerateEmptyLineReferenceList)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();

    Symbol_LineReferenceEnumStart(m_pSymbol1);
    nextLineEnumAttemptShouldFail(m_pSymbol1);
}

TEST(SymbolTable, RemoveOneItemFromLineReferenceList)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo1);
    Symbol_LineReferenceRemove(m_pSymbol1, &m_lineInfo1);

    Symbol_LineReferenceEnumStart(m_pSymbol1);
    nextLineEnumAttemptShouldFail(m_pSymbol1);
}

TEST(SymbolTable, AddTwoLineInfoToSymbolAndRemoveLastOne)
{
    m_pSymbolTable = SymbolTable_Create(1);
    createOneSymbol();
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo1);
    Symbol_LineReferenceAdd(m_pSymbol1, &m_lineInfo2);
    Symbol_LineReferenceRemove(m_pSymbol1, &m_lineInfo1);
    
    Symbol_LineReferenceEnumStart(m_pSymbol1);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(m_pSymbol1);
    POINTERS_EQUAL(&m_lineInfo2, pLineInfo);
    nextLineEnumAttemptShouldFail(m_pSymbol1);
}
