// Checks that rays, colors and scenes cross the wire exactly, that JSON
// scenes are read and checked properly, and that scene files survive a load
// and save unchanged.
//
//   format_test                   the wire and JSON checks
//   format_test <scene.ray> <dir> load, save, load, save; the two saves must match
#include "Globals.h"
#include "WireProtocol.h"
#include "Mesh.h"
#include "Cone.h"
#include "Box.h"
#include "Sphere.h"
#include "ImageExport.h"
#include "HdrColorModel.h"
#include "Lens.h"
#include "AdaptiveSampler.h"
#include "ImageFile.h"
#include "ThreadTaskMaster.h"
#include "SocketTaskMaster.h"
#include "SocketTaskSlave.h"
#include "Streams/SocketStream.h"
#include <thread>
#include <chrono>
#include "SpecularMaterial.h"
#include "TransparentMaterial.h"
#include "MarbleTexture.h"
#include "NoiseTexture.h"
#include "WoodTexture.h"
#include "SceneJson.h"
#include "Scene.h"
#include "ReinhardColorModel.h"


#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <filesystem>
#include <memory>
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

    // Tiles and their pixels must cross the wire exactly.
static void WireChecks( void )
{
    const double        adValues[] = { 0.0, 0.1, -3.141592653589793, 1e-300, 6.02214076e23, 2.0 / 3.0, 1e100, -7.25, NAN };
    const int           nValues = (int)( sizeof( adValues ) / sizeof( adValues[ 0 ] ) );
    STile               oTile = { 64, 96, 7, 5 };
    STile               oRead;
    uint32_t            nTrace = 0;
    uint64_t            nRays = 0;
    std::vector<double> anColors, anRead;

    Check( RayWire::DecodeHello( RayWire::EncodeHello() ), "the hello reads back" );

    Check( RayWire::DecodeTile( RayWire::EncodeTile( 42, oTile ), nTrace, oRead ) && nTrace == 42 && oRead.nLeft == 64 &&
           oRead.nTop == 96 && oRead.nWidth == 7 && oRead.nHeight == 5, "a tile arrives exactly" );

    for ( int nIndex = 0; nIndex < 7 * 5 * 3; nIndex++ )
        anColors.push_back( adValues[ nIndex % nValues ] + nIndex * 1e-9 );
    Check( RayWire::DecodePixels( RayWire::EncodePixels( 43, oTile, 0x123456789ull, anColors ), nTrace, oRead, nRays, anRead ) &&
           nTrace == 43 && nRays == 0x123456789ull && oRead.nWidth == 7 && anRead.size() == anColors.size(),
           "a tile's pixels decode" );

    bool bExact = anRead.size() == anColors.size();

    for ( size_t nIndex = 0; bExact && nIndex < anColors.size(); nIndex++ )
        bExact = ( memcmp( &anRead[ nIndex ], &anColors[ nIndex ], sizeof( double ) ) == 0 );
    Check( bExact, "every pixel arrives exactly, bit for bit" );

    std::string strReadable = RayWire::ToReadable( MESSAGE_PIXELS, RayWire::EncodePixels( 43, oTile, 9, anColors ) );
    Check( strReadable.find( "\"pixels\"" ) != std::string::npos && strReadable.find( "\"left\": 64" ) != std::string::npos,
           "a pixels message can be read as JSON" );

    std::string strShort = RayWire::EncodePixels( 43, oTile, 9, anColors );
    strShort.resize( strShort.size() - 8 );
    Check( ! RayWire::DecodePixels( strShort, nTrace, oRead, nRays, anRead ), "a short pixels message is refused" );
    STile oEmpty = { 0, 0, 0, 5 };
    Check( ! RayWire::DecodeTile( RayWire::EncodeTile( 1, oEmpty ), nTrace, oRead ), "an empty tile is refused" );
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

    // A sphere scene whose diffuse material has the texture strTexture.
static std::string WithTexture( const char *strTexture )
{
    std::string strTo = std::string( "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": " ) + strTexture + " } ]";

    return Minimal( "\"radius\": 1", strTo.c_str() );
}

static void SolidTextureChecks( void )
{
    CNoiseTexture   oNoise;
    double          dLargest = 0.0;
    bool            bSmooth = true;
    bool            bLattice = true;

        // Gradient noise: 0 at every lattice point, within about -1 to 1, and
        // smooth (a tiny step makes a tiny change).
    for ( int nIndex = 0; nIndex < 20000; nIndex++ )
    {
        FLT nX = ( ( nIndex * 37 ) % 1000 ) * 0.0137 - 6.0;
        FLT nY = ( ( nIndex * 91 ) % 1000 ) * 0.0113 - 5.0;
        FLT nZ = ( ( nIndex * 53 ) % 1000 ) * 0.0171 - 8.0;
        FLT nValue = oNoise.Noise( nX, nY, nZ );

        dLargest = ( std::max )( dLargest, (double)ABS( nValue ) );
        if ( ABS( oNoise.Noise( nX + 1e-6, nY, nZ ) - nValue ) > 1e-4 )
            bSmooth = false;
        if ( ABS( oNoise.Noise( floor( nX ), floor( nY ), floor( nZ ) ) ) > 1e-12 )
            bLattice = false;
    }
    Check( dLargest > 0.5 && dLargest < 1.1, "gradient noise stays within about -1 to 1" );
    Check( bSmooth, "gradient noise is smooth" );
    Check( bLattice, "gradient noise is 0 on the lattice, negative cells too" );

        // The seed picks the noise; the same seed always the same.
    CScene oA, oB, oC;
    Check( oA.FromJson( WithTexture( "{ \"type\": \"noise\", \"seed\": 4 }" ) ) == 0 &&
           oB.FromJson( WithTexture( "{ \"type\": \"noise\", \"seed\": 4 }" ) ) == 0 &&
           oC.FromJson( WithTexture( "{ \"type\": \"noise\", \"seed\": 5 }" ) ) == 0, "noise textures load" );

    CNoiseTexture oFour, oFourAgain, oFive;
    std::unique_ptr<IASLib::CJsonNode> pFour( RayJson::Parse( "{ \"type\": \"noise\", \"seed\": 4 }" ) );
    std::unique_ptr<IASLib::CJsonNode> pFive( RayJson::Parse( "{ \"type\": \"noise\", \"seed\": 5 }" ) );
    Check( pFour && pFive && oFour.FromJson( *pFour ) && oFourAgain.FromJson( *pFour ) && oFive.FromJson( *pFive ),
           "noise settings read" );

    int nSame = 0, nDifferent = 0;
    for ( int nIndex = 0; nIndex < 100; nIndex++ )
    {
        FLT nX = nIndex * 0.31 + 0.1, nY = nIndex * 0.17 + 0.2, nZ = nIndex * 0.07 + 0.3;
        if ( oFour.Noise( nX, nY, nZ ) == oFourAgain.Noise( nX, nY, nZ ) )
            nSame++;
        if ( oFour.Noise( nX, nY, nZ ) != oFive.Noise( nX, nY, nZ ) )
            nDifferent++;
    }
    Check( nSame == 100, "the same seed gives the same noise" );
    Check( nDifferent > 90, "another seed gives other noise" );

        // The ramp: its ends, and evenly spaced blends between.
    std::unique_ptr<IASLib::CJsonNode> pRamp( RayJson::Parse( "{ \"colors\": [ [0, 0, 0], [1, 0, 0], [1, 1, 1] ] }" ) );
    CMarbleTexture oMarble;
    Check( pRamp && oMarble.FromJson( *pRamp ), "a ramp of three colors reads" );
    CColor oStart = oMarble.RampAt( -0.5 ), oEnd = oMarble.RampAt( 2.0 ), oQuarter = oMarble.RampAt( 0.25 ), oMiddle = oMarble.RampAt( 0.5 );
    Check( oStart.GetR() == 0.0 && oEnd.GetB() == 1.0, "the ramp is clamped at its ends" );
    Check( ABS( oQuarter.GetR() - 0.5 ) < 1e-12 && oQuarter.GetG() == 0.0 && oMiddle.GetR() == 1.0 && oMiddle.GetG() == 0.0,
           "the ramp blends evenly between its colors" );

        // Every kind round-trips through JSON, axis made a unit vector.
    const char *astrKinds[] =
    {
        "{ \"type\": \"marble\", \"axis\": [0, 2, 0], \"scale\": 0.5, \"turbulence\": 1.5, \"octaves\": 4, \"seed\": 9 }",
        "{ \"type\": \"wood\", \"origin\": [1, 0, 2], \"colors\": [ [1, 0.8, 0.5], [0.4, 0.2, 0.1] ] }",
        "{ \"type\": \"noise\", \"turbulence\": 1 }",
    };
    for ( size_t nIndex = 0; nIndex < sizeof( astrKinds ) / sizeof( astrKinds[ 0 ] ); nIndex++ )
    {
        CScene oScene, oAgain;
        Check( oScene.FromJson( WithTexture( astrKinds[ nIndex ] ) ) == 0, "a solid texture loads" );
        std::string strSaved = oScene.ToJson();
        Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "a solid texture reads back as it was saved" );
        if ( nIndex == 0 )
            Check( strSaved.find( "\"axis\": [0.0, 1.0, 0.0]" ) != std::string::npos, "a marble axis is saved as a unit vector" );
    }

    ExpectError( WithTexture( "{ \"type\": \"marble\", \"scale\": 0 }" ), BAD_FILE, "texture: scale must be positive" );
    ExpectError( WithTexture( "{ \"type\": \"wood\", \"axis\": [0, 0, 0] }" ), BAD_FILE, "texture: axis can't be [0, 0, 0]" );
    ExpectError( WithTexture( "{ \"type\": \"noise\", \"octaves\": 13 }" ), BAD_FILE, "octaves" );
    ExpectError( WithTexture( "{ \"type\": \"noise\", \"turbulence\": -1 }" ), BAD_FILE, "turbulence can't be negative" );
    ExpectError( WithTexture( "{ \"type\": \"noise\", \"colors\": [ [1, 1, 1] ] }" ), BAD_FILE, "two or more colors" );
    ExpectError( WithTexture( "{ \"type\": \"noise\", \"size\": 1 }" ), BAD_FILE, "size" );
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

    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"plaid\" } } ]" ),
                 BAD_FILE, "objects[0].materials[0].texture: unknown texture type \"plaid\"" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"image\", \"file\": \"no/such.ppm\" } } ]" ),
                 BAD_FILE, "objects[0].materials[0].texture: can't open the image \"no/such.ppm\"" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"image\", "
                          "\"width\": 2, \"height\": 2, \"pixels\": \"AAAA\" } } ]" ),
                 BAD_FILE, "pixels must be width x height RGB bytes" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"checker\", \"size\": 0 } } ]" ),
                 BAD_FILE, "objects[0].materials[0].texture: size must be positive" );

    SolidTextureChecks();
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

    // The color a ray along +z from (dX, 0, -5) brings back through a glass
    // ball of radius 1 at the origin, against a white background, with
    // transparent settings strGlass (index 1, so it goes straight through).
static CColor ThroughBall( const char *strGlass, double dX )
{
    std::string strTo = std::string( "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"color\": [1, 1, 1], "
                                     "\"indexOfRefraction\": 1" ) + strGlass + " } ]";
    std::string strText = Minimal( "\"radius\": 1", strTo.c_str() );
    CScene      oScene;
    CRay        oRay;
    CColor      oColor( 0, 0, 0 );

    strText.replace( strText.find( "\"camera\"" ), 0, "\"background\": [1, 1, 1], " );
    Check( oScene.FromJson( strText ) == 0, "a glass ball scene loads" );
    oRay.Set( dX, 0.0, -5.0, 0.0, 0.0, 1.0, 1e100, 1.0, 0, 0 );
    oScene.TraceRay( oRay, oColor, NULL );
    return oColor;
}

static void AbsorptionChecks( void )
{
        // Beer-Lambert: through the middle the ray goes 2 inside, so with
        // [0.5, 0.25, 1] left after 1 it keeps [0.25, 0.0625, 1].
    const char *strTinted = ", \"attenuationColor\": [0.5, 0.25, 1], \"attenuationDistance\": 1";
    CColor      oMiddle = ThroughBall( strTinted, 0.0 );
    CColor      oClear = ThroughBall( "", 0.0 );

    Check( ABS( oClear.GetR() - 1.0 ) < 1e-9 && ABS( oClear.GetG() - 1.0 ) < 1e-9, "clear glass absorbs nothing" );
    Check( ABS( oMiddle.GetR() - 0.25 ) < 1e-9 && ABS( oMiddle.GetG() - 0.0625 ) < 1e-9 && ABS( oMiddle.GetB() - 1.0 ) < 1e-9,
           "glass absorbs by the distance through it" );

        // Off center the chord is 2 sqrt( 1 - 0.6^2 ) = 1.6.
    CColor oSide = ThroughBall( strTinted, 0.6 );
    Check( ABS( oSide.GetR() - pow( 0.5, 1.6 ) ) < 1e-9 && ABS( oSide.GetG() - pow( 0.25, 1.6 ) ) < 1e-9,
           "a shorter path through glass absorbs less" );

        // Twice the distance for the same color is half the absorption.
    CColor oThinner = ThroughBall( ", \"attenuationColor\": [0.5, 0.25, 1], \"attenuationDistance\": 2", 0.0 );
    Check( ABS( oThinner.GetR() - 0.5 ) < 1e-9 && ABS( oThinner.GetG() - 0.25 ) < 1e-9, "attenuationDistance scales the absorption" );

    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"attenuationDistance\": -1 } ]" ),
                 BAD_FILE, "attenuationDistance can't be negative" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"attenuationColor\": [1.5, 1, 1] } ]" ),
                 BAD_FILE, "attenuationColor must be between 0 and 1" );
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

    AbsorptionChecks();
}

    // A ray from (dX, dY, dZ) along (dI, dJ, dK), which must be a unit vector.
static CRay RayFrom( double dX, double dY, double dZ, double dI, double dJ, double dK )
{
    CRay oRay;

    oRay.Set( dX, dY, dZ, dI, dJ, dK, 1e100, 1.0, 0, 0 );
    return oRay;
}

    // The ray hits oObject at t = dT, where the outward normal is
    // (dNX, dNY, dNZ).
static bool HitsAt( CObjects &oObject, CRay oRay, double dT, double dNX, double dNY, double dNZ )
{
    CVector oHit;

    if ( ! oObject.Intersect( oRay, oHit ) || ABS( oRay.GetT() - dT ) > 1e-9 )
        return false;

    CCone  *pCone = dynamic_cast<CCone *>( &oObject );
    CBox   *pBox = dynamic_cast<CBox *>( &oObject );
    CVector oNormal = pCone ? pCone->NormalAt( oHit ) : pBox->NormalAt( oHit );

    return ABS( oNormal.GetX() - dNX ) < 1e-9 && ABS( oNormal.GetY() - dNY ) < 1e-9 && ABS( oNormal.GetZ() - dNZ ) < 1e-9;
}

static bool Misses( CObjects &oObject, CRay oRay )
{
    CVector oHit;

    return oObject.Intersect( oRay, oHit ) ? false : true;
}

static void PrimitiveChecks( void )
{
    CScene      oScene;
    CObjects   *pObject;
    CVector     oHit;

        // A capped cylinder of radius 1 from y = -1 to 1.
    pObject = FirstObject( oScene, "{ \"type\": \"cylinder\", \"base\": [0, -1, 0], \"top\": [0, 1, 0], \"radius\": 1 }" );
    if ( pObject )
    {
        Check( HitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a ray hits a cylinder's side" );
        Check( HitsAt( *pObject, RayFrom( 0.5, 5, 0, 0, -1, 0 ), 4.0, 0, 1, 0 ), "a ray hits a cylinder's top cap" );
        Check( HitsAt( *pObject, RayFrom( 0.5, -5, 0, 0, 1, 0 ), 4.0, 0, -1, 0 ), "a ray hits a cylinder's base cap" );
        Check( HitsAt( *pObject, RayFrom( 0, 0, 0, 1, 0, 0 ), 1.0, 1, 0, 0 ), "a ray from inside a cylinder hits its side" );
        Check( Misses( *pObject, RayFrom( 0, 1.5, -5, 0, 0, 1 ) ), "a ray over a cylinder misses it" );
        Check( Misses( *pObject, RayFrom( 0, 0, -5, 0, 0, -1 ) ), "a ray away from a cylinder misses it" );

        CRay oLeaving = RayFrom( 0, 0, -1, 0, 0, 1 );
        Check( ! pObject->Intersect( oLeaving, oHit, pObject ), "a ray leaving a capped cylinder doesn't hit it again" );

            // u as on a sphere, v up the side.
        CSphere oSphere( 0.0, 0.0, 0.0, 1.0 );
        CVector oSide( 0.0, 0.0, 1.0 );
        FLT     nU, nV, nSphereU, nSphereV;
        Check( pObject->SurfaceCoords( oSide, nU, nV ) && oSphere.SurfaceCoords( oSide, nSphereU, nSphereV ) &&
               ABS( nU - nSphereU ) < 1e-12 && ABS( nV - 0.5 ) < 1e-12, "a cylinder's u runs as a sphere's, and v up its side" );
        CVector oCap( 0.5, 1.0, 0.0 );
        Check( pObject->SurfaceCoords( oCap, nU, nV ) && ABS( nU - 0.75 ) < 1e-12 && ABS( nV - 0.5 ) < 1e-12,
               "a cylinder's top cap fills the unit square" );
    }

        // The same, open.
    pObject = FirstObject( oScene, "{ \"type\": \"cylinder\", \"base\": [0, -1, 0], \"top\": [0, 1, 0], \"radius\": 1, \"capped\": false }" );
    if ( pObject )
    {
        Check( Misses( *pObject, RayFrom( 0.5, 5, 0, 0, -1, 0 ) ), "a ray down an open cylinder passes through" );
        Check( HitsAt( *pObject, RayFrom( 0, 0.5, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a ray hits an open cylinder's side" );

        CRay oLeaving = RayFrom( 0, 0, -1, 0, 0, 1 );
        Check( pObject->Intersect( oLeaving, oHit, pObject ) && ABS( oLeaving.GetT() - 2.0 ) < 1e-9,
               "a ray leaving an open cylinder's inside hits its far side" );
        CRay oOutward = RayFrom( 0, 0, -1, 0, 0, -1 );
        Check( ! pObject->Intersect( oOutward, oHit, pObject ), "a ray leaving an open cylinder outward doesn't hit it" );
    }

        // A cone from radius 1 at y = 0 to a point at y = 2.
    pObject = FirstObject( oScene, "{ \"type\": \"cone\", \"base\": [0, 0, 0], \"top\": [0, 2, 0], \"baseRadius\": 1 }" );
    if ( pObject )
    {
        double dLength = sqrt( 1.25 );

        Check( HitsAt( *pObject, RayFrom( 0, 1, -5, 0, 0, 1 ), 4.5, 0, 0.5 / dLength, -1.0 / dLength ),
               "a ray hits a cone's side, where the normal tips up by the slope" );
        Check( HitsAt( *pObject, RayFrom( 0.2, -1, 0, 0, 1, 0 ), 1.0, 0, -1, 0 ), "a ray hits a cone's base" );
        Check( Misses( *pObject, RayFrom( 0, 2.1, -5, 0, 0, 1 ) ), "a ray over a cone's point misses it" );
        Check( Misses( *pObject, RayFrom( 0, 1, -5, 0, 1, 0 ) ), "a ray past a cone misses it, and the other half of the double cone" );
    }

        // Truncated: radius 0.5 at the top.
    pObject = FirstObject( oScene, "{ \"type\": \"cone\", \"base\": [0, 0, 0], \"top\": [0, 2, 0], \"baseRadius\": 1, \"topRadius\": 0.5 }" );
    if ( pObject )
    {
        Check( HitsAt( *pObject, RayFrom( 0.4, 5, 0, 0, -1, 0 ), 3.0, 0, 1, 0 ), "a ray hits a truncated cone's top" );
            // Radius 0.75 at y = 1; the radius shrinks 0.25 a unit up.
        Check( HitsAt( *pObject, RayFrom( 0.75, 5, 0, 0, -1, 0 ), 4.0, 1.0 / sqrt( 1.0625 ), 0.25 / sqrt( 1.0625 ), 0 ),
               "a ray past a truncated cone's top hits its side" );
    }

        // A box 2 by 4 by 6, turned 90 degrees about y: its x runs along -z
        // and its z along +x.
    pObject = FirstObject( oScene, "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": [2, 4, 6], \"rotation\": [0, 90, 0] }" );
    if ( pObject )
    {
        Check( HitsAt( *pObject, RayFrom( -10, 0, 0, 1, 0, 0 ), 7.0, -1, 0, 0 ), "a ray hits a turned box's long side" );
        Check( HitsAt( *pObject, RayFrom( 0, 0, -10, 0, 0, 1 ), 9.0, 0, 0, -1 ), "a ray hits a turned box's short side" );
        Check( HitsAt( *pObject, RayFrom( 0, 10, 0, 0, -1, 0 ), 8.0, 0, 1, 0 ), "a ray hits a box's top" );
        Check( Misses( *pObject, RayFrom( -10, 2.5, 0, 1, 0, 0 ) ), "a ray over a box misses it" );
    }

        // A cube of size 2, and the texture on its faces.
    pObject = FirstObject( oScene, "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 2 }" );
    if ( pObject )
    {
        Check( HitsAt( *pObject, RayFrom( 0, 0, 0, 1, 0, 0 ), 1.0, 1, 0, 0 ), "a ray from inside a cube hits its side" );

        FLT     nU, nV;
        CVector oFront( 0.5, -0.5, -1.0 ), oRight( 1.0, -0.5, 0.5 );
        Check( pObject->SurfaceCoords( oFront, nU, nV ) && ABS( nU - 0.75 ) < 1e-12 && ABS( nV - 0.25 ) < 1e-12,
               "a cube's front face holds the unit square, upright" );
        Check( pObject->SurfaceCoords( oRight, nU, nV ) && ABS( nU - 0.75 ) < 1e-12 && ABS( nV - 0.25 ) < 1e-12,
               "a cube's right face holds the unit square, upright as seen from the right" );
        Check( oScene.ToJson().find( "\"size\": 2" ) != std::string::npos, "a cube's size is written as one number" );
    }

    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"cylinder\", \"base\": [0, 0, 0], \"top\": [0, 1, 0], \"radius\": 0" ),
                 BAD_FILE, "objects[0]: the radius must be more than 0" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"cone\", \"base\": [0, 1, 0], \"top\": [0, 1, 0], \"baseRadius\": 1" ),
                 BAD_FILE, "objects[0]: the base and top can't be the same point" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"cone\", \"base\": [0, 0, 0], \"top\": [0, 1, 0], \"baseRadius\": -1" ),
                 BAD_FILE, "objects[0]: a radius can't be negative" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"box\", \"center\": [0, 0, 0], \"size\": [1, 0, 1]" ),
                 BAD_FILE, "objects[0]: every size must be more than 0" );
    ExpectError( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
                          "\"type\": \"cylinder\", \"base\": [0, 0, 0], \"top\": [0, 1, 0], \"radius\": 1, \"baseRadius\": 1" ),
                 BAD_FILE, "objects[0]: unknown field \"baseRadius\"" );
}

    // The mesh's tree must find exactly the hit testing every face does:
    // rays at the teapot from all around, many grazing its edges.
static void MeshTreeChecks( const char *strScenes )
{
    CScene  oScene;

    Check( oScene.Load( ( std::string( strScenes ) + "/teapot.ray" ).c_str() ) == 0, "the teapot loads" );

    CMesh  *pMesh = dynamic_cast<CMesh *>( oScene.GetObjectAt( 1 ) );

    Check( pMesh != NULL, "the teapot is a mesh" );
    if ( pMesh == NULL )
        return;

    unsigned    nSeed = 12345;
    int         nHits = 0;
    int         nDiffer = 0;

    for ( int nRay = 0; nRay < 2000; nRay++ )
    {
        double adValue[ 6 ];

        for ( int nIndex = 0; nIndex < 6; nIndex++ )
        {
            nSeed = ( nSeed * 1103515245u ) + 12345u;
            adValue[ nIndex ] = ( ( nSeed >> 8 ) & 0xFFFF ) / 65535.0;
        }

            // From a point around the teapot toward a point inside its box.
        double dAngle = adValue[ 0 ] * 6.283185307179586;
        CVector oFrom( 8.0 * cos( dAngle ), ( adValue[ 1 ] * 8.0 ) - 2.0, 8.0 * sin( dAngle ) );
        CVector oTo( ( adValue[ 2 ] * 6.6 ) - 3.1, adValue[ 3 ] * 3.3, ( adValue[ 4 ] * 4.2 ) - 2.1 );
        CVector oDirection = oTo - oFrom;
        CRay    oTree, oEach;
        CVector oTreeHit, oEachHit;

        oDirection.Normalize();
        oTree.Set( oFrom.GetX(), oFrom.GetY(), oFrom.GetZ(), oDirection.GetX(), oDirection.GetY(), oDirection.GetZ(), 1e100, 1.0, 0, 0 );
        oEach = oTree;

        BOOL bTree = pMesh->Intersect( oTree, oTreeHit );
        BOOL bEach = pMesh->IntersectEachFace( oEach, oEachHit );

        if ( bTree )
            nHits++;
        if ( bTree != bEach || oTree.GetT() != oEach.GetT() || oTreeHit.GetX() != oEachHit.GetX() ||
             oTreeHit.GetY() != oEachHit.GetY() || oTreeHit.GetZ() != oEachHit.GetZ() )
            nDiffer++;
    }
    Check( nHits > 500, "many rays at the teapot hit it" );
    Check( nDiffer == 0, "the mesh's tree finds exactly the hit testing every face finds" );
}

static void HdrChecks( void )
{
    BYTE anRgbe[ 4 ];

        // RGBE: a mantissa byte a channel over a shared power of two.
    CImageExport::ToRgbe( 1.0f, 1.0f, 1.0f, anRgbe );
    Check( anRgbe[ 0 ] == 128 && anRgbe[ 1 ] == 128 && anRgbe[ 2 ] == 128 && anRgbe[ 3 ] == 129, "white is RGBE 128 128 128 129" );
    CImageExport::ToRgbe( 12.0f, 3.0f, 0.0f, anRgbe );
    Check( anRgbe[ 0 ] == 192 && anRgbe[ 1 ] == 48 && anRgbe[ 2 ] == 0 && anRgbe[ 3 ] == 132, "a color brighter than white keeps its brightness" );
    CImageExport::ToRgbe( -1.0f, NAN, 0.0f, anRgbe );
    Check( anRgbe[ 0 ] == 0 && anRgbe[ 1 ] == 0 && anRgbe[ 2 ] == 0 && anRgbe[ 3 ] == 0, "negative and NaN channels are black" );

        // The hdr color model scales by its exposure and doesn't clip; its
        // bytes are the sRGB of the scaled color.
    CScene          oScene;
    CHdrColorModel  oModel;
    CColor          oBright( 3.0, 0.5, 0.0 );
    BYTE            nRed, nGreen, nBlue;

    Check( oModel.SetExposure( 2.0 ) && ! oModel.SetExposure( 0.0 ), "the hdr exposure must be positive" );
    CColor oLinear = oModel.ToLinear( oBright );
    Check( oLinear.GetR() == 6.0 && oLinear.GetG() == 1.0 && oLinear.GetB() == 0.0, "the hdr model scales by its exposure and doesn't clip" );
    oModel.ToRGB( oBright, nRed, nGreen, nBlue );
    Check( nRed == 255 && nGreen == 255 && nBlue == 0, "the hdr model's bytes clip the scaled color" );

    Check( oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"colorModel\": { \"type\": \"hdr\", \"exposure\": 0.5 }" ) ) == 0 &&
           strcmp( oScene.GetCamera().GetColorModel().GetName(), "hdr" ) == 0 &&
           oScene.ToJson().find( "\"exposure\": 0.5" ) != std::string::npos, "a scene chooses the hdr color model and its exposure" );
    ExpectError( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"colorModel\": { \"type\": \"hdr\", \"exposure\": -1 }" ),
                 BAD_FILE, "exposure must be a positive number" );

        // The PNG's chunks.
    std::vector<BYTE> anPixels( 4 * 3 * 3, 200 );
    std::vector<BYTE> anPng = CImageExport::EncodePng( 4, 3, anPixels );
    Check( anPng.size() > 33 && memcmp( anPng.data(), "\x89PNG\r\n\x1a\n", 8 ) == 0 && memcmp( anPng.data() + 12, "IHDR", 4 ) == 0 &&
           memcmp( anPng.data() + anPng.size() - 8, "IEND", 4 ) == 0, "a PNG has its signature, header and end" );
}

    // An engine that quits after its first tile, without sending it.
class CQuitter : public CSocketTaskSlave
{
    public:
        virtual BOOL PixelsToMaster( uint32_t nTrace, const STile &oTile, long long nRays, const std::vector<double> &anColors )
        {
            (void)nTrace; (void)oTile; (void)nRays; (void)anColors;
            delete m_pStream;       // drops the connection
            m_pStream = NULL;
            return FALSE;
        }
};

    // Engines that come and go: the image must still be whole, and exactly
    // what threads make.
static void EngineChecks( const char *strScenes, const char *strDir )
{
    std::string strThreads = std::string( strDir ) + "/churn_threads.raw";
    std::string strEngines = std::string( strDir ) + "/churn_engines.raw";
    CScene      oScene;

    Check( oScene.Load( ( std::string( strScenes ) + "/scan_test.ray" ).c_str() ) == 0, "scan_test loads" );
    oScene.GetSettings().SetImageWidth( 96 );
    oScene.GetSettings().SetImageHeight( 72 );

    {
        CThreadTaskMaster   oThreads;
        CImageFile          oImage( strThreads.c_str(), 96, 72, FALSE );

        oThreads.SetTileSize( 16 );
        oThreads.SpawnTasks( 3 );
        Check( oThreads.TraceScene( oScene, oImage ) == TRUE, "threads trace the image" );
        oThreads.EndTasks();
    }

    CSocketTaskMaster oMaster( 0, 30 );

    Check( oMaster.Listen() == TRUE, "the master listens" );

    int         nPort = oMaster.GetPort();
    std::thread oFirst( [ nPort ]()
    {
        CQuitter oQuitter;

        if ( oQuitter.Connect( "127.0.0.1", nPort, 10 ) )
            oQuitter.RunEngine();
    } );

    oMaster.SetTileSize( 16 );
    Check( oMaster.SpawnTasks( 1 ) == TRUE, "the first engine connects" );

        // The second engine comes once the trace is under way.
    std::thread oSecond( [ nPort ]()
    {
        CSocketTaskSlave oEngine;

        std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
        if ( oEngine.Connect( "127.0.0.1", nPort, 10 ) )
            oEngine.RunEngine();
    } );

    {
        CImageFile oImage( strEngines.c_str(), 96, 72, FALSE );

        Check( oMaster.TraceScene( oScene, oImage ) == TRUE, "the trace finishes though its first engine quit" );
    }
    oMaster.EndTasks();
    oFirst.join();
    oSecond.join();

    Check( ReadFile( strEngines ).size() == 4 + 96 * 72 * 3 && ReadFile( strEngines ) == ReadFile( strThreads ),
           "the image is exactly what threads make, with an engine lost and one joining late" );
    remove( strThreads.c_str() );
    remove( strEngines.c_str() );
}

    // The scene's tree must find exactly what testing every object does:
    // rays through the torture test from all around, and shadow rays.
static void SceneTreeChecks( const char *strScenes )
{
    CScene      oScene;
    unsigned    nSeed = 4242;
    int         nHits = 0, nDiffer = 0, nShadowDiffer = 0, nShadowed = 0;

    Check( oScene.Load( ( std::string( strScenes ) + "/torture_test.ray" ).c_str() ) == 0, "the torture test loads" );

    for ( int nRay = 0; nRay < 3000; nRay++ )
    {
        double adValue[ 6 ];

        for ( int nIndex = 0; nIndex < 6; nIndex++ )
        {
            nSeed = ( nSeed * 1103515245u ) + 12345u;
            adValue[ nIndex ] = ( ( nSeed >> 8 ) & 0xFFFF ) / 65535.0;
        }

        CVector oFrom( ( adValue[ 0 ] * 24.0 ) - 12.0, adValue[ 1 ] * 8.0, ( adValue[ 2 ] * 24.0 ) - 12.0 );
        CVector oTo( ( adValue[ 3 ] * 14.0 ) - 7.0, adValue[ 4 ] * 4.0, ( adValue[ 5 ] * 14.0 ) - 6.0 );
        CVector oDirection = oTo - oFrom;
        CRay    oTree, oEach;
        CVector oTreeHit, oEachHit;

        oDirection.Normalize();
        oTree.Set( oFrom.GetX(), oFrom.GetY(), oFrom.GetZ(), oDirection.GetX(), oDirection.GetY(), oDirection.GetZ(), 1e100, 1.0, 0, 0 );
        oEach = oTree;

        CObjects *pTree = oScene.ClosestHit( oTree, oTreeHit, NULL );
        CObjects *pEach = oScene.ClosestHitByEachObject( oEach, oEachHit, NULL );

        if ( pTree )
            nHits++;
        if ( pTree != pEach || oTree.GetT() != oEach.GetT() || oTreeHit.GetX() != oEachHit.GetX() ||
             oTreeHit.GetY() != oEachHit.GetY() || oTreeHit.GetZ() != oEachHit.GetZ() )
            nDiffer++;

            // A shadow ray from the hit toward a light, ignoring the object.
        if ( pEach )
        {
            CVector oLight( 0.0, 9.0, 3.0 );
            CVector oToLight = oLight - oEachHit;
            CRay    oShadowTree, oShadowEach;

            oShadowTree.CreateRay( oToLight, oEachHit, 1.0, -1, -1 );
            oShadowEach = oShadowTree;

            BOOL bTree = oScene.Intersect( oShadowTree, pEach );
            BOOL bEach = oScene.IntersectEachObject( oShadowEach, pEach );

            nShadowed += bEach ? 1 : 0;
            nShadowDiffer += ( bTree != bEach ) ? 1 : 0;
        }
    }
    Check( nHits > 1000 && nShadowed > 50, "many rays through the torture test hit something, and some are shadowed" );
    Check( nDiffer == 0, "the scene's tree finds exactly the closest hit testing every object finds" );
    Check( nShadowDiffer == 0, "the scene's tree finds exactly the shadows testing every object finds" );
}

    // A lens's exit pupil tables go into the scene and come back exactly;
    // tables for another setup are worked out again.
static void LensTableChecks( const char *strScenes )
{
    CScene oScene;

    Check( oScene.Load( ( std::string( strScenes ) + "/lens_test.ray" ).c_str() ) == 0, "lens_test loads" );

    std::string strJson = oScene.ToJson( TRUE );

    Check( strJson.find( "\"exitPupil\"" ) != std::string::npos, "the scene carries the lens's exit pupil tables" );

    CScene oAgain;

    Check( oAgain.FromJson( strJson ) == 0 && oAgain.ToJson( TRUE ) == strJson, "the tables come back exactly" );

        // The tables can't be cut short.
    std::string strReadable = oScene.ToJson();
    size_t nAt = strReadable.find( "\"data\": \"" );
    std::string strDamaged = strReadable;

    if ( nAt != std::string::npos )
        strDamaged.replace( nAt + 9, 8, "" );
    ExpectError( strDamaged, BAD_FILE, "data must be the exit pupil tables, in base64" );

        // Refocused, the lens works out tables of its own.
    size_t      nFocus = strReadable.find( "\"focusDistance\": " );
    std::string strMoved = strReadable;

    if ( nFocus != std::string::npos )
        strMoved.insert( nFocus + 17, "1" );
    CScene oMoved;
    Check( oMoved.FromJson( strMoved ) == 0, "a refocused lens_test loads" );

    std::string strMovedJson = oMoved.ToJson();
    size_t      nKeyA = strReadable.find( "\"key\": \"" ), nKeyB = strMovedJson.find( "\"key\": \"" );

    Check( nKeyA != std::string::npos && nKeyB != std::string::npos &&
           strReadable.substr( nKeyA, 26 ) != strMovedJson.substr( nKeyB, 26 ), "a refocused lens's tables are worked out again" );

    ExpectError( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"sampler\": { \"type\": \"adaptive\", \"min\": 16, \"max\": 8 }" ),
                 BAD_FILE, "max can't be less than min" );
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
    if ( argc >= 4 && strcmp( argv[ 1 ], "--engines" ) == 0 )
        EngineChecks( argv[ 2 ], argv[ 3 ] );
    else if ( argc >= 4 && strcmp( argv[ 1 ], "--textures" ) == 0 )
    {
        TextureChecks( argv[ 2 ] );
        ArchiveChecks( argv[ 2 ], argv[ 3 ] );
        MeshTreeChecks( argv[ 2 ] );
        SceneTreeChecks( argv[ 2 ] );
        LensTableChecks( argv[ 2 ] );
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
        PrimitiveChecks();
        HdrChecks();
    }

    if ( g_nFailures == 0 )
        printf( "All checks passed\n" );
    return ( g_nFailures == 0 ) ? 0 : 1;
}
