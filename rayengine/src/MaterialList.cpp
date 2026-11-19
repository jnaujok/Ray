#ifdef _WINDOWS
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "MaterialList.h"

#define MATERIAL_LIST_VERSION       ((DWORD)1)

typedef CMaterial *LPMATERIAL;

#ifdef _WINDOWS
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CMaterialList::CMaterialList( int nSize )
{
    int     nCount;

    if ( nSize == 0 )
        nSize = 1;

    m_nSize = nSize;
    m_nItems = 0;
    m_apoItems = new LPMATERIAL[ m_nSize ];
    m_anInfluence = new FLT[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    {
        m_apoItems[ nCount ] = NULL;
        m_anInfluence[ nCount ] = 0.0;
        nCount++;
    }
}

CMaterialList::CMaterialList( const CMaterialList &oSrc )
{
    int     nCount;

    m_nSize = oSrc.m_nSize;
    m_nItems = oSrc.m_nItems;
    m_apoItems = new LPMATERIAL[ m_nSize ];
    m_anInfluence = new FLT[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    { 
        if ( nCount < m_nItems )
        {
            m_apoItems[ nCount ] = oSrc.m_apoItems[ nCount ];
            m_anInfluence[ nCount ] = oSrc.m_anInfluence[ nCount ];
        }
        else
        {
            m_apoItems[ nCount ] = NULL;
            m_anInfluence[ nCount ] = 0.0;
        }
        nCount++;
    }
}

CMaterialList &CMaterialList::operator =( const CMaterialList &oSrc )
{
    int     nCount;

    delete m_apoItems;
    delete m_anInfluence;

    m_nSize = oSrc.m_nSize;
    m_nItems = oSrc.m_nItems;
    m_apoItems = new LPMATERIAL[ m_nSize ];
    m_anInfluence = new FLT[ m_nSize ];
    nCount = 0;
    while ( nCount < m_nSize )
    { 
        if ( nCount < m_nItems )
        {
            m_apoItems[ nCount ] = oSrc.m_apoItems[ nCount ];
            m_anInfluence[ nCount ] = oSrc.m_anInfluence[ nCount ];
        }
        else
        {
            m_apoItems[ nCount ] = NULL;
            m_anInfluence[ nCount ] = 0.0;
        }
        nCount++;
    }

    return *this;
}

CMaterialList::~CMaterialList( void )
{
    delete [] m_apoItems;
    delete [] m_anInfluence;
}

int CMaterialList::Add( CMaterial *pObject, FLT nInfluence )
{
    if ( m_nItems == m_nSize )
        Resize();

    m_apoItems[ m_nItems ] = pObject;
    m_anInfluence[ m_nItems ] = nInfluence;
    m_nItems++;
    return (m_nItems - 1);
}

BOOL CMaterialList::Delete( int nIndex )
{
    int     nCount;

    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return FALSE;

    nCount = nIndex;
    while ( nCount < (m_nItems - 1) )
    {
        m_apoItems[ nCount ] = m_apoItems[ nCount + 1];
        m_anInfluence[ nCount ] = m_anInfluence[ nCount + 1];
        nCount++;
    }
    m_nItems--;
    Trim();
    return TRUE;
}

int CMaterialList::Insert( CMaterial *pObject, FLT nInfluence, int nIndex )
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
        m_anInfluence[ nCount ] = m_anInfluence[ nCount - 1];
        nCount--;
    }
    m_apoItems[ nIndex ] = pObject;
    m_anInfluence[ nIndex ] = nInfluence;

    m_nItems++;

    return nIndex;
}

CMaterial *CMaterialList::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return NULL;
    return m_apoItems[ nIndex ];
}

CMaterial *CMaterialList::operator[] ( int nIndex )
{
    return GetAt( nIndex );
}

FLT CMaterialList::GetInfluence( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= m_nItems ) )
        return NULL;
    return m_anInfluence[ nIndex ];
}    

void CMaterialList::SetInfluence( int nIndex, FLT nInfluence )
{
    if ( ( nIndex >= 0 ) || ( nIndex < m_nItems ) )
    {
        m_anInfluence[ nIndex ] = nInfluence;
    }
}

void CMaterialList::Resize( void )
{
    CMaterial     **pTemp;
    FLT            *pTempInf;
    int             nCount;

    pTemp = new LPMATERIAL[ m_nSize + 16 ];
    pTempInf = new FLT[ m_nSize + 16 ];

    nCount = 0;
    while ( nCount < m_nItems )
    {
        pTemp[ nCount ] = m_apoItems[ nCount ];
        pTempInf[ nCount ] = m_anInfluence[ nCount ];
        nCount++;
    }
    m_nSize += 16;
    delete [] m_apoItems;
    delete [] m_anInfluence;
    m_apoItems = pTemp;
    m_anInfluence = pTempInf;
}

void CMaterialList::Trim( void )
{
    // This should really trim down the array. But hey, we've got gobbs of memory, so forget it.
}


void CMaterialList::RemoveAll( void )
{
    m_nItems = 0;
    Trim();
}

CStream &operator >>( CStream &oStream, CMaterialList &oList )
{
    DWORD               dwVersion;
    DWORD               dwCount;
    CMaterial          *pMaterial;
    CString             strMaterial;
    CDynMaterialList    oMats;
    FLT                 nInfluence;

    oStream >> dwVersion;
    switch ( dwVersion )
    {
        case MATERIAL_LIST_VERSION:
            oList.RemoveAll();
            oStream >> dwCount;
            while ( dwCount )
            {
                oStream >> strMaterial;
                pMaterial = oMats.Instantiate( strMaterial, oStream );
                oStream >> nInfluence;
                oList.Add( pMaterial, nInfluence );
                dwCount--;
            }
            break;

        default:
            break;
    }
    return oStream;
}

CStream &operator <<( CStream &oStream, CMaterialList &oList )
{
    CMaterial          *pMaterial;
    CString             strMaterial;
    CDynMaterialList    oMats;
    FLT                 nInfluence;
    int                 nCount;

    oStream << MATERIAL_LIST_VERSION;

    oStream << (DWORD)oList.GetSize();
    nCount = 0;
    while ( nCount < oList.GetSize() )
    {
        pMaterial = oList[ nCount ];
        nInfluence = oList.GetInfluence( nCount );
        strMaterial = pMaterial->ClassType();
        oStream << strMaterial;
        pMaterial->Out( oStream );
        oStream << nInfluence;
        nCount++;
    }
    return oStream;
}
