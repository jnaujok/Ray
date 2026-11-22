#include "Globals.h"

#include "PixelSampler.h"
#include "CenterSampler.h"
#include "CornerSampler.h"
#include "BoxSampler.h"
#include "RandomSampler.h"
#include "StratifiedSampler.h"
#include "AdaptiveSampler.h"

#include "SceneJson.h"

#include <string.h>

namespace
{
    CPixelSampler *CreateCenter( void ) { return new CCenterSampler; }
    CPixelSampler *CreateCorner( void ) { return new CCornerSampler; }
    CPixelSampler *CreateBox( void )    { return new CBoxSampler; }
    CPixelSampler *CreateRandom( void ) { return new CRandomSampler; }
    CPixelSampler *CreateStratified( void ) { return new CStratifiedSampler; }
    CPixelSampler *CreateAdaptive( void ) { return new CAdaptiveSampler; }

        // Every sampler that can be chosen by name. The first is the default.
    const CPixelSampler::SType s_aoTypes[] =
    {
        { "none",    "one ray through the center of each pixel",                           CreateCenter },
        { "corners", "five rays per pixel, at its corners and center (about two per pixel)", CreateCorner },
        { "box",     "four rays per pixel, halfway from its center to each corner",            CreateBox },
        { "random",  "a center ray worth half the pixel, and N random rays sharing the other "
                     "half (settings: samples, N, default 4; seed, default 0)",                  CreateRandom },
        { "stratified", "an n x n grid of equally weighted rays, each at a random point in its "
                        "cell; best for a real lens (settings: grid, n, default 4; seed)",      CreateStratified },
        { "adaptive", "more rays only where a pixel's rays disagree: at least min (default 9), "
                      "at most max (default 64), until the brightness is within threshold "
                      "(default 0.01; seed)",                                                    CreateAdaptive },
    };
}

const CPixelSampler::SType *CPixelSampler::GetTypes( int &nCount )
{
    nCount = (int)( sizeof( s_aoTypes ) / sizeof( s_aoTypes[ 0 ] ) );
    return s_aoTypes;
}

CPixelSampler *CPixelSampler::Create( const char *strName )
{
    int             nCount;
    const SType    *aoTypes = GetTypes( nCount );

    if ( strName == NULL )
        return NULL;

    for ( int nIndex = 0; nIndex < nCount; nIndex++ )
    {
        if ( strcmp( aoTypes[ nIndex ].strName, strName ) == 0 )
            return aoTypes[ nIndex ].pfnCreate();
    }
    return NULL;
}

BOOL CPixelSampler::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { NULL };

    return oNode.isObject() ? RayJson::OnlyFields( oNode, astrFields ) : TRUE;
}
