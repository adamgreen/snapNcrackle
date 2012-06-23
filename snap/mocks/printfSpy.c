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
size_t  g_BufferSize = 0;
int (*g_printfPrevious)(const char* pFormat, ...) = NULL;

static void _AllocateAndInitBuffer(size_t BufferSize)
{
    g_BufferSize = BufferSize + 1;
    g_pBuffer = malloc(g_BufferSize);
    CHECK_C(NULL != g_pBuffer);
    g_pBuffer[0] = '\0';
}

static void _FreeBuffer(void)
{
    free(g_pBuffer);
    g_pBuffer = NULL;
    g_BufferSize = 0;
}

static void _SetFunctionPointer(int (*pFunction)(const char*, ...))
{
    g_printfPrevious = __printf;
    __printf = pFunction;
}

static void _RestoreFunctionPointer(void)
{
    if (g_printfPrevious)
    {
        __printf = g_printfPrevious;
    }
}

/********************/
/* Public routines. */
/********************/
void printfSpy_Construct(size_t BufferSize)
{
    printfSpy_Destruct();

    _AllocateAndInitBuffer(BufferSize);
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
                            g_BufferSize,
                            pFormat,
                            valist);
    
    return WrittenSize;
}

const char* printfSpy_GetLastOutput(void)
{
    return g_pBuffer;
}

