#ifdef _WINDOWS
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Objects.h"
#include "FileStream.h"
#include <stdio.h>
#include <string.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFileStream::CFileStream( LPCSTR strFilename )
{
    m_fpStreamFile = fopen( strFilename, "r+b" );

    if ( m_fpStreamFile == NULL )
    {
        m_fpStreamFile = fopen( strFilename, "w+b" );
        m_dwFileSize = 0;
    }
    else
    {
        fseek( m_fpStreamFile, 0, SEEK_END );
        m_dwFileSize = (DWORD)ftell( m_fpStreamFile );
        fseek( m_fpStreamFile, 0, SEEK_SET );
    }

}

CFileStream::~CFileStream( void )
{
    fclose( m_fpStreamFile );
}


void CFileStream::Write( char nChar )
{
    fwrite( &nChar, sizeof( char ), 1, m_fpStreamFile );
}

void CFileStream::Write( unsigned char nUChar )
{
    fwrite( &nUChar, sizeof( char ), 1, m_fpStreamFile );
}

void CFileStream::Write( short nShort )
{
    Write( (unsigned short)nShort );
}

void CFileStream::Write( unsigned short nUShort )
{
    unsigned char   nHigh;
    unsigned char   nLow;

    nHigh = (unsigned char)(nUShort / 256);
    nLow = (unsigned char)(nUShort % 256);
    Write( nHigh );
    Write( nLow );
}

void CFileStream::Write( unsigned long nULong )
{
    unsigned short nHigh, nLow;

    nHigh = (unsigned short)( nULong / (unsigned long)65536 );
    nLow = (unsigned short)( nULong % (unsigned long)65536 );
    Write( nHigh );
    Write( nLow );
}

void CFileStream::Write( long nLong )
{
    Write( (unsigned long)nLong );
}

void CFileStream::Write( float nFloat )
{
    char            Buffer[ 64 ];

    sprintf( Buffer, "%.10e", nFloat );

    Write( Buffer );
}

void CFileStream::Write( double nDouble )
{
    char            Buffer[ 64 ];

    sprintf( Buffer, "%.18e", nDouble );

    Write( Buffer );
}

void CFileStream::Write( LPCSTR strString )
{
    unsigned short  nLength;
    int             nCount;

    nLength = strlen( strString );
    Write( nLength );

    nCount = 0;
    while ( nCount < nLength )
    {
        Write( strString[ nCount ] );
        nCount++;
    }
}

void CFileStream::Read( char &nChar )
{
    fread( &nChar, sizeof( char ), 1, m_fpStreamFile );
}

void CFileStream::Read( unsigned char &nUChar )
{
    fread( &nUChar, sizeof( unsigned char ), 1, m_fpStreamFile );
}

void CFileStream::Read( short &nShort )
{
    unsigned short nUShort;

    Read( nUShort );
    nShort = (short)nUShort;
}

void CFileStream::Read( unsigned short &nUShort )
{
    unsigned char   nLow;
    unsigned char   nHigh;

    Read( nHigh );
    Read( nLow );
    nUShort = (unsigned short)((unsigned short)nHigh * (unsigned short)256) + (unsigned short)nLow;
}

void CFileStream::Read( unsigned long &nULong )
{
    unsigned short  nHigh, nLow;

    Read( nHigh );
    Read( nLow );
    nULong = (unsigned long)((unsigned long)nHigh * (unsigned long)65536) + (unsigned long)nLow;
}

void CFileStream::Read( long &nLong )
{
    unsigned long   nULong;

    Read( nULong );
    nLong = (long)nULong;
}

void CFileStream::Read( float &nFloat )
{
    CString     strValue;
    char        *stop;

    Read( strValue );

    nFloat = (float)strtod( strValue, &stop );
}

void CFileStream::Read( double &nDouble )
{
    CString     strValue;
    char        *stop;

    Read( strValue );

    nDouble = strtod( strValue, &stop );
/*
    long    nWhole;
    long    nDecimal;

    Read( nWhole );
    Read( nDecimal );

    nDouble = (double)((double)nDecimal / 2147483648.0 );
    nDouble += (double)nWhole;
*/
}

void CFileStream::Read( CString &strString )
{
    unsigned short  nLength;
    int             nCount;
    char            chTemp;

    strString = "";
    Read( nLength );

    nCount = 0;
    while ( nCount < nLength )
    {
        Read( chTemp );
        strString += chTemp;
        nCount++;
    }
}

void CFileStream::Flush( void )
{
    fflush( m_fpStreamFile );
}

BOOL CFileStream::DataAvailable( void )
{
    if ( ftell( m_fpStreamFile ) == (long)m_dwFileSize )
        return FALSE;
    return TRUE;
}


