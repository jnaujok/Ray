#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Texture.h"
#include "CheckerTexture.h"
#include "ImageTexture.h"

#include <string.h>

namespace
{
    CTexture *CreateChecker( void ) { return new CCheckerTexture; }
    CTexture *CreateImage( void )   { return new CImageTexture; }

        // Every texture that can be named in a scene.
    const CTexture::SType s_aoTypes[] =
    {
        { "checker", "a checkerboard of two colors, on the surface or solid", CreateChecker },
        { "image",   "an image file laid over the surface",                   CreateImage },
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
