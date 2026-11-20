#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ColorModel.h"
#include "ClipColorModel.h"
#include "GammaColorModel.h"
#include "SrgbColorModel.h"
#include "ReinhardColorModel.h"
#include "HdrColorModel.h"
#include "Color.h"

#include "SceneJson.h"

#include <string.h>

namespace
{
    CColorModel *CreateClip( void )     { return new CClipColorModel; }
    CColorModel *CreateGamma( void )    { return new CGammaColorModel; }
    CColorModel *CreateSrgb( void )     { return new CSrgbColorModel; }
    CColorModel *CreateReinhard( void ) { return new CReinhardColorModel; }
    CColorModel *CreateHdr( void )      { return new CHdrColorModel; }

        // Every color model that can be chosen by name. The first is the default.
    const CColorModel::SType s_aoTypes[] =
    {
        { "clip",     "clip each channel at 1 and write it unencoded, as in 1997",   CreateClip },
        { "gamma",    "clip, then gamma encode (setting: gamma, default 2.2)",       CreateGamma },
        { "srgb",     "clip, then encode with the sRGB curve",                       CreateSrgb },
        { "reinhard", "tone map highlights instead of clipping them, then sRGB "
                      "(settings: exposure, default 1; white, default none)",        CreateReinhard },
        { "hdr",      "high dynamic range: scale by exposure (default 1) and keep "
                      "every brightness for --hdr files; 8-bit files clip, then sRGB", CreateHdr },
    };
}

const CColorModel::SType *CColorModel::GetTypes( int &nCount )
{
    nCount = (int)( sizeof( s_aoTypes ) / sizeof( s_aoTypes[ 0 ] ) );
    return s_aoTypes;
}

CColorModel *CColorModel::Create( const char *strName )
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

FLT CColorModel::Clamp( FLT nValue )
{
    if ( ! ( nValue > 0.0 ) )
        return 0.0;
    if ( nValue > 1.0 )
        return 1.0;
    return nValue;
}

BYTE CColorModel::ToByte( FLT nValue )
{
    return (BYTE)( ( Clamp( nValue ) * 255.0 ) + 0.5 );
}

CColor CColorModel::ToLinear( const CColor &oColor ) const
{
    return oColor;
}

BOOL CColorModel::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { NULL };

    return oNode.isObject() ? RayJson::OnlyFields( oNode, astrFields ) : TRUE;
}
