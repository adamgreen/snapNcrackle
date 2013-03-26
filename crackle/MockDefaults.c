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
#include <stdlib.h>
#include <stdio.h>
#include "FileOpen.h"


/* Not using my test mocks in production so point hooks to Standard CRT functions. */
void*  (*hook_malloc)(size_t size) = malloc;
void*  (*hook_realloc)(void* ptr, size_t size) = realloc;
void   (*hook_free)(void* ptr) = free;
int    (*hook_printf)(const char* pFormat, ...) = printf;
int    (*hook_fprintf)(FILE* pFile, const char* pFormat, ...) = fprintf;
#ifdef FOPEN_IS_CASE_SENSITIVE
FILE*  (*hook_fopen)(const char* filename, const char* mode) = FileOpen;
#else
FILE*  (*hook_fopen)(const char* filename, const char* mode) = fopen;
#endif
int    (*hook_fseek)(FILE* stream, long offset, int whence) = fseek;
long   (*hook_ftell)(FILE* stream) = ftell;
size_t (*hook_fwrite)(const void* ptr, size_t size, size_t nitems, FILE* stream) = fwrite;
size_t (*hook_fread)(void* ptr, size_t size, size_t nitems, FILE* stream) = fread;
