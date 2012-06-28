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
/* Module for injecting allocation failures into malloc() calls from code under test. */
#ifndef _MALLOC_FAILURE_INJECT_H_
#define _MALLOC_FAILURE_INJECT_H_


/* Pointer to malloc routine which can intercepted by this module. */
extern void* (*hook_malloc)(size_t size);

/* Provide a hook for free as well so that production code can skip leak detection. */
extern void  (*hook_free)(void* ptr);

void        MallocFailureInject_FailAllocation(unsigned int allocationToFail);
void        MallocFailureInject_Restore(void);

#endif /* _MALLOC_FAILURE_INJECT_H_ */
