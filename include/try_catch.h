/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)

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

#include <setjmp.h>

#define noException                         0
#define bufferOverrunException              1
#define outOfMemoryException                2
#define fileOpenException                   3
#define fileException                       4
#define invalidHexDigitException            5
#define invalidBinaryDigitException         6
#define invalidDecimalDigitException        7
#define invalidArgumentException            8
#define invalidSourceOffsetException        9
#define invalidLengthException              10
#define invalidSideException                11
#define invalidTrackException               12
#define invalidSectorException              13
#define invalidIntraBlockOffsetException    14
#define invalidIntraTrackOffsetException    15
#define blockExceedsImageBoundsException    16
#define invalidInsertionTypeException       17
#define missingOperandException             18
#define invalidArgumentCountException       19
#define encounteredCommentException         20
#define badTrackException                   21


#ifndef __debugbreak
#define __debugbreak()  { __asm volatile ("int3"); }
#endif


typedef struct ExceptionHandler
{
    struct ExceptionHandler* pPrevious;
    jmp_buf*                 pJumpBuffer;
} ExceptionHandler;


extern ExceptionHandler* g_pExceptionHandlers;
extern int               g_exceptionCode;


/* On Linux, it is possible that __try and __catch are already defined. */
#undef __try
#undef __catch

#define __throws

#define __try \
        do \
        { \
            jmp_buf jumpBuffer; \
            struct ExceptionHandler exceptionHandler; \
            exceptionHandler.pPrevious = g_pExceptionHandlers; \
            exceptionHandler.pJumpBuffer = &jumpBuffer; \
            g_pExceptionHandlers = &exceptionHandler; \
            clearExceptionCode(); \
            \
            if (0 == setjmp(jumpBuffer)) \
            { \
            
#define __catch \
            } \
            g_pExceptionHandlers = exceptionHandler.pPrevious; \
        } while(0); \
        if (g_exceptionCode)

#define __throw(EXCEPTION) \
        { \
            setExceptionCode(EXCEPTION); \
            if (!g_pExceptionHandlers) \
            { \
                __debugbreak(); \
                exit(-1); \
            } \
            else \
            { \
                longjmp(*g_pExceptionHandlers->pJumpBuffer, 1); \
                exit(-1); \
            } \
        }

#define __rethrow __throw(getExceptionCode())

#define __nothrow { clearExceptionCode(); return; }

#define __nothrow_and_return(RETURN) return (clearExceptionCode(), (RETURN))

#define __try_and_catch(X) __try(X); __catch { }


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
