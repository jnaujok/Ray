#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "LightArray.h"
#include "Light.h"

CLightArray::CLightArray( int nSize )
{
    if ( nSize > 0 )
        m_apoItems.Reserve( (size_t)nSize );
}

int CLightArray::Add( CLight *pObject )
{
    return (int)m_apoItems.Push( pObject );
}

BOOL CLightArray::Delete( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return FALSE;
    m_apoItems.Remove( (size_t)nIndex );
    return TRUE;
}

int CLightArray::Insert( CLight *pObject, int nIndex )
{
    if ( nIndex < 0 )
        nIndex = 0;
    if ( nIndex > GetSize() )
        nIndex = GetSize();
    return (int)m_apoItems.Insert( (size_t)nIndex, pObject );
}

CLight *CLightArray::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return NULL;
    return m_apoItems[ (size_t)nIndex ];
}

void CLightArray::RemoveAll( void )
{
    m_apoItems.DeleteAll();
}
