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
/* Module for injecting allocation failures into realloc() calls from code under test. */
#include "MallocFailureInject.h"
#include "CppUTest/TestHarness_c.h"


static void* defaultMalloc(size_t size);
static void  defaultFree(void* ptr);

void* (*hook_malloc)(size_t size) = defaultMalloc;
void  (*hook_free)(void* ptr) = defaultFree;

unsigned int   g_allocationToFail = 0;


/* Mallocs default to this routine so that the memory leak detection macros from the CppUTest code will be used. */
static void* defaultMalloc(size_t size)
{
    return malloc(size);
}

static void defaultFree(void* ptr)
{
    free(ptr);
}

static int shouldThisAllocationBeFailed(void)
{
    return g_allocationToFail && 0 == --g_allocationToFail;
}

static void* MallocFailureInject_malloc(size_t size)
{
    if (shouldThisAllocationBeFailed())
        return NULL;
    else
        return malloc(size);
}


/********************/
/* Public routines. */
/********************/
void MallocFailureInject_FailAllocation(unsigned int allocationToFail)
{
    hook_malloc = MallocFailureInject_malloc;
    g_allocationToFail = allocationToFail;
}

void MallocFailureInject_Restore(void)
{
    hook_malloc = defaultMalloc;
}
