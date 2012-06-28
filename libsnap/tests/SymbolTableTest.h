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
/* Used to redirect specific calls to stubs as necessary for testing. */
#ifndef _SYMBOL_TABLE_TEST_H_
#define _SYMBOL_TABLE_TEST_H_

#include <MallocFailureInject.h>


/* Force malloc() to go through function pointer so that memory failures can be injected. */
#undef  malloc
#define malloc hook_malloc

/* Force free() to go through function pointer so that it doesn't need CppUTest to do leak detection in production. */
#undef  free
#define free hook_free

#endif /* _SYMBOL_TABLE_TEST_H_ */
