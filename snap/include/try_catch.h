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
/* Very rough exception handling like macros for C. */
#ifndef _TRY_CATCH_H_
#define _TRY_CATCH_H_

#define noException                         0
#define bufferOverrunException              1
#define invalidHexDigitException            2
#define invalidValueException               3
#define invalidArgumentException            4
#define timeoutException                    5
#define invalidIndexException               6
#define notFoundException                   7
#define exceededHardwareResourcesException  8
#define invalidDecDigitException            9
#define outOfMemoryException                10
#define endOfFileException                  11
#define fileNotFoundException               12
#define fileException                       13

extern int g_exceptionCode;

/* On Linux, it is possible that __try and __catch are already defined. */
#undef __try
#undef __catch

#define __throws

#define __try \
        do \
        { \
            clearExceptionCode();

#define __throwing_func(X) \
            X; \
            if (g_exceptionCode) \
                break;

#define __catch \
        } while (0); \
        if (g_exceptionCode)

#define __throw(EXCEPTION) { setExceptionCode(EXCEPTION); return; }

#define __throw_and_return(EXCEPTION, RETURN) return (setExceptionCode(EXCEPTION), (RETURN))
        
#define __rethrow return

#define __rethrow_and_return(RETURN) return RETURN

static inline int getExceptionCode(void)
{
    return g_exceptionCode;
}

static inline void setExceptionCode(int exceptionCode)
{
    g_exceptionCode = exceptionCode > g_exceptionCode ? exceptionCode : g_exceptionCode;
}

static inline void clearExceptionCode(void)
{
    g_exceptionCode = noException;
}

#endif /* _TRY_CATCH_H_ */
