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


unsigned int   g_allocationToFail = 0;
void*        (*g_mallocPrevious)(size_t size) = NULL;


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
void MallocFailureInject_Construct(unsigned int allocationToFail)
{
    g_mallocPrevious = __malloc;
    __malloc = MallocFailureInject_malloc;
    
    g_allocationToFail = allocationToFail;
}

void MallocFailureInject_Destruct(void)
{
    __malloc = g_mallocPrevious;
    g_mallocPrevious = NULL;
}
