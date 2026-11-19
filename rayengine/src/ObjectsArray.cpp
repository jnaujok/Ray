#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Objects.h"
#include "ObjectsArray.h"

#ifdef _WINDOWS
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

typedef CObjects *LPOBJECTS;

CObjectsArray::CObjectsArray( int nSize )
{
    int     nCount;

    if ( nSize == 0 )
        nSize = 1;

    m_nSize = nSize;
    m_nItems = 0;
    m_apoItems = new LPOBJECTS[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    {
        m_apoItems[ nCount ] = NULL;
        nCount++;
    }
}

CObjectsArray::~CObjectsArray( void )
{
    delete m_apoItems;
}

int CObjectsArray::Add( CObjects *pObject )
{
    if ( m_nItems == m_nSize )
        Resize();

    m_apoItems[ m_nItems ] = pObject;
    m_nItems++;
    return (m_nItems - 1);
}

BOOL CObjectsArray::Delete( int nIndex )
{
    int     nCount;

    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return FALSE;

    nCount = nIndex;
    while ( nCount < (m_nItems - 1) )
    {
        m_apoItems[ nCount ] = m_apoItems[ nCount + 1];
        nCount++;
    }
    Trim();
    return TRUE;
}

int CObjectsArray::Insert( CObjects *pObject, int nIndex )
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
        m_apoItems[ nCount ] = m_apoItems[ nCount - 1 ];
        nCount--;
    }
    m_apoItems[ nIndex ] = pObject;

    m_nItems++;

    return nIndex;
}

CObjects *CObjectsArray::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return NULL;
    return m_apoItems[ nIndex ];
}

CObjects *CObjectsArray::operator[] ( int nIndex )
{
    return GetAt( nIndex );
}

void CObjectsArray::Resize( void )
{
    CObjects    **pTemp;
    int         nCount;

    pTemp = new LPOBJECTS[ m_nSize + 16 ];

    nCount = 0;
    while ( nCount < m_nItems )
    {
        pTemp[ nCount ] = m_apoItems[ nCount ];
        nCount++;
    }
    m_nSize += 16;
}

void CObjectsArray::Trim( void )
{
    // This should really trim down the array. But hey, we've got gobbs of memory, so forget it.
}

void CObjectsArray::RemoveAll( void )
{
    int     nCount;

    nCount = 0;
    while ( nCount < m_nItems )
    {
        delete m_apoItems[ nCount ];
        nCount++;
    }
    m_nItems = 0;
    Trim();
}

