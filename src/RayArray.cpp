#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Rays.h"
#include "RayArray.h"
#include "Stream.h"

#ifdef _WINDOWS
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define RAYARRAY_VERSION        ((DWORD)1)

CRayArray::CRayArray( int nSize )
{
    int     nCount;

    if ( nSize == 0 )
        nSize = 1;

    m_nSize = nSize;
    m_nItems = 0;
    m_aoItems = new CRay[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    {
        m_aoItems[ nCount ] = m_oEmpty;
        nCount++;
    }
}

CRayArray::~CRayArray( void )
{
    delete [] m_aoItems;
}

int CRayArray::Add( CRay &oRay )
{
    if ( m_nItems == m_nSize )
        Resize();

    m_aoItems[ m_nItems ] = oRay;
    m_nItems++;
    return (m_nItems - 1);
}

BOOL CRayArray::Delete( int nIndex )
{
    int     nCount;

    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return FALSE;

    nCount = nIndex;
    while ( nCount < (m_nItems - 1) )
    {
        m_aoItems[ nCount ] = m_aoItems[ nCount + 1];
        nCount++;
    }
    Trim();
    return TRUE;
}

int CRayArray::Insert( CRay &oRay, int nIndex )
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
        nCount--;
    }
    m_aoItems[ nIndex ] = oRay;

    m_nItems++;

    return nIndex;
}

CRay &CRayArray::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return m_oEmpty;
    return m_aoItems[ nIndex ];
}

CRay &CRayArray::operator[] ( int nIndex )
{
    return GetAt( nIndex );
}

void CRayArray::Resize( void )
{
    CRay       *pTemp;
    int         nCount;

    do
    {
        pTemp = new CRay[ m_nSize + 16 ];
    } while ( ! pTemp );

    nCount = 0;
    while ( nCount < (m_nSize + 16) )
    {
        pTemp[ nCount ] = m_oEmpty;
        nCount++;
    }

    nCount = 0;
    while ( nCount < m_nItems )
    {
        pTemp[ nCount ] = m_aoItems[ nCount ];
        nCount++;
    }
    m_nSize += 16;
}

void CRayArray::Trim( void )
{
    // This should really trim down the array. But hey, we've got gobbs of memory, so forget it.
}

void CRayArray::RemoveAll( void )
{
    m_nItems = 0;
    Trim();
}

CStream &operator >>( CStream &oStream, CRayArray &oList )
{
    DWORD               dwVersion;
    DWORD               dwCount;
    CRay                oRay;

    oStream >> dwVersion;
    switch ( dwVersion )
    {
        case RAYARRAY_VERSION:
            oList.RemoveAll();
            oStream >> dwCount;
            while ( dwCount )
            {
                oStream >> oRay;
                oList.Add( oRay );
                dwCount--;
            }
            break;

        default:
            break;
    }
    return oStream;
}

CStream &operator <<( CStream &oStream, CRayArray &oList )
{
    int                 nCount;

    oStream << RAYARRAY_VERSION;

    oStream << (DWORD)oList.GetSize();
    nCount = 0;
    while ( nCount < oList.GetSize() )
    {
        oStream << oList.GetAt( nCount );
        nCount++;
    }
    return oStream;
}
