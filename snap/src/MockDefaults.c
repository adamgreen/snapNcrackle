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
/* Default to mocks for certain functions but production code can override. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int (*__printf)(const char* pFormat, ...) = printf;

/* Mallocs default to this routine so that the memory leak detection macros from the CppUTest code will be used. */
static void* defaultMalloc(size_t size)
{
    return malloc(size);
}
void* (*__malloc)(size_t size) = defaultMalloc;
