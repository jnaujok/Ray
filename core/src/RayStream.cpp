/*************************************************************************
*   CRayStream                                                           *
*                                                                        *
*   Floating-point values travel as text so that scene files written on  *
*   one platform read back identically on another. "%.18e" carries more  *
*   than the 17 significant digits a double needs to round-trip.         *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifdef RAY_GUI
    #include "StdAfx.h"
#endif

#include "RayStream.h"

#include <stdio.h>
#include <string.h>
#include <string>

BOOL CRayStream::DataAvailable( void )
{
    size_t  nRemaining = m_oStream.bytesRemaining();

    if ( nRemaining != (size_t)IASLib::NOT_FOUND )
        return ( nRemaining > 0 ) ? TRUE : FALSE;
    return m_oStream.IsEOS() ? FALSE : TRUE;
}

void CRayStream::WriteFloat( float nValue )
{
    char    achBuffer[ 64 ];

    snprintf( achBuffer, sizeof( achBuffer ), "%.10e", (double)nValue );
    WriteString( achBuffer );
}

void CRayStream::WriteDouble( double nValue )
{
    char    achBuffer[ 64 ];

    snprintf( achBuffer, sizeof( achBuffer ), "%.18e", nValue );
    WriteString( achBuffer );
}

void CRayStream::WriteString( const char *strValue )
{
    size_t  nLength = ( strValue ) ? strlen( strValue ) : 0;

        // The length is stored in 16 bits.
    if ( nLength > 0xFFFF )
        nLength = 0xFFFF;

    WriteUInt16( (uint16_t)nLength );
    if ( nLength )
        m_oStream.PutBuffer( strValue, (int)nLength );
}

float CRayStream::ReadFloat( void )
{
    if ( m_bLegacyDoubles )
        return (float)ReadDouble();

    IASLib::CString strValue = ReadString();

    return (float)strtod( (const char *)strValue, NULL );
}

double CRayStream::ReadDouble( void )
{
    if ( m_bLegacyDoubles )
    {
            // The decoding the original CFileStream used for this layout.
        int32_t nWhole = (int32_t)ReadUInt32();
        int32_t nDecimal = (int32_t)ReadUInt32();

        return ( (double)nDecimal / 2147483648.0 ) + (double)nWhole;
    }

    IASLib::CString strValue = ReadString();

    return strtod( (const char *)strValue, NULL );
}

IASLib::CString CRayStream::ReadString( void )
{
    uint16_t    nLength = ReadUInt16();
    std::string strValue( nLength, '\0' );

    if ( nLength )
        m_oStream.GetExact( &strValue[ 0 ], (int)nLength );

    return IASLib::CString( strValue.c_str(), (int)nLength );
}
