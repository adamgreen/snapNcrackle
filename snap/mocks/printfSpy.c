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
int (*__printf)(const char* pFormat, ...) = printf;

static void _AllocateAndInitBuffer(size_t BufferSize)
{
    g_bufferSize = BufferSize + 1;
    g_pBuffer = malloc(g_bufferSize);
    CHECK_C(NULL != g_pBuffer);
    g_pBuffer[0] = '\0';
}

static void _FreeBuffer(void)
{
    free(g_pBuffer);
    g_pBuffer = NULL;
    g_bufferSize = 0;
}

static void _SetFunctionPointer(int (*pFunction)(const char*, ...))
{
    __printf = pFunction;
}

static void _RestoreFunctionPointer(void)
{
    __printf = printf;
}

/********************/
/* Public routines. */
/********************/
void printfSpy_Construct(size_t BufferSize)
{
    printfSpy_Destruct();

    _AllocateAndInitBuffer(BufferSize);
    g_callCount = 0;
    _SetFunctionPointer(printfSpy_printf);
}

void printfSpy_Destruct(void)
{
    _RestoreFunctionPointer();
    _FreeBuffer();
}

int printfSpy_printf(const char* pFormat, ...)
{
    int     WrittenSize = -1;
    va_list valist;
    
    va_start(valist, pFormat);
    WrittenSize = vsnprintf(g_pBuffer,
                            g_bufferSize,
                            pFormat,
                            valist);
    g_callCount++;
    return WrittenSize;
}

const char* printfSpy_GetLastOutput(void)
{
    return g_pBuffer;
}

size_t printfSpy_GetCallCount(void)
{
    return g_callCount;
}
