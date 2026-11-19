#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "RayStream.h"
#include "ColorArray.h"

#define COLOR_ARRAY_VERSION       ((DWORD)1)

CColorArray::CColorArray( int nSize ) : m_oBlack( 0, 0, 0 )
{
    if ( nSize > 0 )
    {
        m_aoItems.Reserve( (size_t)nSize );
        m_anXCoords.Reserve( (size_t)nSize );
        m_anYCoords.Reserve( (size_t)nSize );
    }
}

int CColorArray::Add( const CColor &oColor, int nX, int nY )
{
    m_anXCoords.Push( nX );
    m_anYCoords.Push( nY );
    return (int)m_aoItems.Push( oColor );
}

BOOL CColorArray::Delete( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return FALSE;
    m_aoItems.Remove( (size_t)nIndex );
    m_anXCoords.Remove( (size_t)nIndex );
    m_anYCoords.Remove( (size_t)nIndex );
    return TRUE;
}

int CColorArray::Insert( const CColor &oColor, int nX, int nY, int nIndex )
{
    if ( nIndex < 0 )
        nIndex = 0;
    if ( nIndex > GetSize() )
        nIndex = GetSize();
    m_anXCoords.Insert( (size_t)nIndex, nX );
    m_anYCoords.Insert( (size_t)nIndex, nY );
    return (int)m_aoItems.Insert( (size_t)nIndex, oColor );
}

CColor &CColorArray::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return m_oBlack;
    return m_aoItems[ (size_t)nIndex ];
}

void CColorArray::RemoveAll( void )
{
    m_aoItems.Clear();
    m_anXCoords.Clear();
    m_anYCoords.Clear();
}

int CColorArray::GetX( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return 0;
    return m_anXCoords[ (size_t)nIndex ];
}

void CColorArray::SetX( int nIndex, int nX )
{
    if ( ( nIndex >= 0 ) && ( nIndex < GetSize() ) )
        m_anXCoords[ (size_t)nIndex ] = nX;
}

int CColorArray::GetY( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return 0;
    return m_anYCoords[ (size_t)nIndex ];
}

void CColorArray::SetY( int nIndex, int nY )
{
    if ( ( nIndex >= 0 ) && ( nIndex < GetSize() ) )
        m_anYCoords[ (size_t)nIndex ] = nY;
}

CRayStream &operator >>( CRayStream &oStream, CColorArray &oList )
{
    DWORD               dwVersion;
    DWORD               dwCount;
    CColor              oColor;
    DWORD               nX, nY;

    oStream >> dwVersion;
    switch ( dwVersion )
    {
        case COLOR_ARRAY_VERSION:
            oList.RemoveAll();
            oStream >> dwCount;
            while ( dwCount )
            {
                oStream >> nX;
                oStream >> nY;
                oStream >> oColor;
                oList.Add( oColor, (int)nX, (int)nY );
                dwCount--;
            }
            break;

        default:
            break;
    }
    return oStream;
}

CRayStream &operator <<( CRayStream &oStream, CColorArray &oList )
{
    int                 nCount;

    oStream << COLOR_ARRAY_VERSION;

    oStream << (DWORD)oList.GetSize();
    nCount = 0;
    while ( nCount < oList.GetSize() )
    {
        oStream << (DWORD)oList.GetX(nCount);
        oStream << (DWORD)oList.GetY(nCount);
        oStream << oList[nCount];
        nCount++;
    }
    return oStream;
}
