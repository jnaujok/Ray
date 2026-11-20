// Checks that rays, colors and scenes cross the wire exactly, that JSON
// scenes are read and checked properly, and that scene files survive a load
// and save unchanged.
//
//   format_test                   the wire and JSON checks
//   format_test <scene.ray> <dir> load, save, load, save; the two saves must match
#include "Globals.h"
#include "RayArray.h"
#include "ColorArray.h"
#include "WireProtocol.h"
#include "Mesh.h"
#include "SpecularMaterial.h"
#include "TransparentMaterial.h"
#include "Scene.h"
#include "ReinhardColorModel.h"


#include <math.h>
#include <stdio.h>
#include <string.h>

#include <filesystem>
#include <string>

static int g_nFailures = 0;

static void Check( bool bCondition, const char *strWhat )
{
    if ( ! bCondition )
    {
        fprintf( stderr, "FAILED: %s\n", strWhat );
        g_nFailures++;
    }
}

    // Rays, colors and a scene must cross the wire exactly.
static void WireChecks( void )
{
    RayWire::CEncoder   oEncoder( RAY_WIRE_MASTER_ID );
    RayWire::CDecoder   oDecoder( RAY_WIRE_ENGINE_ID, RAY_WIRE_MASTER_ID );
    const double        adValues[] = { 0.0, 0.1, -3.141592653589793, 1e-300, 6.02214076e23, 2.0 / 3.0, 1e100, -7.25 };
    const int           nValues = (int)( sizeof( adValues ) / sizeof( adValues[ 0 ] ) );
    CRayArray           oRays;
    CRay                oRay;

    Check( RayWire::DecodeHello( RayWire::EncodeHello() ), "the hello reads back" );

        // Several batches, so stream mode's caches and deltas are used.
    for ( int nBatch = 0; nBatch < 3; nBatch++ )
    {
        oRays.RemoveAll();
        for ( int nIndex = 0; nIndex < 40; nIndex++ )
        {
            double d = adValues[ ( nIndex + nBatch ) % nValues ] + nIndex * 1e-9;

            oRay.Set( d, -d, 0.5, d * 3.0, 1.0 / ( 1.0 + nIndex ), -0.25, 1e100, 1.0, nIndex, 1000 + nBatch );
            oRays.Add( oRay );
        }

        CRayArray oRead;
        Check( oDecoder.Rays( oEncoder.Rays( oRays ), oRead ), "a batch of rays decodes" );
        Check( oRead.GetSize() == oRays.GetSize(), "every ray arrives" );
        for ( int nIndex = 0; nIndex < oRead.GetSize() && nIndex < oRays.GetSize(); nIndex++ )
        {
            CRay &oA = oRays[ nIndex ];
            CRay &oB = oRead[ nIndex ];

            Check( oA.GetX() == oB.GetX() && oA.GetY() == oB.GetY() && oA.GetZ() == oB.GetZ() &&
                   oA.GetI() == oB.GetI() && oA.GetJ() == oB.GetJ() && oA.GetK() == oB.GetK() &&
                   oA.GetT() == oB.GetT() && oA.GetInfluence() == oB.GetInfluence() &&
                   oA.GetPixelX() == oB.GetPixelX() && oA.GetPixelY() == oB.GetPixelY(), "a ray arrives exactly" );
        }
    }

    std::string strReadable = RayWire::ToReadable( MESSAGE_RAYS, oEncoder.Rays( oRays ) );
    Check( strReadable.find( "\"influence\"" ) != std::string::npos && strReadable.find( "\"py\": 1002" ) != std::string::npos,
           "a batch of rays can be read as JSON" );

    CRayArray oEmpty, oRead;
    Check( oDecoder.Rays( oEncoder.Rays( oEmpty ), oRead ) && oRead.GetSize() == 0, "an empty batch (stop) arrives" );

    RayWire::CEncoder   oColorOut( RAY_WIRE_ENGINE_ID );
    RayWire::CDecoder   oColorIn( RAY_WIRE_MASTER_ID, RAY_WIRE_ENGINE_ID );
    CColorArray         oColors, oColorsRead;

    for ( int nIndex = 0; nIndex < nValues; nIndex++ )
        oColors.Add( CColor( adValues[ nIndex ], 1.0 - adValues[ nIndex ], 0.3 ), nIndex, 2 * nIndex );
    Check( oColorIn.Colors( oColorOut.Colors( oColors ), oColorsRead ), "a batch of colors decodes" );
    Check( oColorsRead.GetSize() == oColors.GetSize(), "every color arrives" );
    for ( int nIndex = 0; nIndex < oColorsRead.GetSize() && nIndex < oColors.GetSize(); nIndex++ )
        Check( oColorsRead[ nIndex ] == oColors[ nIndex ] && oColorsRead.GetX( nIndex ) == oColors.GetX( nIndex ) &&
               oColorsRead.GetY( nIndex ) == oColors.GetY( nIndex ), "a color arrives exactly" );

    Check( ! oColorIn.Colors( std::string( "\x50garbage", 8 ), oColorsRead ), "a damaged packet is refused" );
}

static std::string ReadFile( const std::string &strFile )
{
    std::string strData;
    FILE       *pFile = fopen( strFile.c_str(), "rb" );
    char        achBuffer[ 8192 ];
    size_t      nRead;

    if ( pFile == NULL )
        return strData;
    while ( ( nRead = fread( achBuffer, 1, sizeof( achBuffer ), pFile ) ) > 0 )
        strData.append( achBuffer, nRead );
    fclose( pFile );
    return strData;
}

    // A scene with only what has no default: a camera.
static const char *s_strMinimal =
    "{\n"
    "  \"format\": \"Ray scene\", \"version\": 3,\n"
    "  \"camera\": { \"position\": [0, 0, -10], \"lookAt\": [0, 0, 0] },\n"
    "  \"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 } ],\n"
    "  \"lights\": [ { \"type\": \"point\", \"position\": [5, 5, -5], \"color\": [1, 1, 1] } ]\n"
    "}\n";

    // s_strMinimal with strFrom replaced by strTo.
static std::string Minimal( const char *strFrom, const char *strTo )
{
    std::string strText = s_strMinimal;
    size_t      nAt = strText.find( strFrom );

    if ( nAt != std::string::npos )
        strText.replace( nAt, strlen( strFrom ), strTo );
    return strText;
}

    // Loading strText must fail with a message containing strExpected, and
    // leave the scene as it was.
static void ExpectError( const std::string &strText, DWORD nExpected, const char *strExpected )
{
    CScene  oScene;

    Check( oScene.FromJson( s_strMinimal ) == 0, "the minimal scene loads" );

    std::string strBefore = oScene.ToJson();
    DWORD       nResult = oScene.FromJson( strText );

    if ( nResult != nExpected || strstr( oScene.GetLoadError(), strExpected ) == NULL )
    {
        fprintf( stderr, "FAILED: expected error \"%s\", got %u \"%s\"\n", strExpected, (unsigned)nResult, oScene.GetLoadError() );
        g_nFailures++;
    }
    Check( oScene.ToJson() == strBefore, "a failed load leaves the scene as it was" );
}

static void JsonChecks( void )
{
    CScene  oScene;

    Check( oScene.FromJson( s_strMinimal ) == 0, "a minimal JSON scene loads" );
    Check( oScene.GetSettings().GetImageWidth() == 640, "settings that are left out keep their defaults" );
    Check( strcmp( oScene.GetCamera().GetSampler().GetName(), "none" ) == 0, "the default sampler" );
    Check( strcmp( oScene.GetCamera().GetColorModel().GetName(), "clip" ) == 0, "the default color model" );

        // The sampler and color model, with settings, are saved and read back.
    std::string strModels = Minimal( "\"lookAt\": [0, 0, 0]",
                                     "\"lookAt\": [0, 0, 0], \"sampler\": \"corners\", "
                                     "\"colorModel\": { \"type\": \"reinhard\", \"exposure\": 2, \"white\": 4 }" );
    Check( oScene.FromJson( strModels ) == 0, "a scene with a sampler and color model loads" );
    Check( strcmp( oScene.GetCamera().GetSampler().GetName(), "corners" ) == 0, "the sampler is read" );

    CReinhardColorModel *pModel = dynamic_cast<CReinhardColorModel *>( &oScene.GetCamera().GetColorModel() );
    Check( pModel && pModel->GetExposure() == 2.0 && pModel->GetWhite() == 4.0, "the color model's settings are read" );

    std::string strSaved = oScene.ToJson();
    CScene      oReread;
    Check( oReread.FromJson( strSaved ) == 0 && oReread.ToJson() == strSaved, "a saved JSON scene reads back the same" );
    Check( strSaved.find( "\"sampler\": \"corners\"" ) != std::string::npos, "a sampler without settings is saved by name" );
    Check( strSaved.find( "\"exposure\": 2.0" ) != std::string::npos, "a color model's settings are saved" );

        // A sampler with settings.
    CScene oRandom;
    Check( oRandom.FromJson( Minimal( "\"lookAt\": [0, 0, 0]",
                                      "\"lookAt\": [0, 0, 0], \"sampler\": { \"type\": \"random\", \"samples\": 9, \"seed\": 3 }" ) ) == 0,
           "a random sampler with settings loads" );
    Check( oRandom.ToJson().find( "\"samples\": 9" ) != std::string::npos && oRandom.ToJson().find( "\"seed\": 3" ) != std::string::npos,
           "the random sampler's settings are saved" );
    ExpectError( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"sampler\": { \"type\": \"random\", \"samples\": 0 }" ),
                 BAD_FILE, "camera.sampler: samples must be a whole number from 1 to 1024" );

        // Mistakes are reported where they are.
    ExpectError( "{ \"format\": ", BAD_FILE, "not valid JSON" );
    ExpectError( Minimal( "\"Ray scene\"", "\"Something else\"" ), BAD_FILE, "\"format\" must be \"Ray scene\"" );
    ExpectError( Minimal( "\"version\": 3", "\"version\": 4" ), WRONG_VERSION, "newer version" );
    ExpectError( Minimal( "\"radius\": 1", "\"radus\": 1" ), BAD_FILE, "objects[0]: unknown field \"radus\"" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": \"big\"" ), BAD_FILE, "objects[0]: radius must be a number" );
    ExpectError( Minimal( "\"center\": [0, 0, 0]", "\"center\": [0, 0]" ), BAD_FILE, "objects[0].center: must be an array of three numbers" );
    ExpectError( Minimal( "\"type\": \"sphere\"", "\"type\": \"cube\"" ), BAD_FILE, "objects[0]: unknown object type \"cube\"" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"sphere\"" ), BAD_FILE, "lights[0]: unknown light type \"sphere\"" );
    ExpectError( Minimal( "\"position\": [0, 0, -10], ", "" ), BAD_FILE, "camera: needs \"position\"" );
    ExpectError( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"sampler\": \"jittered\"" ), BAD_FILE,
                 "camera.sampler: unknown sampler \"jittered\"" );
    ExpectError( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"colorModel\": { \"type\": \"gamma\", \"gamma\": -1 }" ),
                 BAD_FILE, "camera.colorModel: gamma must be a positive number" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"chrome\" } ]" ), BAD_FILE,
                 "objects[0].materials[0]: unknown material type \"chrome\"" );
}

    // A ray from (0, 0.3, nZ) along z, toward the default triangle at z = 0.
static bool HitsFrom( CObjects &oObject, double dZ )
{
    CRay    oRay;
    CVector oHit;

    oRay.Set( 0.0, 0.3, dZ, 0.0, 0.0, ( dZ < 0 ) ? 1.0 : -1.0, 1e100, 1.0, 0, 0 );
    return oObject.Intersect( oRay, oHit ) ? true : false;
}

    // A scene's first object, loaded from JSON objects text.
static CObjects *FirstObject( CScene &oScene, const char *strObjects )
{
    std::string strText = Minimal( "\"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 } ]",
                                   ( std::string( "\"objects\": [ " ) + strObjects + " ]" ).c_str() );

    if ( oScene.FromJson( strText ) != 0 )
    {
        fprintf( stderr, "FAILED: %s\n", oScene.GetLoadError() );
        g_nFailures++;
        return NULL;
    }
    return oScene.GetObjectAt( 0 );
}

static void TriangleChecks( void )
{
    CScene      oScene;
    CObjects   *pObject;

        // The corners (0,1,0), (1,0,0), (-1,0,0) face -z: their normal is
        // (V2 - V1) x (V3 - V2) = (0, 0, -2).
    pObject = FirstObject( oScene, "{ \"type\": \"triangle\", \"corners\": [[0, 1, 0], [1, 0, 0], [-1, 0, 0]] }" );
    Check( pObject && HitsFrom( *pObject, -5.0 ), "a single-sided triangle is hit from the front" );
    Check( pObject && ! HitsFrom( *pObject, 5.0 ), "a single-sided triangle is not hit from behind" );

    pObject = FirstObject( oScene, "{ \"type\": \"triangle\", \"corners\": [[0, 1, 0], [1, 0, 0], [-1, 0, 0]], \"doubleSided\": true }" );
    Check( pObject && HitsFrom( *pObject, -5.0 ) && HitsFrom( *pObject, 5.0 ), "a double-sided triangle is hit from both sides" );

    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"triangle\", \"corners\": [[0, 0, 0], [1, 1, 1], [2, 2, 2]]" ),
                 BAD_FILE, "objects[0]: the corners are in a line" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"mesh\", \"vertices\": [[0, 0, 0], [1, 0, 0], [0, 1, 0]], \"faces\": [[0, 1, 3]]" ),
                 BAD_FILE, "objects[0].faces[0]: vertex indices must be whole numbers from 0 to 2" );

        // Normals given in the scene: a triangle shades with them, smoothly,
        // and saves them; a mesh uses them instead of summing its faces'.
    {
        CScene      oGiven;
        CTriangle  *pTriangle = dynamic_cast<CTriangle *>( FirstObject( oGiven,
            "{ \"type\": \"triangle\", \"corners\": [[0, 1, 0], [1, 0, 0], [-1, 0, 0]], "
            "\"normals\": [[0, 2, -2], [1, 0, -1], [-1, 0, -1]] }" ) );
        CVector     oCorner( 0.0, 1.0, 0.0 );
        CVector     oExpected( 0.0, 1.0, -1.0 );

        oExpected.Normalize();
        Check( pTriangle != NULL, "a triangle with normals loads" );
        if ( pTriangle )
        {
            CVector oDifference = pTriangle->NormalAt( oCorner ) - oExpected;
            Check( oDifference.Magnitude() < 1e-12, "a triangle shades with its given normals, made unit vectors" );
        }
        Check( oGiven.ToJson().find( "\"normals\"" ) != std::string::npos && oGiven.ToJson().find( "\"smooth\": true" ) != std::string::npos,
               "given normals make a triangle smooth and are saved" );

        CScene  oMeshGiven;
        CMesh  *pMesh = dynamic_cast<CMesh *>( FirstObject( oMeshGiven,
            "{ \"type\": \"mesh\", \"vertices\": [[-1, 0, 0], [1, 0, 0], [0, 1, 0]], \"faces\": [[0, 2, 1]], "
            "\"normals\": [[0, 0, -1], [0, 0, -1], [0, 1, 0]] }" ) );
        Check( pMesh && pMesh->GetFaceCount() == 1, "a mesh with normals loads" );
        if ( pMesh )
        {
            CVector oUp( 0.0, 1.0, 0.0 );
            CVector oDifference = pMesh->FaceNormalAt( 0, oUp ) - oUp;
            Check( oDifference.Magnitude() < 1e-12, "a mesh shades with its given vertex normals" );
        }
    }
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"triangle\", \"corners\": [[0, 1, 0], [1, 0, 0], [-1, 0, 0]], \"smooth\": false, "
                          "\"normals\": [[0, 0, -1], [0, 0, -1], [0, 0, -1]]" ),
                 BAD_FILE, "objects[0]: normals are only used by smooth shading" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"triangle\", \"corners\": [[0, 1, 0], [1, 0, 0], [-1, 0, 0]], "
                          "\"normals\": [[0, 0, -1], [0, 0, 0], [0, 0, -1]]" ),
                 BAD_FILE, "objects[0].normals[1]: a normal can't be [0, 0, 0]" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"mesh\", \"vertices\": [[0, 0, 0], [1, 0, 0], [0, 1, 0]], \"faces\": [[0, 1, 2]], "
                          "\"normals\": [[0, 0, -1]]" ),
                 BAD_FILE, "objects[0].normals: must be an array of [x, y, z] normals, one for each of the 3 vertices" );

        // A mesh of two faces folded along a shared edge: smooth shading must
        // give the same normal from either face at a point on that edge, and
        // flat shading different ones.
    for ( int nSmooth = 0; nSmooth < 2; nSmooth++ )
    {
        std::string strMesh = std::string( "{ \"type\": \"mesh\", \"doubleSided\": true, \"smooth\": " ) + ( nSmooth ? "true" : "false" ) +
                              ", \"vertices\": [[-1, 0, 0], [1, 0, 0], [0, 1, 0], [0, -1, -1]], \"faces\": [[0, 2, 1], [0, 1, 3]] }";
        CScene  oMeshScene;
        CMesh  *pMesh = dynamic_cast<CMesh *>( FirstObject( oMeshScene, strMesh.c_str() ) );

        Check( pMesh && pMesh->GetFaceCount() == 2, "a mesh builds its faces" );
        if ( ! pMesh )
            continue;

            // Shade the point (0.25, 0, 0) on the shared edge through each
            // face, with no lights, and compare the normals the faces use.
        CVector aoNormals[ 2 ];
        for ( int nFace = 0; nFace < 2; nFace++ )
            aoNormals[ nFace ] = pMesh->FaceNormalAt( nFace, CVector( 0.25, 0.0, 0.0 ) );

        CVector oDifference = aoNormals[ 0 ] - aoNormals[ 1 ];
        if ( nSmooth )
            Check( oDifference.Magnitude() < 1e-12, "smooth faces agree on the normal along a shared edge" );
        else
            Check( oDifference.Magnitude() > 0.1, "flat faces keep their own normals" );
    }
}

    // s_strMinimal's camera with this lens.
static std::string WithLens( const char *strLens )
{
    return Minimal( "\"lookAt\": [0, 0, 0]", ( std::string( "\"lookAt\": [0, 0, 0], \"lens\": " ) + strLens ).c_str() );
}

static void LensChecks( void )
{
    CScene  oScene;

        // The double Gauss, focused at infinity: a 50 mm f/2 lens whose back
        // focal distance is about 36 mm.
    Check( oScene.FromJson( WithLens( "{ \"prescription\": \"double-gauss-50mm\", \"focusDistance\": 1e9 }" ) ) == 0,
           "the double Gauss loads" );
    CLens *pLens = oScene.GetCamera().GetLens();
    Check( pLens && ABS( pLens->GetFocalLength() - 50.358 ) < 0.01, "the double Gauss is a 50 mm lens" );
    Check( pLens && ABS( pLens->GetWideOpen() - 2.02 ) < 0.01, "the double Gauss opens to f/2" );
    Check( pLens && ABS( pLens->GetFilmDistance() - 36.106 ) < 0.01, "focused at infinity, the sensor is 36.1 mm behind it" );
    Check( oScene.GetCamera().GetExposure() >= 1.0, "a lens lets through no more light than a pinhole" );

        // Focusing closer moves the sensor back.
    Check( oScene.FromJson( WithLens( "{ \"prescription\": \"double-gauss-50mm\", \"focusDistance\": 1.0 }" ) ) == 0 &&
           oScene.GetCamera().GetLens()->GetFilmDistance() > 36.2, "focusing at 1 m moves the sensor back" );

        // A lens can be given surface by surface, and saves that way.
    const char *strSingle = "{ \"surfaces\": [ { \"radius\": 50, \"thickness\": 5, \"ior\": 1.5, \"aperture\": 10 }, "
                            "{ \"radius\": 0, \"thickness\": 2, \"aperture\": 6, \"stop\": true }, "
                            "{ \"radius\": -50, \"thickness\": 0, \"aperture\": 10 } ], \"focusDistance\": 5 }";
    Check( oScene.FromJson( WithLens( strSingle ) ) == 0, "a lens given surface by surface loads" );
    Check( oScene.ToJson().find( "\"surfaces\"" ) != std::string::npos && oScene.ToJson().find( "\"stop\": true" ) != std::string::npos,
           "a lens given surface by surface saves its surfaces" );

        // Chromatic aberration: a BK7 singlet focuses blue nearer than red, by
        // about f / V; the double Gauss is corrected, so its colors nearly agree.
    const char *strSinglet = "{ \"surfaces\": [ { \"radius\": 50, \"thickness\": 5, \"ior\": 1.5168, \"abbe\": 64.17, \"aperture\": 12 }, "
                             "{ \"radius\": -50, \"thickness\": 3, \"aperture\": 12 }, "
                             "{ \"radius\": 0, \"thickness\": 0, \"aperture\": 7, \"stop\": true } ], \"focusDistance\": 5 }";
    Check( oScene.FromJson( WithLens( strSinglet ) ) == 0, "a singlet with an Abbe number loads" );
    pLens = oScene.GetCamera().GetLens();
    if ( pLens )
    {
        FLT nRed = pLens->FocalLengthIn( CLens::CHANNEL_RED );
        FLT nGreen = pLens->FocalLengthIn( CLens::CHANNEL_GREEN );
        FLT nBlue = pLens->FocalLengthIn( CLens::CHANNEL_BLUE );

        Check( nRed > nGreen && nGreen > nBlue, "a singlet focuses red farther away than green, and green than blue" );
        Check( ABS( ( nRed - nBlue ) - nGreen / 64.17 ) < 0.1 * nGreen / 64.17, "a singlet's color focus shift is about f / V" );
        Check( nGreen == pLens->GetFocalLength(), "green is traced at the index as given" );
        Check( oScene.GetCamera().GetChannels() == 3, "a dispersing lens traces three channels" );
    }
    Check( oScene.ToJson().find( "\"abbe\": 64.17" ) != std::string::npos, "Abbe numbers are saved" );

    Check( oScene.FromJson( WithLens( "{ \"prescription\": \"double-gauss-50mm\" }" ) ) == 0, "the double Gauss loads again" );
    pLens = oScene.GetCamera().GetLens();
    Check( pLens && ABS( pLens->FocalLengthIn( CLens::CHANNEL_RED ) - pLens->FocalLengthIn( CLens::CHANNEL_BLUE ) ) < 0.1,
           "the double Gauss is corrected for color (red and blue focal lengths within 0.1 mm)" );
    Check( oScene.FromJson( WithLens( "{ \"prescription\": \"double-gauss-50mm\", \"chromatic\": false }" ) ) == 0 &&
           oScene.GetCamera().GetChannels() == 1, "chromatic aberration can be turned off" );
    ExpectError( WithLens( "{ \"surfaces\": [ { \"radius\": 50, \"thickness\": 5, \"ior\": 1.5, \"abbe\": -3, \"aperture\": 10 } ] }" ),
                 BAD_FILE, "camera.lens.surfaces[0]: abbe can't be negative" );

        // The blueprint's "simplified double Gauss" diverges: it can't form an image.
    ExpectError( WithLens( "{ \"surfaces\": [ { \"radius\": 38.94, \"thickness\": 5.12, \"ior\": 1.658, \"aperture\": 16.3 }, "
                           "{ \"radius\": 101.44, \"thickness\": 4.11, \"aperture\": 15.1 }, "
                           "{ \"radius\": 24.9, \"thickness\": 3.76, \"ior\": 1.643, \"aperture\": 12.3 }, "
                           "{ \"radius\": 15.54, \"thickness\": 4.9, \"aperture\": 11.2 }, "
                           "{ \"radius\": 0, \"thickness\": 3.23, \"aperture\": 10.5, \"stop\": true }, "
                           "{ \"radius\": -14.65, \"thickness\": 1.25, \"ior\": 1.613, \"aperture\": 10.3 }, "
                           "{ \"radius\": -29.81, \"thickness\": 5.18, \"ior\": 1.62, \"aperture\": 11.7 }, "
                           "{ \"radius\": 21.84, \"thickness\": 15.35, \"aperture\": 12.3 } ] }" ),
                 BAD_FILE, "camera.lens: the lens doesn't bring light to a focus" );
    ExpectError( WithLens( "{ \"prescription\": \"double-gauss-50mm\", \"fNumber\": 1.4 }" ), BAD_FILE,
                 "camera.lens: the lens opens only to f/2.02" );
    ExpectError( WithLens( "{ \"prescription\": \"double-gauss-50mm\", \"focusDistance\": 0.01 }" ), BAD_FILE,
                 "camera.lens: the lens can't focus at 0.01 scene units" );
    ExpectError( WithLens( "{ \"prescription\": \"zoom-24-70mm\" }" ), BAD_FILE, "camera.lens: unknown lens prescription" );
    ExpectError( WithLens( "{ \"prescription\": \"double-gauss-50mm\", \"surfaces\": [] }" ), BAD_FILE,
                 "camera.lens: a lens needs a \"prescription\"" );
    ExpectError( WithLens( "{ \"surfaces\": [ { \"radius\": 5, \"thickness\": 1, \"aperture\": 10 } ] }" ), BAD_FILE,
                 "camera.lens.surfaces[0]: the surface is wider than its sphere" );
}

static void TextureChecks( const char *strScenes )
{
    CScene      oScene;
    std::string strFile = std::string( strScenes ) + "/texture_test.ray";

    Check( oScene.Load( strFile.c_str() ) == 0, "the texture test scene loads, with its image" );

        // A saved scene names the image; a copy for a task carries its pixels,
        // and loads with no file to read.
    std::string strSaved = oScene.ToJson( FALSE );
    std::string strCopy = oScene.ToJson( TRUE );
    Check( strSaved.find( "\"pixels\"" ) == std::string::npos && strSaved.find( "textures/uv_grid.ppm" ) != std::string::npos,
           "a saved scene names its texture images" );
    Check( strCopy.find( "\"pixels\"" ) != std::string::npos, "a copy for a task carries its texture images" );

    CScene oCopy;
    Check( oCopy.FromJson( strCopy ) == 0, "a copy with embedded images loads without the files" );
    Check( oCopy.ToJson( TRUE ) == strCopy, "an embedded image reads back exactly" );

    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"marble\" } } ]" ),
                 BAD_FILE, "objects[0].materials[0].texture: unknown texture type \"marble\"" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"image\", \"file\": \"no/such.ppm\" } } ]" ),
                 BAD_FILE, "objects[0].materials[0].texture: can't open the image \"no/such.ppm\"" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"image\", "
                          "\"width\": 2, \"height\": 2, \"pixels\": \"AAAA\" } } ]" ),
                 BAD_FILE, "pixels must be width x height RGB bytes" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"checker\", \"size\": 0 } } ]" ),
                 BAD_FILE, "objects[0].materials[0].texture: size must be positive" );
}

    // The surface coordinates of face nFace of a scene's first object (a mesh)
    // at a point.
static bool MeshCoords( CScene &oScene, int nFace, double dX, double dY, double dZ, FLT &nU, FLT &nV )
{
    CMesh *pMesh = dynamic_cast<CMesh *>( oScene.GetObjectAt( 0 ) );

    return pMesh && nFace < pMesh->GetFaceCount() && pMesh->FaceCoordsAt( nFace, CVector( dX, dY, dZ ), nU, nV );
}

static void UVMapChecks( void )
{
    CScene  oScene;
    FLT     nU, nV;

        // Planar along y: x and z across the bounding box, 0 to 1.
    Check( oScene.FromJson( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
           "\"type\": \"mesh\", \"vertices\": [[-2, 0, -1], [2, 0, -1], [2, 0, 1], [-2, 0, 1]], \"faces\": [[0, 1, 2], [0, 2, 3]], "
           "\"uvMap\": { \"type\": \"planar\", \"axis\": \"y\" }" ) ) == 0, "a mesh with a planar UV map loads" );
    Check( MeshCoords( oScene, 0, 2.0, 0.0, -1.0, nU, nV ) && ABS( nU - 1.0 ) < 1e-9 && ABS( nV - 0.0 ) < 1e-9,
           "a planar map along y runs u along x and v along z, 0 to 1 across the bounding box" );

        // Cylindrical about y: a face across the seam (u near 1 and near 0)
        // runs on past 1 rather than back across the texture.
    Check( oScene.FromJson( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
           "\"type\": \"mesh\", \"vertices\": [[-1, 0, -0.1], [-1, 0, 0.1], [-1, 1, 0], [1, 0, 0], [1, 1, 0]], \"faces\": [[0, 1, 2], [3, 4, 2]], "
           "\"uvMap\": { \"type\": \"cylindrical\", \"axis\": \"y\", \"center\": [0, 0, 0] }" ) ) == 0,
           "a mesh with a cylindrical UV map loads" );
    {
        FLT nU0, nV0, nU1, nV1;
        bool bOk = MeshCoords( oScene, 0, -1.0, 0.0, -0.1, nU0, nV0 ) && MeshCoords( oScene, 0, -1.0, 0.0, 0.1, nU1, nV1 );
        Check( bOk && ABS( nU0 - nU1 ) < 0.1, "a face across the seam is unwrapped" );
    }

        // Spherical: a corner on the pole takes its neighbors' angle.
    Check( oScene.FromJson( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
           "\"type\": \"mesh\", \"vertices\": [[0, 1, 0], [1, 0, 0.2], [1, 0, -0.2]], \"faces\": [[0, 1, 2]], "
           "\"uvMap\": { \"type\": \"spherical\", \"center\": [0, 0, 0] }" ) ) == 0, "a mesh with a spherical UV map loads" );
    {
        FLT nUPole, nVPole;
        bool bOk = MeshCoords( oScene, 0, 0.0, 1.0, 0.0, nUPole, nVPole );
        Check( bOk && ABS( nUPole - 0.5 ) < 1e-6 && ABS( nVPole - 1.0 ) < 1e-6, "a pole corner has v 1 and its neighbors' u" );
    }

        // Box: each face is projected along the axis it faces most.
    Check( oScene.FromJson( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
           "\"type\": \"mesh\", \"vertices\": [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 0, 1]], \"faces\": [[0, 2, 1], [0, 1, 3]], "
           "\"uvMap\": { \"type\": \"box\" }" ) ) == 0, "a mesh with a box UV map loads" );
    Check( MeshCoords( oScene, 0, 1.0, 1.0, 0.0, nU, nV ) && ABS( nU - 1.0 ) < 1e-6 && ABS( nV - 1.0 ) < 1e-6 &&
           MeshCoords( oScene, 1, 0.0, 0.0, 1.0, nU, nV ) && ABS( nU - 0.0 ) < 1e-6 && ABS( nV - 1.0 ) < 1e-6,
           "a box map projects each face along its own axis" );

    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"mesh\", \"vertices\": [[0, 0, 0], [1, 0, 0], [0, 1, 0]], \"faces\": [[0, 1, 2]], "
                          "\"uvs\": [[0, 0], [1, 0], [0, 1]], \"uvMap\": { \"type\": \"planar\" }" ),
                 BAD_FILE, "objects[0].uvMap: a mesh takes \"uvs\" or a \"uvMap\", not both" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"mesh\", \"vertices\": [[0, 0, 0], [1, 0, 0], [0, 1, 0]], \"faces\": [[0, 1, 2]], "
                          "\"uvMap\": { \"type\": \"conical\" }" ),
                 BAD_FILE, "objects[0].uvMap: unknown uvMap type \"conical\"" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"mesh\", \"vertices\": [[0, 0, 0], [1, 0, 0], [0, 1, 0]], \"faces\": [[0, 1, 2]], "
                          "\"uvMap\": { \"type\": \"planar\", \"axis\": \"w\" }" ),
                 BAD_FILE, "objects[0].uvMap: axis must be" );
}

    // The angle in degrees between a direction and -N (straight through).
static double AngleFromNormal( CVector oDirection, CVector oInto )
{
    oDirection.Normalize();
    oInto.Normalize();
    return acos( fmax( -1.0, fmin( 1.0, oDirection.Dot( oInto ) ) ) ) * 180.0 / 3.14159265358979323846;
}

static void MaterialChecks( void )
{
    CVector oNormal( 0.0, 1.0, 0.0 );
    CVector oDown( 0.0, -1.0, 0.0 );
    CVector oUp( 0.0, 1.0, 0.0 );
    CVector oTransmit;

        // Snell: 45 degrees into glass of index 1.5 comes out at
        // asin( sin 45 / 1.5 ) = 28.1255 degrees from the normal.
    CVector oIn45( 0.70710678118654752, -0.70710678118654752, 0.0 );
    Check( CTransparentMaterial::Refract( oIn45, oNormal, 1.5, oTransmit ) &&
           ABS( AngleFromNormal( oTransmit, oDown ) - 28.1255 ) < 0.001, "refraction into glass follows Snell's law" );

        // And back out: 28.1255 degrees inside leaves at 45.
    CVector oInside( sin( 28.1255 * 3.14159265358979323846 / 180 ), cos( 28.1255 * 3.14159265358979323846 / 180 ), 0.0 );
    Check( CTransparentMaterial::Refract( oInside, oNormal, 1.5, oTransmit ) &&
           ABS( AngleFromNormal( oTransmit, oUp ) - 45.0 ) < 0.001, "refraction out of glass follows Snell's law" );

    Check( CTransparentMaterial::Refract( oDown, oNormal, 1.5, oTransmit ) && AngleFromNormal( oTransmit, oDown ) < 1e-9,
           "a ray straight into glass goes straight on" );

        // Past the critical angle (41.8 degrees for 1.5), from inside, all of
        // it reflects.
    CVector oSteep( sin( 60.0 * 3.14159265358979323846 / 180 ), cos( 60.0 * 3.14159265358979323846 / 180 ), 0.0 );
    Check( ! CTransparentMaterial::Refract( oSteep, oNormal, 1.5, oTransmit ) && oTransmit.GetY() < 0.0 &&
           ABS( AngleFromNormal( oTransmit, oDown ) - 60.0 ) < 1e-6, "past the critical angle the ray reflects inside" );

        // Specular: no highlight (and no NaN) for a light on the horizon or
        // behind the surface; a highlight where the light mirrors to the eye.
    CScene              oScene;
    CSpecularMaterial   oSpecular;
    CColor              oWhite( 1, 1, 1 );
    CRay                oEye;
    CVector             oHit( 0, 0, 0 );

    Check( oScene.FromJson( s_strMinimal ) == 0, "a scene for the material checks loads" );
    oEye.Set( -1.0, 1.0, 0.0, 0.70710678118654752, -0.70710678118654752, 0.0, 1e100, 1.0, 0, 0 );

    CVector oHorizon( 1.0, 0.0, 0.0 ), oBehind( 1.0, -1.0, 0.0 ), oMirror( 1.0, 1.0, 0.0 ), oN( oNormal );
    CColor  oColor = oSpecular.GetColor( oScene, oEye, oWhite, oHorizon, oHit, oN, NULL );
    Check( oColor.GetR() == 0.0 && oColor.GetG() == 0.0, "no highlight from a light on the horizon" );
    oN = oNormal;
    oColor = oSpecular.GetColor( oScene, oEye, oWhite, oBehind, oHit, oN, NULL );
    Check( oColor.GetR() == 0.0, "no highlight from a light behind the surface" );
    oN = oNormal;
    oColor = oSpecular.GetColor( oScene, oEye, oWhite, oMirror, oHit, oN, NULL );
    Check( oColor.GetR() > 0.0 && isfinite( (double)oColor.GetR() ), "a highlight where the light mirrors to the eye, with the default index" );

        // Shading a plane from behind must not change the plane.
    CScene oPlaneScene;
    Check( oPlaneScene.FromJson( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                                          "\"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [0, 1, 0]" ) ) == 0,
           "a plane scene loads" );
    std::string strBefore = oPlaneScene.ToJson();
    CRay        oFromBelow;
    CColor      oResult;

    oFromBelow.Set( 0.3, -2.0, 0.1, 0.0, 1.0, 0.0, 1e100, 1.0, 0, 0 );
    oPlaneScene.TraceRay( oFromBelow, oResult, NULL );
    Check( oPlaneScene.ToJson() == strBefore, "shading a plane from behind leaves the plane as it was" );
}

static void RoundTrip( const char *strScene, const char *strDir )
{
    std::string strBase = std::string( strDir ) + "/roundtrip_";
    const char *strName = strrchr( strScene, '/' );
    strBase += ( strName ) ? strName + 1 : strScene;

    std::string strFirst = strBase + ".1";
    std::string strSecond = strBase + ".2";

    CScene  oScene;
    Check( oScene.Load( strScene ) == 0, "the scene loads" );
    Check( oScene.Save( strFirst.c_str() ) == 0, "the scene saves" );
    Check( ReadFile( strFirst ).compare( 0, 8, "IASRES\r\n" ) == 0, "a saved scene is a resource archive" );

    CScene  oAgain;
    Check( oAgain.Load( strFirst.c_str() ) == 0, "the saved scene loads" );
    Check( oAgain.Save( strSecond.c_str() ) == 0, "the reloaded scene saves" );

        // Archives record when they were written, so compare the scenes.
    CScene  oThird;
    Check( oThird.Load( strSecond.c_str() ) == 0, "the second save loads" );
    Check( oScene.ToJson() == oAgain.ToJson() && oAgain.ToJson() == oThird.ToJson(), "saving is stable across a reload" );

    remove( strFirst.c_str() );
    remove( strSecond.c_str() );
}

    // A texture named outside the scene's directory is stored, and extracted,
    // under files/ instead.
static void ArchiveChecks( const char *strScenes, const char *strDir )
{
    std::string strOutside = std::string( strDir ) + "/archive_outside.ray";
    std::string strArchive = std::string( strDir ) + "/archive_test.ray";
    std::string strJsonDir = std::string( strDir ) + "/archive_json";
    std::string strJson = strJsonDir + "/scene.ray";
        // Forward slashes: a Windows path's backslashes would be JSON escapes.
    std::string strImage = std::filesystem::absolute( std::string( strScenes ) + "/textures/uv_grid.ppm" ).generic_string();
    CScene      oScene;

    {
        FILE *pFile = fopen( strOutside.c_str(), "wb" );
        std::string strText = Minimal( "\"radius\": 1", ( "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": "
                                       "{ \"type\": \"image\", \"file\": \"" + strImage + "\" } } ]" ).c_str() );
        fwrite( strText.data(), 1, strText.size(), pFile );
        fclose( pFile );
    }
    Check( oScene.Load( strOutside.c_str() ) == 0, "a scene naming a texture by absolute path loads" );
    Check( oScene.Save( strArchive.c_str() ) == 0, "it saves as an archive" );
    Check( oScene.SaveJson( strJson.c_str() ) == 0, "it saves as JSON" );
    Check( ReadFile( strJsonDir + "/files/uv_grid.ppm" ) == ReadFile( strImage ), "the texture is extracted under files/" );
    Check( ReadFile( strJson ).find( "\"files/uv_grid.ppm\"" ) != std::string::npos, "the JSON names the extracted texture" );

    CScene oArchived;
    Check( oArchived.Load( strArchive.c_str() ) == 0, "the archive loads with the texture from inside it" );

    remove( strOutside.c_str() );
    remove( strArchive.c_str() );
}

int main( int argc, char **argv )
{
    if ( argc >= 4 && strcmp( argv[ 1 ], "--textures" ) == 0 )
    {
        TextureChecks( argv[ 2 ] );
        ArchiveChecks( argv[ 2 ], argv[ 3 ] );
    }
    else if ( argc >= 3 )
        RoundTrip( argv[ 1 ], argv[ 2 ] );
    else
    {
        WireChecks();
        JsonChecks();
        TriangleChecks();
        LensChecks();
        UVMapChecks();
        MaterialChecks();
    }

    if ( g_nFailures == 0 )
        printf( "All checks passed\n" );
    return ( g_nFailures == 0 ) ? 0 : 1;
}
