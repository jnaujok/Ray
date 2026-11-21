#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Texture.h"
#include "CheckerTexture.h"
#include "ImageTexture.h"
#include "MarbleTexture.h"
#include "NoiseTexture.h"
#include "WoodTexture.h"
#include "LayersTexture.h"
#include "FaceColorTexture.h"
#include "CellsTexture.h"
#include "CausticsTexture.h"
#include "Presets.h"
#include "SceneJson.h"

#include <string.h>

namespace
{
    CTexture *CreateChecker( void ) { return new CCheckerTexture; }
    CTexture *CreateImage( void )   { return new CImageTexture; }
    CTexture *CreateMarble( void )  { return new CMarbleTexture; }
    CTexture *CreateWood( void )    { return new CWoodTexture; }
    CTexture *CreateNoise( void )   { return new CNoiseTexture; }
    CTexture *CreateLayers( void )  { return new CLayersTexture; }
    CTexture *CreateFaceColors( void ) { return new CFaceColorTexture; }
    CTexture *CreateCells( void )   { return new CCellsTexture; }
    CTexture *CreateCaustics( void ) { return new CCausticsTexture; }

        // Every texture that can be named in a scene.
    const CTexture::SType s_aoTypes[] =
    {
        { "checker", "a checkerboard of two colors, on the surface or solid", CreateChecker },
        { "image",   "an image file laid over the surface",                   CreateImage },
        { "marble",  "solid marble: bands bent into veins by turbulence",     CreateMarble },
        { "wood",    "solid wood: rings about an axis, wobbled by noise",     CreateWood },
        { "noise",   "solid noise or turbulence through a ramp of colors",    CreateNoise },
        { "layers",  "the rounded layer lines of a 3D-printed part",          CreateLayers },
        { "faceColors", "the colors a colored model file gives its faces",    CreateFaceColors },
        { "cells",   "solid cellular noise: flecks, spots or cell edges",     CreateCells },
        { "caustics", "the web of light that waves focus under water",        CreateCaustics },
    };
}

const CTexture::SType *CTexture::GetTypes( int &nCount )
{
    nCount = (int)( sizeof( s_aoTypes ) / sizeof( s_aoTypes[ 0 ] ) );
    return s_aoTypes;
}

CTexture *CTexture::Create( const char *strName )
{
    int             nCount;
    const SType    *aoTypes = GetTypes( nCount );

    for ( int nIndex = 0; strName && nIndex < nCount; nIndex++ )
    {
        if ( strcmp( aoTypes[ nIndex ].strName, strName ) == 0 )
            return aoTypes[ nIndex ].pfnCreate();
    }
    return NULL;
}

CTexture *CTexture::Read( const IASLib::CJsonNode &oNode )
{
    std::unique_ptr<IASLib::CJsonNode>  pExpanded;
    const IASLib::CJsonNode            *pUse = &oNode;
    std::string                         strType;

    if ( ! oNode.isObject() )
    {
        RayJson::Fail( "must be an object with a \"type\" or a \"preset\"" );
        return NULL;
    }
    if ( oNode.get( "preset" ) )
    {
        pExpanded.reset( RayPresets::ExpandTexture( oNode ) );
        if ( ! pExpanded )
            return NULL;
        pUse = pExpanded.get();
    }
    if ( ! RayJson::GetString( *pUse, "type", strType, TRUE ) )
        return NULL;

    std::unique_ptr<CTexture> pNew( Create( strType.c_str() ) );

    if ( ! pNew )
    {
        RayJson::Fail( "unknown texture type \"" + strType + "\"" );
        return NULL;
    }
    if ( ! pNew->FromJson( *pUse ) )
        return NULL;
    if ( pExpanded )
        pNew->m_pPreset.reset( oNode.clone() );
    return pNew.release();
}

namespace
{
        // A texture's full JSON (its type and settings), as text.
    std::string FullText( const CTexture &oTexture )
    {
        IASLib::CObjectNode oNode;

        oNode.put( "type", oTexture.GetName() );
        oTexture.ToJson( oNode );
        return RayJson::Write( oNode );
    }
}

void CTexture::Write( IASLib::CObjectNode &oParent, const char *strKey ) const
{
    IASLib::CObjectNode *pNode = oParent.putObject( strKey );

    if ( m_pPreset )
    {
            // Written as the preset only while that still gives exactly this
            // texture (it hasn't been changed since).
        std::unique_ptr<CTexture> pFresh( Read( *m_pPreset ) );

        if ( pFresh && FullText( *pFresh ) == FullText( *this ) )
        {
            const IASLib::CObjectNode *pPreset = dynamic_cast<const IASLib::CObjectNode *>( m_pPreset.get() );

            for ( int nIndex = 0; pPreset && nIndex < pPreset->size(); nIndex++ )
                pNode->set( pPreset->fieldNameString( nIndex ), pPreset->fieldValue( nIndex )->clone() );
            return;
        }
    }
    pNode->put( "type", GetName() );
    ToJson( *pNode );
}

std::string CTexture::GetPreset( void ) const
{
    const IASLib::CJsonNode *pName = m_pPreset ? m_pPreset->get( "preset" ) : NULL;

    return pName ? pName->textString() : std::string();
}
