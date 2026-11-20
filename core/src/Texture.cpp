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
