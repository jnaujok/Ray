   // Load in all those big ugly math functions
#include <math.h>

#ifndef WORD
#ifdef SHOW_ME
#pragma message( "Defining WORD" )
#endif
typedef unsigned short     WORD;
#endif

#ifndef DWORD
#ifdef SHOW_ME
#pragma message( "Defining DWORD" )
#endif
typedef unsigned long      DWORD;
#endif

#ifndef _INC_WINDOWS
#ifndef INT
#ifdef SHOW_ME
#pragma message( "Defining INT" )
#endif
typedef short              INT;
#endif

#ifndef UINT           
#ifdef SHOW_ME
#pragma message( "Defining UINT" )
#endif
typedef unsigned short     UINT;
#endif

#ifndef BOOL
#ifdef SHOW_ME
#pragma message( "Defining BOOL" )
#endif
typedef  unsigned int      BOOL;
#define  FALSE             0x00
#define  TRUE              0x01
#endif

#endif // _INC_WINDOWS

#ifndef FLT
#ifdef __NUMERIC_COPROCESSOR
#ifdef SHOW_ME
#pragma message( "Defining FLT as long double" )
#endif
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
#ifdef SHOW_ME
#pragma message( "Defining FLT as double" )
#endif
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
#endif // Numeric coprocessor available on MS-Windows
#endif


#ifndef LPCSTR
#ifdef SHOW_ME
#pragma message( "Defining LPCSTR" )
#endif
typedef  const char *      LPCSTR;
#endif

#ifndef LPSTR
#ifdef SHOW_ME
#pragma message( "Defining LPSTR" )
#endif
typedef  char *            LPSTR;
#endif

#ifndef ASSERT
#ifdef _DEBUG
#ifdef SHOW_ME
#pragma message( "Defining Debugging ASSERT" )
#endif
#define  ASSERT( x )   { if ( ! x ) ::ASSERT_ShowError( #x, __FILE__, __LINE__ ); }
void ASSERT_ShowError( LPCSTR lpszAssertion, LPCSTR lpszFileName, int nLineNumber );
#else // ! _DEBUG
#ifdef SHOW_ME
#pragma message( "Defining NULL ASSERT" )
#endif
#define ASSERT( x ) (NULL)
#endif // _DEBUG
#endif // _ASSERT

#ifndef CFUNC
#ifdef _WINDOWS
#ifdef SHOW_ME
#pragma message( "Defining CFUNC" )
#endif
#define CFUNC     __cdecl
#else
#define CFUNC
#endif
#endif

#ifndef PI
#define PI  ((FLT)3.1415926536897932)
#ifdef SHOW_ME
#pragma message( "Defining PI" )
#endif
#endif

#ifndef RADIANS
#define RADIANS  (PI / (FLT)180.0)
#ifdef SHOW_ME
#pragma message( "Defining RADIANS" )
#endif
#endif


#ifndef _WINDOWS
    #include <stdlib.h>
    #include "CString.h"
#endif

