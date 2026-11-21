/*************************************************************************
*   CScalarMap, CColorMap: settings that are constants or textures.      *
*   See Maps.h.                                                          *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Maps.h"
#include "Objects.h"
#include "SceneJson.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace
{
    const char *const s_astrChannels[] = { "average", "luminance", "red", "green", "blue" };

    BOOL IsTextureObject( const IASLib::CJsonNode &oValue )
    {
        return ( oValue.get( "type" ) != NULL || oValue.get( "preset" ) != NULL ) ? TRUE : FALSE;
    }
}

CMapPoint::CMapPoint( const CVector &oPoint, CObjects *pObject ) :
m_oPoint( oPoint ),
m_pObject( pObject ),
m_bDone( FALSE )
{
    m_oAt.nU = m_oAt.nV = 0.0;
    m_oAt.bHasUV = FALSE;
    m_oAt.bHasColor = FALSE;
}

CMapPoint CMapPoint::InSpace( const CVector &oPoint, const CObjects *pObject )
{
    CMapPoint oResult( oPoint, NULL );

    oResult.m_oAt.oPoint = pObject ? pObject->TextureSpace( oPoint ) : oPoint;
    oResult.m_bDone = TRUE;
    return oResult;
}

const STexturePoint &CMapPoint::Get( void )
{
    if ( ! m_bDone )
    {
        if ( m_pObject )
            m_pObject->TextureAt( m_oPoint, m_oAt );
        else
            m_oAt.oPoint = m_oPoint;
        m_bDone = TRUE;
    }
    return m_oAt;
}

CMapBase::CMapBase( void ) :
m_nScale( 1.0 ),
m_nOffset( 0.0 ),
m_nChannel( CHANNEL_AVERAGE ),
m_bWrapped( FALSE )
{
}

CColor CMapBase::TextureColor( const STexturePoint &oAt ) const
{
    return m_pTexture->ColorAt( oAt );
}

BOOL CMapBase::ReadMap( const IASLib::CJsonNode &oValue, BOOL bScalar )
{
    const IASLib::CJsonNode *pTexture = &oValue;
    FLT                      nScale = 1.0;
    FLT                      nOffset = 0.0;
    int                      nChannel = CHANNEL_AVERAGE;
    BOOL                     bWrapped = FALSE;

    if ( ! IsTextureObject( oValue ) )
    {
        static const char *const astrScalar[] = { "texture", "scale", "offset", "channel", NULL };
        static const char *const astrColor[] = { "texture", "scale", "offset", NULL };

        if ( ! RayJson::OnlyFields( oValue, bScalar ? astrScalar : astrColor ) ||
             ! RayJson::GetNumber( oValue, "scale", nScale, FALSE ) ||
             ! RayJson::GetNumber( oValue, "offset", nOffset, FALSE ) )
            return FALSE;
        if ( ! std::isfinite( (double)nScale ) || ! std::isfinite( (double)nOffset ) )
            return RayJson::Fail( "scale and offset must be numbers" );
        if ( bScalar )
        {
            std::string strChannel;

            if ( ! RayJson::GetString( oValue, "channel", strChannel, FALSE ) )
                return FALSE;
            if ( ! strChannel.empty() )
            {
                nChannel = -1;
                for ( int nIndex = 0; nIndex < 5; nIndex++ )
                {
                    if ( strChannel == s_astrChannels[ nIndex ] )
                        nChannel = nIndex;
                }
                if ( nChannel < 0 )
                {
                    RayJson::CWhere oWhere( "channel" );

                    return RayJson::Fail( "must be \"average\", \"luminance\", \"red\", \"green\" or \"blue\"" );
                }
            }
        }
        pTexture = oValue.get( "texture" );
        if ( pTexture == NULL )
            return RayJson::Fail( "must be a texture (with a \"type\" or \"preset\"), or an object with a \"texture\"" );
        bWrapped = TRUE;
    }

    CTexture       *pNew;

    if ( bWrapped )
    {
        RayJson::CWhere oWhere( "texture" );

        pNew = CTexture::Read( *pTexture );
    }
    else
        pNew = CTexture::Read( *pTexture );
    if ( pNew == NULL )
        return FALSE;
    m_pTexture.reset( pNew );
    m_nScale = nScale;
    m_nOffset = nOffset;
    m_nChannel = nChannel;
    m_bWrapped = bWrapped;
    return TRUE;
}

void CMapBase::WriteMap( IASLib::CObjectNode &oParent, const char *strKey, BOOL bScalar ) const
{
    if ( ! m_bWrapped && m_nScale == 1.0 && m_nOffset == 0.0 && m_nChannel == CHANNEL_AVERAGE )
    {
        m_pTexture->Write( oParent, strKey );
        return;
    }

    IASLib::CObjectNode *pMap = oParent.putObject( strKey );

    m_pTexture->Write( *pMap, "texture" );
    if ( m_nScale != 1.0 )
        RayJson::PutNumber( *pMap, "scale", m_nScale );
    if ( m_nOffset != 0.0 )
        RayJson::PutNumber( *pMap, "offset", m_nOffset );
    if ( bScalar && m_nChannel != CHANNEL_AVERAGE )
        pMap->put( "channel", s_astrChannels[ m_nChannel ] );
}

CScalarMap::CScalarMap( FLT nValue ) :
m_nValue( nValue ),
m_nMin( -(std::numeric_limits<FLT>::max)() ),
m_nMax( (std::numeric_limits<FLT>::max)() )
{
}

CScalarMap &CScalarMap::operator =( FLT nValue )
{
    m_nValue = nValue;
    m_pTexture.reset();
    return *this;
}

FLT CScalarMap::At( const STexturePoint &oAt ) const
{
    if ( ! m_pTexture )
        return m_nValue;

    CColor  oColor( TextureColor( oAt ) );
    FLT     nRead;

    switch ( m_nChannel )
    {
        case CHANNEL_LUMINANCE:
            nRead = ( 0.2126 * oColor.GetR() ) + ( 0.7152 * oColor.GetG() ) + ( 0.0722 * oColor.GetB() );
            break;
        case CHANNEL_RED:
            nRead = oColor.GetR();
            break;
        case CHANNEL_GREEN:
            nRead = oColor.GetG();
            break;
        case CHANNEL_BLUE:
            nRead = oColor.GetB();
            break;
        default:
            nRead = ( oColor.GetR() + oColor.GetG() + oColor.GetB() ) / 3.0;
            break;
    }

    FLT nValue = m_nOffset + ( m_nScale * nRead );

    return (std::min)( m_nMax, (std::max)( m_nMin, nValue ) );
}

BOOL CScalarMap::Read( const IASLib::CJsonNode &oNode, const char *strKey, BOOL bRequired )
{
    const IASLib::CJsonNode *pValue = RayJson::Field( oNode, strKey, bRequired );

    if ( pValue == NULL )
        return ( bRequired || ! oNode.isObject() ) ? FALSE : TRUE;
    if ( pValue->isObject() )
    {
        RayJson::CWhere oWhere( strKey );

        return ReadMap( *pValue, TRUE );
    }
    if ( ! RayJson::GetNumber( oNode, strKey, m_nValue, bRequired ) )
        return FALSE;
    m_pTexture.reset();
    return TRUE;
}

void CScalarMap::Write( IASLib::CObjectNode &oNode, const char *strKey ) const
{
    if ( m_pTexture )
        WriteMap( oNode, strKey, TRUE );
    else
        RayJson::PutNumber( oNode, strKey, m_nValue );
}

CColorMap &CColorMap::operator =( const CColor &oValue )
{
    m_oValue = oValue;
    m_pTexture.reset();
    return *this;
}

CColor CColorMap::At( const STexturePoint &oAt ) const
{
    if ( ! m_pTexture )
        return m_oValue;

    CColor oColor( TextureColor( oAt ) );

    if ( m_nScale == 1.0 && m_nOffset == 0.0 )
        return oColor;
    return CColor( m_nOffset + ( m_nScale * oColor.GetR() ),
                   m_nOffset + ( m_nScale * oColor.GetG() ),
                   m_nOffset + ( m_nScale * oColor.GetB() ) );
}

BOOL CColorMap::Read( const IASLib::CJsonNode &oNode, const char *strKey, BOOL bRequired )
{
    const IASLib::CJsonNode *pValue = RayJson::Field( oNode, strKey, bRequired );

    if ( pValue == NULL )
        return ( bRequired || ! oNode.isObject() ) ? FALSE : TRUE;
    if ( pValue->isObject() )
    {
        RayJson::CWhere oWhere( strKey );

        return ReadMap( *pValue, FALSE );
    }
    if ( ! RayJson::GetColor( oNode, strKey, m_oValue, bRequired ) )
        return FALSE;
    m_pTexture.reset();
    return TRUE;
}

void CColorMap::Write( IASLib::CObjectNode &oNode, const char *strKey ) const
{
    if ( m_pTexture )
        WriteMap( oNode, strKey, FALSE );
    else
        RayJson::PutColor( oNode, strKey, m_oValue );
}
