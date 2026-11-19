#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Globals.h"
#include "Rays.h"
#include "RayArray.h"

#define RAYARRAY_VERSION        ((DWORD)1)

CRayArray::CRayArray( int nSize )
{
    if ( nSize > 0 )
        m_aoItems.Reserve( (size_t)nSize );
}

int CRayArray::Add( const CRay &oRay )
{
    return (int)m_aoItems.Push( oRay );
}

BOOL CRayArray::Delete( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return FALSE;
    m_aoItems.Remove( (size_t)nIndex );
    return TRUE;
}

int CRayArray::Insert( const CRay &oRay, int nIndex )
{
    if ( nIndex < 0 )
        nIndex = 0;
    if ( nIndex > GetSize() )
        nIndex = GetSize();
    return (int)m_aoItems.Insert( (size_t)nIndex, oRay );
}

CRay &CRayArray::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return m_oEmpty;
    return m_aoItems[ (size_t)nIndex ];
}
