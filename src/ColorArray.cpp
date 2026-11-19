#ifdef _WINDOWS
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ColorArray.h"

#define COLOR_ARRAY_VERSION       ((DWORD)1)

#ifdef _WINDOWS
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CColorArray::CColorArray( int nSize )
{
    int     nCount;

    if ( nSize == 0 )
        nSize = 1;

    m_nSize = nSize;
    m_nItems = 0;
    m_aoItems = new CColor[ m_nSize ];
    m_anXCoords = new int[ m_nSize ];
    m_anYCoords = new int[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    {
        m_anXCoords[ nCount ] = 0;
        m_anYCoords[ nCount ] = 0;
        nCount++;
    }
}

CColorArray::CColorArray( const CColorArray &oSrc )
{
    int     nCount;

    m_nSize = oSrc.m_nSize;
    m_nItems = oSrc.m_nItems;
    m_aoItems = new CColor[ m_nSize ];
    m_anXCoords = new int[ m_nSize ];
    m_anYCoords = new int[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    { 
        if ( nCount < m_nItems )
        {
            m_aoItems[ nCount ] = oSrc.m_aoItems[ nCount ];
            m_anXCoords[ nCount ] = oSrc.m_anXCoords[ nCount ];
            m_anYCoords[ nCount ] = oSrc.m_anYCoords[ nCount ];
        }
        nCount++;
    }
}

CColorArray &CColorArray::operator =( const CColorArray &oSrc )
{
    int     nCount;

    delete [] m_aoItems;
    delete [] m_anXCoords;
    delete [] m_anYCoords;

    m_nSize = oSrc.m_nSize;
    m_nItems = oSrc.m_nItems;
    m_aoItems = new CColor[ m_nSize ];
    m_anXCoords = new int[ m_nSize ];
    m_anYCoords = new int[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    { 
        if ( nCount < m_nItems )
        {
            m_aoItems[ nCount ] = oSrc.m_aoItems[ nCount ];
            m_anXCoords[ nCount ] = oSrc.m_anXCoords[ nCount ];
            m_anYCoords[ nCount ] = oSrc.m_anYCoords[ nCount ];
        }
        nCount++;
    }

    return *this;
}

CColorArray::~CColorArray( void )
{
    RemoveAll();

    delete [] m_aoItems;
    delete [] m_anXCoords;
    delete [] m_anYCoords;
}

int CColorArray::Add( CColor &oColor, int nX, int nY )
{
    if ( m_nItems == m_nSize )
        Resize();

    m_aoItems[ m_nItems ] = oColor;
    m_anXCoords[ m_nItems ] = nX;
    m_anYCoords[ m_nItems ] = nY;
    m_nItems++;
    return (m_nItems - 1);
}

BOOL CColorArray::Delete( int nIndex )
{
    int     nCount;

    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return FALSE;

    nCount = nIndex;
    while ( nCount < (m_nItems - 1) )
    {
        m_aoItems[ nCount ] = m_aoItems[ nCount + 1];
        m_anXCoords[ nCount ] = m_anXCoords[ nCount + 1 ];
        m_anYCoords[ nCount ] = m_anYCoords[ nCount + 1 ];
        nCount++;
    }
    m_nItems--;
    Trim();
    return TRUE;
}

int CColorArray::Insert( CColor &oColor, int nX, int nY, int nIndex )
{
    int     nCount;

    if ( nIndex < 0 )
        nIndex = 0;

    if ( nIndex > m_nItems )
        nIndex = m_nItems;

    if ( m_nItems == m_nSize )
        Resize();

    nCount = m_nItems;

    while ( nCount > nIndex )
    {
        m_aoItems[ nCount ] = m_aoItems[ nCount - 1 ];
        m_anXCoords[ nCount ] = m_anXCoords[ nCount - 1 ];
        m_anYCoords[ nCount ] = m_anYCoords[ nCount - 1 ];
        nCount--;
    }
    m_aoItems[ nIndex ] = oColor;
    m_anXCoords[ nIndex ] = nX;
    m_anYCoords[ nIndex ] = nY;

    m_nItems++;

    return nIndex;
}

CColor &CColorArray::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return m_oBlack;
    return m_aoItems[ nIndex ];
}

CColor &CColorArray::operator[] ( int nIndex )
{
    return GetAt( nIndex );
}

int CColorArray::GetX( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return NULL;
    return m_anXCoords[ nIndex ];
}    

void CColorArray::SetX( int nIndex, int nX )
{
    if ( ( nIndex >= 0 ) || ( nIndex < m_nItems ) )
    {
        m_anXCoords[ nIndex ] = nX;
    }
}

int CColorArray::GetY( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return NULL;
    return m_anYCoords[ nIndex ];
}    

void CColorArray::SetY( int nIndex, int nX )
{
    if ( ( nIndex >= 0 ) || ( nIndex < m_nItems ) )
    {
        m_anYCoords[ nIndex ] = nX;
    }
}

void CColorArray::Resize( void )
{
    CColor         *pTemp;
    int            *pTempX;
    int            *pTempY;
    int             nCount;

    pTemp = new CColor[ m_nSize + 16 ];
    pTempX = new int[ m_nSize + 16 ];
    pTempY = new int[ m_nSize + 16 ];

    nCount = 0;
    while ( nCount < m_nItems )
    {
        pTemp[ nCount ] = m_aoItems[ nCount ];
        pTempX[ nCount ] = m_anXCoords[ nCount ];
        pTempY[ nCount ] = m_anYCoords[ nCount ];
        nCount++;
    }
    m_nSize += 16;
    delete [] m_aoItems;
    delete [] m_anXCoords;
    delete [] m_anYCoords;
    m_aoItems = pTemp;
    m_anXCoords = pTempX;
    m_anYCoords = pTempY;
}

void CColorArray::Trim( void )
{
    // This should really trim down the array. But hey, we've got gobbs of memory, so forget it.
}


void CColorArray::RemoveAll( void )
{
    m_nItems = 0;
    Trim();
}

CStream &operator >>( CStream &oStream, CColorArray &oList )
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

CStream &operator <<( CStream &oStream, CColorArray &oList )
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
