/*************************************************************************
*   Globals                                                              *
*                                                                        *
*   Basic types and math macros shared by every part of the ray tracer.  *
*   On Windows the Win32 headers already supply the Windows types;       *
*   everywhere else they are defined here with the same sizes as on      *
*   Win32, so scene files and network messages agree.                    *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "BaseTypes/IASLib_String.h"
#undef ASSERT

#if defined( _WIN32 )

    // On Windows, <windows.h> (included by IASLib's headers)
    // already defines BYTE, WORD, DWORD, BOOL, LPCSTR, TRUE and FALSE.
#ifndef ASSERT
#include <assert.h>
#define ASSERT( x )     assert( x )
#endif

#else

typedef uint8_t         BYTE;
typedef uint16_t        WORD;
typedef uint32_t        DWORD;
typedef int             BOOL;
typedef const char     *LPCSTR;
typedef char           *LPSTR;

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif

#ifndef ASSERT
#include <assert.h>
#define ASSERT( x )     assert( x )
#endif

#endif // _WIN32

#ifndef CFUNC
#if defined( _WIN32 )
#define CFUNC           __cdecl
#else
#define CFUNC
#endif
#endif

#ifndef FLT
#ifdef __NUMERIC_COPROCESSOR
typedef long double         FLT;
#define ABS(x)              fabsl(x)
#define TAN(x)              tanl(x)
#define SIN(x)              sinl(x)
#define COS(x)              cosl(x)
#define ASIN(x)             asinl(x)
#define ACOS(x)             acosl(x)
#define SQRT(x)             (long double)sqrt((double)x)
#define POW(x,y)            (long double)pow((double)x, (double)y)
#define EXP(x)              expl(x)
#define LOG(x)              log10l(x)
#else
typedef double              FLT;
#define ABS(x)              fabs(x)
#define SIN(x)              sin(x)
#define COS(x)              cos(x)
#define TAN(x)              tan(x)
#define ASIN(x)             asin(x)
#define ACOS(x)             acos(x)
#define SQRT(x)             sqrt(x)
#define POW(x,y)            pow(x, y)
#define EXP(x)              exp(x)
#define LOG(x)              log10(x)
#endif
#endif

#ifndef PI
#define PI  ((FLT)3.1415926536897932)
#endif

#ifndef RADIANS
#define RADIANS  (PI / (FLT)180.0)
#endif

#endif // __GLOBALS_H__
