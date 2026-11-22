#include "Globals.h"

#include "CombineTexture.h"
#include "SceneJson.h"

#include <math.h>
#include <string>

CCombineTexture::CCombineTexture( void ) :
m_bAdd( FALSE ),
m_oColor( 1.0, 1.0, 1.0 )
{
}

CColor CCombineTexture::ColorAt( const STexturePoint &oAt ) const
{
    CColor oResult( m_bAdd ? 0.0 : 1.0, m_bAdd ? 0.0 : 1.0, m_bAdd ? 0.0 : 1.0 );

    for ( size_t nIndex = 0; nIndex < m_apTextures.size(); nIndex++ )
    {
        CColor oPart = m_apTextures[ nIndex ]->ColorAt( oAt );

        if ( m_bAdd )
        {
            FLT nWeight = m_anWeights[ nIndex ];

            oResult = CColor( oResult.GetR() + nWeight * oPart.GetR(), oResult.GetG() + nWeight * oPart.GetG(),
                              oResult.GetB() + nWeight * oPart.GetB() );
        }
        else
            oResult = CColor( oResult.GetR() * oPart.GetR(), oResult.GetG() * oPart.GetG(), oResult.GetB() * oPart.GetB() );
    }
    CColor oTint( m_oColor );

    return CColor( oResult.GetR() * oTint.GetR(), oResult.GetG() * oTint.GetG(), oResult.GetB() * oTint.GetB() );
}

void CCombineTexture::ToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "mode", m_bAdd ? "add" : "multiply" );

    IASLib::CArrayNode *pTextures = oNode.putArray( "textures" );

    for ( size_t nIndex = 0; nIndex < m_apTextures.size(); nIndex++ )
    {
            // Each child is written as a texture is anywhere else (a preset
            // stays a preset), then moved into the array.
        IASLib::CObjectNode         oHolder;

        m_apTextures[ nIndex ]->Write( oHolder, "t" );

        const IASLib::CObjectNode  *pChild = dynamic_cast<const IASLib::CObjectNode *>( oHolder.get( "t" ) );
        IASLib::CObjectNode        *pOut = pTextures->addObject();

        for ( int nField = 0; pChild && nField < pChild->size(); nField++ )
            pOut->set( pChild->fieldNameString( nField ), pChild->fieldValue( nField )->clone() );
    }
    if ( m_bAdd )
    {
        BOOL bAllOne = TRUE;

        for ( size_t nIndex = 0; nIndex < m_anWeights.size(); nIndex++ )
            bAllOne = bAllOne && ( m_anWeights[ nIndex ] == 1.0 );
        if ( ! bAllOne )
        {
            IASLib::CArrayNode *pWeights = oNode.putArray( "weights" );

            for ( size_t nIndex = 0; nIndex < m_anWeights.size(); nIndex++ )
                pWeights->add( RayJson::Rounded( m_anWeights[ nIndex ] ) );
        }
    }
    CColor oTint( m_oColor );

    if ( ! ( oTint.GetR() == 1.0 && oTint.GetG() == 1.0 && oTint.GetB() == 1.0 ) )
        RayJson::PutColor( oNode, "color", oTint );
}

BOOL CCombineTexture::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "type", "mode", "textures", "weights", "color", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) )
        return FALSE;

    std::string strMode = "multiply";

    if ( ! RayJson::GetString( oNode, "mode", strMode, FALSE ) )
        return FALSE;
    if ( strMode != "multiply" && strMode != "add" )
        return RayJson::Fail( "mode must be \"multiply\" or \"add\"" );
    m_bAdd = ( strMode == "add" ) ? TRUE : FALSE;

    const IASLib::CJsonNode *pTextures = oNode.get( "textures" );

    if ( ! pTextures || ! pTextures->isArray() || pTextures->size() < 2 )
        return RayJson::Fail( "textures must be an array of two or more textures" );
    m_apTextures.clear();
    for ( int nIndex = 0; nIndex < (int)pTextures->size(); nIndex++ )
    {
        RayJson::CWhere oWhere( "textures", nIndex );
        CTexture       *pNew = CTexture::Read( *pTextures->get( nIndex ) );

        if ( ! pNew )
            return FALSE;
        m_apTextures.push_back( std::shared_ptr<const CTexture>( pNew ) );
    }

    m_anWeights.assign( m_apTextures.size(), 1.0 );

    const IASLib::CJsonNode *pWeights = oNode.get( "weights" );

    if ( pWeights )
    {
        RayJson::CWhere oWhere( "weights" );

        if ( ! m_bAdd )
            return RayJson::Fail( "weights are for \"mode\": \"add\"" );
        if ( ! pWeights->isArray() || (int)pWeights->size() != (int)m_apTextures.size() )
            return RayJson::Fail( "must be an array with one number a texture" );
        for ( int nIndex = 0; nIndex < (int)pWeights->size(); nIndex++ )
        {
            const IASLib::CJsonNode *pWeight = pWeights->get( nIndex );

            if ( ! pWeight->isNumber() || ! isfinite( pWeight->doubleValue() ) )
                return RayJson::Fail( "each weight must be a number" );
            m_anWeights[ (size_t)nIndex ] = (FLT)pWeight->doubleValue();
        }
    }

    m_oColor = CColor( 1.0, 1.0, 1.0 );
    if ( oNode.get( "color" ) )
    {
        CVector oColor;

        RayJson::CWhere oWhere( "color" );
        if ( ! RayJson::ToVector( *oNode.get( "color" ), oColor ) )
            return FALSE;
        m_oColor = CColor( oColor.GetX(), oColor.GetY(), oColor.GetZ() );
    }
    return TRUE;
}
