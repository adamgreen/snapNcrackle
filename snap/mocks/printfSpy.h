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
#ifndef _PRINTF_SPY_H_
#define _PRINTF_SPY_H_


/* Pointer to printf routine which can intercepted by this module. */
extern int (*__printf)(const char* pFormat, ...);

void        printfSpy_Construct(size_t BufferSize);
int         printfSpy_printf(const char* pFormat, ...);
const char* printfSpy_GetLastOutput(void);
void        printfSpy_Destruct(void);

#endif /* _PRINTF_SPY_H_ */
