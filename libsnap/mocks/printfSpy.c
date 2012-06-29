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
/* Module for spying on printf output from code under test. */
#include <stdio.h>
#include <stdarg.h>
#include "printfSpy.h"
#include "CppUTest/TestHarness_c.h"


char*   g_pBuffer = NULL;
size_t  g_bufferSize = 0;
size_t  g_callCount = 0;
FILE*   g_pFile = NULL;
int (*hook_printf)(const char* pFormat, ...) = printf;
int (*hook_fprintf)(FILE* pFile, const char* pFormat, ...) = fprintf;

static void _AllocateAndInitBuffer(size_t BufferSize)
{
    g_bufferSize = BufferSize + 1;
    g_pBuffer = malloc(g_bufferSize);
    CHECK_C(NULL != g_pBuffer);
    g_pBuffer[0] = '\0';
    g_pFile = NULL;
}

static void _FreeBuffer(void)
{
    free(g_pBuffer);
    g_pBuffer = NULL;
    g_bufferSize = 0;
    g_pFile = NULL;
}

static int mock_common(FILE* pFile, const char* pFormat, va_list valist)
{
    int     WrittenSize = -1;
    
    WrittenSize = vsnprintf(g_pBuffer,
                            g_bufferSize,
                            pFormat,
                            valist);
    g_callCount++;
    g_pFile = pFile;
    return WrittenSize;
}

static int mock_printf(const char* pFormat, ...)
{
    va_list valist;
    va_start(valist, pFormat);
    return mock_common(stdout, pFormat, valist);
}

static int mock_fprintf(FILE* pFile, const char* pFormat, ...)
{
    va_list valist;
    va_start(valist, pFormat);
    return mock_common(pFile, pFormat, valist);
}

static void setHookFunctionPointers(void)
{
    hook_printf = mock_printf;
    hook_fprintf = mock_fprintf;
}

static void restoreHookFunctionPointers(void)
{
    hook_printf = printf;
    hook_fprintf = fprintf;
}


/********************/
/* Public routines. */
/********************/
void printfSpy_Hook(size_t BufferSize)
{
    printfSpy_Unhook();

    _AllocateAndInitBuffer(BufferSize);
    g_callCount = 0;
    setHookFunctionPointers();
}

void printfSpy_Unhook(void)
{
    restoreHookFunctionPointers();
    _FreeBuffer();
}

const char* printfSpy_GetLastOutput(void)
{
    return g_pBuffer;
}

FILE* printfSpy_GetLastFile(void)
{
    return g_pFile;
}

size_t printfSpy_GetCallCount(void)
{
    return g_callCount;
}
