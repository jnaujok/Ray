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
#include "SphereLight.h"
#include "RectangleLight.h"
#include "SpotLight.h"
#include "DirectionalLight.h"
#include "Environment.h"
#include "JSON/ObjectNode.h"
#include "Sampling.h"
#include "SceneJson.h"
#include "Scene.h"
#include "ReinhardColorModel.h"
#include "ImageTexture.h"
#include "PngImage.h"
#include "BumpMap.h"
#include "Csg.h"
#include "ModelFile.h"
#include "LayersTexture.h"
#include "FaceColorTexture.h"
#include "Spectrum.h"


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

    // A floor lit by a 2 x 2 rectangle light 4 above it (64 samples), seen
    // by a ray from below the height of strBlocker, if any. The floor is
    // white and diffuse, the background black.
static CColor UnderPanel( const char *strBlocker )
{
    std::string strText =
        "{ \"format\": \"Ray scene\", \"version\": 3, \"background\": [0, 0, 0],\n"
        "  \"camera\": { \"position\": [0, 1, -10], \"lookAt\": [0, 0, 0] },\n"
        "  \"objects\": [ { \"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [0, 1, 0],\n"
        "                   \"materials\": [ { \"type\": \"diffuse\", \"color\": [1, 1, 1] } ] }";
    strText += strBlocker;
    strText += " ],\n"
        "  \"lights\": [ { \"type\": \"rectangleLight\", \"position\": [0, 4, 0], \"color\": [1, 1, 1],\n"
        "                  \"edge1\": [2, 0, 0], \"edge2\": [0, 0, 2], \"samples\": 64 } ] }\n";

    CScene  oScene;
    CRay    oRay;
    CColor  oColor( 0, 0, 0 );

    Check( oScene.FromJson( strText ) == 0, "a scene under a rectangle light loads" );
    oRay.Set( 0.0, 1.0, -3.0, 0.0, -1.0 / sqrt( 10.0 ), 3.0 / sqrt( 10.0 ), 1e100, 1.0, 0, 0 );
    oScene.TraceRay( oRay, oColor, NULL );
    return oColor;
}

static void SoftChecks( void )
{
        // The new fields read, save and read back the same.
    const char *strSoft =
        "{ \"format\": \"Ray scene\", \"version\": 3,\n"
        "  \"camera\": { \"position\": [0, 0, -10], \"lookAt\": [0, 0, 0] },\n"
        "  \"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1, \"materials\": [\n"
        "      { \"type\": \"transparent\", \"indexOfRefraction\": 1.5, \"roughness\": 0.2 },\n"
        "      { \"type\": \"reflective\", \"roughness\": 0.05 } ] } ],\n"
        "  \"lights\": [ { \"type\": \"sphereLight\", \"position\": [5, 5, -5], \"color\": [1, 1, 1], \"radius\": 0.5, \"samples\": 3 },\n"
        "              { \"type\": \"rectangleLight\", \"position\": [0, 5, 0], \"color\": [1, 1, 1],\n"
        "                \"edge1\": [1, 0, 0], \"edge2\": [0, 0, -1] } ] }\n";
    CScene  oScene;

    Check( oScene.FromJson( strSoft ) == 0, "a scene with rough materials and area lights loads" );

    std::string strSaved = oScene.ToJson();
    CScene      oReread;

    Check( oReread.FromJson( strSaved ) == 0 && oReread.ToJson() == strSaved, "rough materials and area lights read back the same" );
    Check( strSaved.find( "\"roughness\": 0.2" ) != std::string::npos && strSaved.find( "\"roughness\": 0.05" ) != std::string::npos,
           "roughness is saved" );
    Check( strSaved.find( "\"sphereLight\"" ) != std::string::npos && strSaved.find( "\"radius\": 0.5" ) != std::string::npos &&
           strSaved.find( "\"samples\": 3" ) != std::string::npos, "a sphere light is saved" );
    Check( strSaved.find( "\"rectangleLight\"" ) != std::string::npos && strSaved.find( "\"edge2\": [0.0, 0.0, -1.0]" ) != std::string::npos &&
           strSaved.find( "\"samples\": 1" ) != std::string::npos, "a rectangle light is saved, with one sample by default" );

        // Smooth materials don't write a roughness, so old scenes save as before.
    CScene oSmooth;

    Check( oSmooth.FromJson( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\" }, "
                                                      "{ \"type\": \"reflective\" } ]" ) ) == 0 &&
           oSmooth.ToJson().find( "roughness" ) == std::string::npos, "smooth materials save no roughness" );

    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"roughness\": 1.5 } ]" ),
                 BAD_FILE, "roughness must be between 0 and 1" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"reflective\", \"roughness\": -0.1 } ]" ),
                 BAD_FILE, "roughness must be between 0 and 1" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"sphereLight\", \"radius\": 0" ), BAD_FILE, "radius must be more than 0" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"sphereLight\", \"samples\": 0" ), BAD_FILE, "samples must be a whole number" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"rectangleLight\", \"edge1\": [1, 0, 0], \"edge2\": [2, 0, 0]" ),
                 BAD_FILE, "edge1 and edge2 must be sides of a rectangle" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"rectangleLight\", \"edge1\": [1, 0, 0]" ), BAD_FILE, "edge2" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"sphereLight\", \"size\": 1" ), BAD_FILE, "size" );

        // GGX: tan theta = alpha sqrt( u / ( 1 - u ) ), so half the
        // microfacets tilt less than atan( alpha ), and a tiny alpha leaves
        // the normal as it is.
    CVector oUp( 0.0, 1.0, 0.0 );
    int     nWithin = 0;
    int     nCount = 1000;

    for ( int nIndex = 0; nIndex < nCount; nIndex++ )
    {
        CVector oMicro = RaySampling::Microfacet( oUp, 0.3, ( nIndex + 0.5 ) / nCount, 0.37 );

        if ( oMicro.Dot( oUp ) >= cos( atan( 0.3 ) ) )
            nWithin++;
    }
    Check( nWithin == nCount / 2, "half of GGX microfacets of roughness 0.3 tilt less than atan( 0.3 )" );

    CVector oFlat = RaySampling::Microfacet( oUp, 1e-9, 0.9, 0.2 );

    Check( ABS( oFlat.Dot( oUp ) - 1.0 ) < 1e-12, "a microfacet of tiny roughness is the normal" );

        // A sphere light's points lie on the disc it shows a viewer: at
        // right angles to the line of sight, within its radius.
    CScene          oBallScene;

    Check( oBallScene.FromJson( Minimal( "\"type\": \"point\"", "\"type\": \"sphereLight\", \"radius\": 0.5" ) ) == 0,
           "a sphere light loads" );

    CLight *pBall = oBallScene.GetLight( 0 );
    bool    bOnDisc = pBall != NULL && pBall->GetSampleCount() == 1;
    CVector oCenter( 5.0, 5.0, -5.0 );
    CVector oViewer( 0.0, 0.0, 0.0 );

    for ( int nIndex = 0; bOnDisc && nIndex < 100; nIndex++ )
    {
        CVector oPoint;

        pBall->SampleAt( oViewer, ( nIndex % 10 + 0.5 ) / 10.0, ( nIndex / 10 + 0.5 ) / 10.0, oPoint );

        CVector oOffset( oPoint - oCenter );
        CVector oSight( oCenter - oViewer );

        oSight.Normalize();
        bOnDisc = ABS( oOffset.Dot( oSight ) ) < 1e-12 && oOffset.Magnitude() <= 0.5 + 1e-12;
    }
    Check( bOnDisc, "a sphere light's points lie on the disc it shows the viewer" );

        // A rectangle light shines only to the side edge1 x edge2 points to.
    CScene oPanelScene;

    Check( oPanelScene.FromJson( Minimal( "\"type\": \"point\"",
                                          "\"type\": \"rectangleLight\", \"edge1\": [1, 0, 0], \"edge2\": [0, 0, 1]" ) ) == 0,
           "a rectangle light loads" );

    CLight *pPanel = oPanelScene.GetLight( 0 );
    CVector oPoint;
    CVector oBelow( 5.0, 0.0, -5.0 );
    CVector oAbove( 5.0, 10.0, -5.0 );

        // [1, 0, 0] x [0, 0, 1] is [0, -1, 0]: it shines down.
    Check( pPanel != NULL && pPanel->SampleAt( oBelow, 0.5, 0.5, oPoint ) && ! pPanel->SampleAt( oAbove, 0.5, 0.5, oPoint ),
           "a rectangle light shines only to its front" );

        // Half the panel hidden, half the light: a blocker above x < 0, its
        // edge straight over the point seen, hides the panel's x < 0 half.
    CColor oOpen = UnderPanel( "" );
    CColor oHalf = UnderPanel( ", { \"type\": \"box\", \"center\": [-5, 2, 0], \"size\": [10, 0.2, 10] }" );

    Check( oOpen.GetR() > 0.5, "the floor under a rectangle light is lit" );
    Check( ABS( oHalf.GetR() / oOpen.GetR() - 0.5 ) < 0.02, "half a rectangle light hidden gives half its light" );

        // The same ray always brings back the same color through rough
        // glass, and rays with other sample points see it differently.
    std::string strFrosted = Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", "
                                      "\"color\": [1, 1, 1], \"indexOfRefraction\": 1.5, \"roughness\": 0.3 } ]" );

    strFrosted.replace( strFrosted.find( "\"objects\": [" ) + 12, 0,
                        "{ \"type\": \"plane\", \"position\": [0, 0, 3], \"normal\": [0, 0, -1], \"materials\": [ { \"type\": \"ambient\", "
                        "\"texture\": { \"type\": \"checker\", \"colors\": [[0, 0, 0], [1, 1, 1]], \"size\": 0.1 } } ] }, " );

    CScene oFrosted;

    Check( oFrosted.FromJson( strFrosted ) == 0, "a frosted ball scene loads" );

    std::vector<double> anSeen;
    bool                bRepeats = true;

    oFrosted.GetSettings().SetImageWidth( 64 );
    oFrosted.GetSettings().SetImageHeight( 64 );
    for ( int nIndex = 0; nIndex < 16; nIndex++ )
    {
        CRay    oFirst;
        CRay    oSecond;
        CColor  oFirstColor;
        CColor  oSecondColor;
        FLT     nAt = 32.0 + ( nIndex + 0.5 ) / 16.0;

        oFirst.CreateRayAt( oFrosted, nAt, 32.3, 0, 0 );
        oSecond.CreateRayAt( oFrosted, nAt, 32.3, 0, 0 );
        oFrosted.TraceRay( oFirst, oFirstColor, NULL );
        oFrosted.TraceRay( oSecond, oSecondColor, NULL );
        bRepeats = bRepeats && oFirstColor.GetR() == oSecondColor.GetR();
        anSeen.push_back( oFirstColor.GetR() );
    }
    std::sort( anSeen.begin(), anSeen.end() );
    Check( bRepeats, "rough glass is the same every time for the same ray" );
    Check( anSeen.back() - anSeen.front() > 0.2, "rough glass scatters a pixel's rays over what is behind it" );
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

// --------------------------------------------------------------------------
// PNG
// --------------------------------------------------------------------------

static uint32_t TestCrc( const std::string &strData )
{
    uint32_t nCrc = 0xFFFFFFFFu;

    for ( size_t nIndex = 0; nIndex < strData.size(); nIndex++ )
    {
        nCrc ^= (BYTE)strData[ nIndex ];
        for ( int nBit = 0; nBit < 8; nBit++ )
            nCrc = ( nCrc & 1u ) ? ( 0xEDB88320u ^ ( nCrc >> 1 ) ) : ( nCrc >> 1 );
    }
    return nCrc ^ 0xFFFFFFFFu;
}

static void PutBig( std::string &strOut, uint32_t nValue )
{
    for ( int nShift = 24; nShift >= 0; nShift -= 8 )
        strOut += (char)( ( nValue >> nShift ) & 0xFF );
}

static std::string PngChunk( const char *strType, const std::string &strData )
{
    std::string strOut;
    std::string strBody = std::string( strType, 4 ) + strData;

    PutBig( strOut, (uint32_t)strData.size() );
    strOut += strBody;
    PutBig( strOut, TestCrc( strBody ) );
    return strOut;
}

    // A PNG of the given kind, each channel's sample from pfnSample (in the
    // image's own bit depth), rows filtered with each filter type in turn.
static std::string MakePng( int nWidth, int nHeight, int nColorType, int nDepth, bool bInterlace, const std::string &strPalette,
                            unsigned ( *pfnSample )( int nX, int nY, int nChannel ) )
{
    static const int anChannels[ 7 ] = { 1, 0, 3, 1, 2, 0, 4 };
    static const int anPass[ 7 ][ 4 ] = { { 0, 0, 8, 8 }, { 4, 0, 8, 8 }, { 0, 4, 4, 8 }, { 2, 0, 4, 4 }, { 0, 2, 2, 4 }, { 1, 0, 2, 2 }, { 0, 1, 1, 2 } };
    int              nChannels = anChannels[ nColorType ];
    size_t           nStep = (size_t)( std::max )( 1, nChannels * nDepth / 8 );
    std::vector<BYTE> anRaw;

    for ( int nPass = 0; nPass < ( bInterlace ? 7 : 1 ); nPass++ )
    {
        int nX0 = bInterlace ? anPass[ nPass ][ 0 ] : 0, nY0 = bInterlace ? anPass[ nPass ][ 1 ] : 0;
        int nDX = bInterlace ? anPass[ nPass ][ 2 ] : 1, nDY = bInterlace ? anPass[ nPass ][ 3 ] : 1;
        std::vector<BYTE> anPrevious;
        int nRow = 0;

        for ( int nY = nY0; nY < nHeight; nY += nDY, nRow++ )
        {
            std::vector<BYTE> anRow;
            int               nBits = 0;

            for ( int nX = nX0; nX < nWidth; nX += nDX )
            {
                for ( int nChannel = 0; nChannel < nChannels; nChannel++ )
                {
                    unsigned nValue = pfnSample( nX, nY, nChannel );

                    if ( nDepth == 16 )
                    {
                        anRow.push_back( (BYTE)( nValue >> 8 ) );
                        anRow.push_back( (BYTE)nValue );
                    }
                    else if ( nDepth == 8 )
                        anRow.push_back( (BYTE)nValue );
                    else
                    {
                        if ( nBits % 8 == 0 )
                            anRow.push_back( 0 );
                        anRow.back() |= (BYTE)( nValue << ( 8 - nDepth - ( nBits % 8 ) ) );
                        nBits += nDepth;
                    }
                }
            }
            if ( anRow.empty() )
                break;
            if ( anPrevious.empty() )
                anPrevious.assign( anRow.size(), 0 );

            int nFilter = nRow % 5;

            anRaw.push_back( (BYTE)nFilter );
            for ( size_t nIndex = 0; nIndex < anRow.size(); nIndex++ )
            {
                int nLeft = ( nIndex >= nStep ) ? anRow[ nIndex - nStep ] : 0;
                int nUp = anPrevious[ nIndex ];
                int nUpLeft = ( nIndex >= nStep ) ? anPrevious[ nIndex - nStep ] : 0;
                int nPredict = 0;

                if ( nFilter == 1 )
                    nPredict = nLeft;
                else if ( nFilter == 2 )
                    nPredict = nUp;
                else if ( nFilter == 3 )
                    nPredict = ( nLeft + nUp ) / 2;
                else if ( nFilter == 4 )
                {
                    int nGuess = nLeft + nUp - nUpLeft;
                    int nA = abs( nGuess - nLeft ), nB = abs( nGuess - nUp ), nC = abs( nGuess - nUpLeft );

                    nPredict = ( nA <= nB && nA <= nC ) ? nLeft : ( nB <= nC ) ? nUp : nUpLeft;
                }
                anRaw.push_back( (BYTE)( anRow[ nIndex ] - nPredict ) );
            }
            anPrevious = anRow;
        }
    }

    std::string strHeader;
    std::vector<BYTE> anCompressed = CImageExport::Deflate( anRaw );

    PutBig( strHeader, (uint32_t)nWidth );
    PutBig( strHeader, (uint32_t)nHeight );
    strHeader += (char)nDepth;
    strHeader += (char)nColorType;
    strHeader += (char)0;
    strHeader += (char)0;
    strHeader += (char)( bInterlace ? 1 : 0 );

    std::string strPng( "\x89PNG\r\n\x1a\n", 8 );

    strPng += PngChunk( "IHDR", strHeader );
    if ( ! strPalette.empty() )
        strPng += PngChunk( "PLTE", strPalette );
    strPng += PngChunk( "IDAT", std::string( anCompressed.begin(), anCompressed.end() ) );
    strPng += PngChunk( "IEND", "" );
    return strPng;
}

static unsigned SampleRgb8( int nX, int nY, int nChannel ) { return (unsigned)( ( nX * 37 + nY * 101 + nChannel * 59 ) & 0xFF ); }
static unsigned SampleGray1( int nX, int nY, int ) { return (unsigned)( ( nX + nY * 3 ) % 2 ); }
static unsigned SampleGray2( int nX, int nY, int ) { return (unsigned)( ( nX + nY ) % 4 ); }
static unsigned SampleGray4( int nX, int nY, int ) { return (unsigned)( ( nX * 3 + nY ) % 16 ); }
static unsigned SampleGray16( int nX, int nY, int ) { return 257u * (unsigned)( ( nX * 29 + nY * 13 ) & 0xFF ); }
static unsigned SampleGrayAlpha( int nX, int nY, int nChannel ) { return nChannel ? 0x40u : (unsigned)( ( nX * 11 + nY * 7 ) & 0xFF ); }
static unsigned SampleRgba16( int nX, int nY, int nChannel ) { return ( nChannel == 3 ) ? 0x1234u : 257u * SampleRgb8( nX, nY, nChannel ); }
static unsigned SamplePalette4( int nX, int nY, int ) { return (unsigned)( ( nX + 2 * nY ) % 5 ); }
static unsigned SamplePalette8( int nX, int nY, int ) { return (unsigned)( ( nX * 7 + nY ) % 5 ); }
static unsigned SamplePastPalette( int nX, int, int ) { return (unsigned)( nX == 2 ? 9 : 1 ); }

static bool Decodes( const std::string &strPng, int nWidth, int nHeight, BYTE ( *pfnExpected )( int nX, int nY, int nChannel ) )
{
    int               nReadWidth = 0;
    int               nReadHeight = 0;
    std::vector<BYTE> anPixels;
    std::string       strError;

    if ( ! CPngImage::Decode( strPng, nReadWidth, nReadHeight, anPixels, strError ) )
    {
        fprintf( stderr, "PNG decode failed: %s\n", strError.c_str() );
        return false;
    }
    if ( nReadWidth != nWidth || nReadHeight != nHeight )
        return false;
    for ( int nY = 0; nY < nHeight; nY++ )
        for ( int nX = 0; nX < nWidth; nX++ )
            for ( int nChannel = 0; nChannel < 3; nChannel++ )
                if ( anPixels[ ( (size_t)nY * nWidth + nX ) * 3 + nChannel ] != pfnExpected( nX, nY, nChannel ) )
                    return false;
    return true;
}

static const BYTE s_anPalette[ 15 ] = { 255, 0, 0, 0, 255, 0, 0, 0, 255, 10, 20, 30, 200, 100, 50 };

static BYTE ExpectRgb8( int nX, int nY, int nChannel ) { return (BYTE)SampleRgb8( nX, nY, nChannel ); }
static BYTE ExpectGray1( int nX, int nY, int ) { return SampleGray1( nX, nY, 0 ) ? 255 : 0; }
static BYTE ExpectGray2( int nX, int nY, int ) { return (BYTE)( SampleGray2( nX, nY, 0 ) * 255 / 3 ); }
static BYTE ExpectGray4( int nX, int nY, int ) { return (BYTE)( SampleGray4( nX, nY, 0 ) * 17 ); }
static BYTE ExpectGray16( int nX, int nY, int ) { return (BYTE)( SampleGray16( nX, nY, 0 ) / 257 ); }
static BYTE ExpectGrayAlpha( int nX, int nY, int ) { return (BYTE)SampleGrayAlpha( nX, nY, 0 ); }
static BYTE ExpectPalette4( int nX, int nY, int nChannel ) { return s_anPalette[ SamplePalette4( nX, nY, 0 ) * 3 + nChannel ]; }
static BYTE ExpectPalette8( int nX, int nY, int nChannel ) { return s_anPalette[ SamplePalette8( nX, nY, 0 ) * 3 + nChannel ]; }

static void PngChecks( void )
{
    std::string strPalette( (const char *)s_anPalette, sizeof( s_anPalette ) );

    Check( Decodes( MakePng( 7, 6, 2, 8, false, "", SampleRgb8 ), 7, 6, ExpectRgb8 ), "an 8-bit RGB PNG reads exactly, every filter type" );
    Check( Decodes( MakePng( 11, 5, 0, 1, false, "", SampleGray1 ), 11, 5, ExpectGray1 ), "a 1-bit gray PNG reads as black and white" );
    Check( Decodes( MakePng( 9, 4, 0, 2, false, "", SampleGray2 ), 9, 4, ExpectGray2 ), "a 2-bit gray PNG reads scaled to 0-255" );
    Check( Decodes( MakePng( 5, 5, 0, 4, false, "", SampleGray4 ), 5, 5, ExpectGray4 ), "a 4-bit gray PNG reads scaled to 0-255" );
    Check( Decodes( MakePng( 6, 3, 0, 16, false, "", SampleGray16 ), 6, 3, ExpectGray16 ), "a 16-bit gray PNG reads rounded to 8 bits" );
    Check( Decodes( MakePng( 4, 4, 4, 8, false, "", SampleGrayAlpha ), 4, 4, ExpectGrayAlpha ), "gray with alpha reads its gray, alpha dropped" );
    Check( Decodes( MakePng( 5, 3, 6, 16, false, "", SampleRgba16 ), 5, 3, ExpectRgb8 ), "16-bit RGBA reads its color, alpha dropped" );
    Check( Decodes( MakePng( 7, 3, 3, 4, false, strPalette, SamplePalette4 ), 7, 3, ExpectPalette4 ), "a 4-bit palette PNG reads its colors" );
    Check( Decodes( MakePng( 6, 6, 3, 8, false, strPalette, SamplePalette8 ), 6, 6, ExpectPalette8 ), "an 8-bit palette PNG reads its colors" );
    Check( Decodes( MakePng( 13, 11, 2, 8, true, "", SampleRgb8 ), 13, 11, ExpectRgb8 ), "an interlaced (Adam7) PNG reads the same as a plain one" );
    Check( Decodes( MakePng( 3, 2, 0, 1, true, "", SampleGray1 ), 3, 2, ExpectGray1 ), "a tiny interlaced 1-bit PNG, with empty passes, reads" );

        // Damage is reported, never read past.
    std::string strGood = MakePng( 7, 6, 2, 8, false, "", SampleRgb8 );
    int         nW, nH;
    std::vector<BYTE> anPixels;
    std::string strError;

    std::string strBadCrc = strGood;
    strBadCrc[ 20 ] ^= 1;      // inside IHDR
    Check( ! CPngImage::Decode( strBadCrc, nW, nH, anPixels, strError ) && strError.find( "CRC" ) != std::string::npos,
           "a damaged chunk is caught by its CRC" );
    bool bTruncations = true;
    for ( size_t nLength = 0; nLength < strGood.size(); nLength += 3 )
        if ( CPngImage::Decode( strGood.substr( 0, nLength ), nW, nH, anPixels, strError ) )
            bTruncations = false;
    Check( bTruncations, "every truncated PNG is refused" );
    Check( ! CPngImage::Decode( "not a png at all", nW, nH, anPixels, strError ), "a file without the signature is refused" );
    Check( ! CPngImage::Decode( MakePng( 4, 1, 3, 8, false, std::string( (const char *)s_anPalette, 6 ), SamplePastPalette ), nW, nH, anPixels, strError ) &&
           strError.find( "palette" ) != std::string::npos, "a palette index past the palette is refused" );
    Check( ! CPngImage::Decode( MakePng( 4, 1, 3, 8, false, "", SamplePalette8 ), nW, nH, anPixels, strError ),
           "a palette PNG without a palette is refused" );
    {
        std::string strCritical = strGood;
        size_t      nIdat = strCritical.find( "IDAT" ) - 4;

        strCritical.insert( nIdat, PngChunk( "ZZZZ", "x" ) );
        Check( ! CPngImage::Decode( strCritical, nW, nH, anPixels, strError ) && strError.find( "ZZZZ" ) != std::string::npos,
               "an unknown critical chunk is refused" );

        std::string strAncillary = strGood;
        strAncillary.insert( nIdat, PngChunk( "tEXt", "Comment\0hi" ) );
        Check( Decodes( strAncillary, 7, 6, ExpectRgb8 ), "an unknown ancillary chunk is skipped" );
    }

        // Inflate: a stored block, and a dynamic Huffman block (from zlib).
    {
        std::vector<BYTE> anStored = { 0x78, 0x01, 0x01, 0x03, 0x00, 0xFC, 0xFF, 'a', 'b', 'c', 0x02, 0x4D, 0x01, 0x27 };
        std::vector<BYTE> anOut;

        Check( CPngImage::Inflate( anStored.data(), anStored.size(), anOut, 100, strError ) && anOut.size() == 3 && anOut[ 0 ] == 'a' && anOut[ 2 ] == 'c',
               "a stored block inflates" );
        anStored.back() ^= 0xFF;
        Check( ! CPngImage::Inflate( anStored.data(), anStored.size(), anOut, 100, strError ) && strError.find( "checksum" ) != std::string::npos,
               "a wrong zlib checksum is caught" );

        static const BYTE anDynamic[] = {
            0x78, 0xDA, 0xED, 0x8F, 0x59, 0x0E, 0x83, 0x30, 0x0C, 0x05, 0xAF, 0xF2, 0xAE, 0xE6, 0x2C, 0x4D, 0x10, 0x26, 0x96, 0xEC,
            0x54, 0x88, 0xDB, 0x77, 0x09, 0xD0, 0xC0, 0x15, 0x9A, 0xFF, 0xD1, 0x68, 0x46, 0x69, 0xC3, 0x83, 0x98, 0x0D, 0x55, 0xC9,
            0xC7, 0x00, 0xAB, 0x52, 0x22, 0x5C, 0x2C, 0xC1, 0xB0, 0x8A, 0x04, 0x24, 0x26, 0x33, 0x38, 0x9D, 0xFC, 0x0C, 0x2A, 0xE1,
            0xE0, 0xA4, 0x80, 0xA7, 0x94, 0x6B, 0x83, 0x6A, 0x56, 0x79, 0xA6, 0x8C, 0x85, 0xD4, 0x71, 0xFC, 0x72, 0x7A, 0x9A, 0x1B,
            0xD7, 0x8B, 0x77, 0xAC, 0x57, 0x5F, 0x22, 0xDE, 0xF2, 0xAE, 0xE0, 0x90, 0xFF, 0x1A, 0x3E, 0xEE, 0x33, 0xA0, 0x89, 0x6F,
            0x05, 0x4D, 0xAD, 0xE3, 0x6E, 0xDC, 0x8D, 0xBB, 0x3F, 0xBF, 0x7B, 0x01, 0x8A, 0x9B, 0xCB, 0x24 };
        static const char *const astrWords[] = { "ray", "traced", "light", "bends", "through", "glass", "and", "falls", "on",
                                                 "stone", "wood", "marble", "brick" };
        std::string strExpected;

        for ( int nIndex = 0; nIndex < 220; nIndex++ )
        {
            if ( nIndex > 0 )
                strExpected += ' ';
            strExpected += astrWords[ ( nIndex * 7 + nIndex / 3 ) % 13 ];
        }
        Check( CPngImage::Inflate( anDynamic, sizeof( anDynamic ), anOut, 100000, strError ) &&
               std::string( anOut.begin(), anOut.end() ) == strExpected, "a dynamic Huffman block (from zlib) inflates exactly" );
        Check( ! CPngImage::Inflate( anDynamic, sizeof( anDynamic ), anOut, 100, strError ), "inflating past the limit is refused" );

        bool bDamage = true;
        for ( size_t nByte = 2; nByte < sizeof( anDynamic ) - 4; nByte++ )
        {
            std::vector<BYTE> anDamaged( anDynamic, anDynamic + sizeof( anDynamic ) );

            anDamaged[ nByte ] ^= 0x5A;
            if ( CPngImage::Inflate( anDamaged.data(), anDamaged.size(), anOut, 100000, strError ) &&
                 std::string( anOut.begin(), anOut.end() ) != strExpected )
                bDamage = false;
        }
        Check( bDamage, "damaged compressed data is refused (by its codes or its checksum)" );
    }

        // An image texture reads a PNG, and a scene copy carries its pixels.
    CImageTexture::SImage oImage;
    Check( CImageTexture::ParseImage( strGood, "test.png", oImage, strError ) && oImage.nWidth == 7 && oImage.anPixels[ 0 ] == ExpectRgb8( 0, 0, 0 ),
           "an image texture reads a PNG" );
    Check( ! CImageTexture::ParseImage( strBadCrc, "bad.png", oImage, strError ) && strError.find( "\"bad.png\"" ) != std::string::npos,
           "a damaged PNG texture is reported by name" );
}

    // The brick PNGs in scenes/textures: each kind reads, and the normal map
    // (interlaced) holds unit normals pointing out of the surface.
static void BrickTextureChecks( const char *strScenes )
{
    const char *astrFiles[] = { "bricks.png", "bricks_normal.png", "bricks_height.png" };

    for ( int nFile = 0; nFile < 3; nFile++ )
    {
        std::string       strData = ReadFile( std::string( strScenes ) + "/textures/" + astrFiles[ nFile ] );
        int               nWidth = 0, nHeight = 0;
        std::vector<BYTE> anPixels;
        std::string       strError;

        Check( CPngImage::Decode( strData, nWidth, nHeight, anPixels, strError ) && nWidth == 256 && nHeight == 256,
               "a brick texture PNG reads" );
        if ( nFile == 1 && anPixels.size() == 256 * 256 * 3 )
        {
            bool bUnit = true;

            for ( size_t nPixel = 0; nPixel < anPixels.size(); nPixel += 3 )
            {
                double dX = anPixels[ nPixel ] / 127.5 - 1.0, dY = anPixels[ nPixel + 1 ] / 127.5 - 1.0, dZ = anPixels[ nPixel + 2 ] / 127.5 - 1.0;
                double dLength = sqrt( dX * dX + dY * dY + dZ * dZ );

                if ( ABS( dLength - 1.0 ) > 0.03 || dZ <= 0.0 )
                    bUnit = false;
            }
            Check( bUnit, "the interlaced normal map reads as unit normals facing out" );
        }
    }
}

// --------------------------------------------------------------------------
// Bump and normal maps
// --------------------------------------------------------------------------

    // A plane at z = 0 facing -z (u along +x, v along +y) with a bump map.
static std::string PlaneWithBump( const char *strBump )
{
    std::string strTo = std::string( "\"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [0, 0, -1], \"bump\": " ) + strBump;

    return Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1", strTo.c_str() );
}

    // The bumped normal of a scene's first object at a point, for a ray in a
    // direction.
static CVector BumpedNormal( CScene &oScene, CVector oPoint, CVector oDirection, CVector oSmooth )
{
    CObjects *pObject = oScene.GetObjectAt( 0 );
    CRay      oRay;
    CVector   oBack( oDirection * 5.0 );
    CVector   oFrom( oPoint - oBack );
    CVector   oNormal( oSmooth );

    oRay.CreateRay( oDirection, oFrom, 1.0, -1, -1 );
    if ( pObject && pObject->GetBump() )
        pObject->GetBump()->Perturb( *pObject, oPoint, oNormal, oRay );
    return oNormal;
}

static bool Near( CVector oA, CVector oB, double dTolerance )
{
    CVector oDifference = oA - oB;

    return oDifference.Magnitude() < dTolerance;
}

static void BumpChecks( void )
{
    CScene  oScene;

        // A height ramp: a 2 x 1 image, black then white, stretched, so the
        // height rises 2 a unit along +x between the pixel centers. With a
        // depth of 0.1 the slope is 0.2, and the normal tilts down it, to -x.
    Check( oScene.FromJson( PlaneWithBump( "{ \"texture\": { \"type\": \"image\", \"width\": 2, \"height\": 1, \"pixels\": \"AAAA////\", "
                                          "\"repeat\": false }, \"depth\": 0.1 }" ) ) == 0, "a plane with a height map loads" );
    {
        CVector oExpected( -0.2, 0.0, -1.0 );

        oExpected.Normalize();
        Check( Near( BumpedNormal( oScene, CVector( 0.5, 0.3, 0.0 ), CVector( 0, 0, 1 ), CVector( 0, 0, -1 ) ), oExpected, 1e-6 ),
               "a height map tilts the normal down its slope, by depth times the slope" );
        Check( Near( BumpedNormal( oScene, CVector( 0.9, 0.3, 0.0 ), CVector( 0, 0, 1 ), CVector( 0, 0, -1 ) ), CVector( 0, 0, -1 ), 1e-9 ),
               "where the heights are flat, the normal is as it was" );
    }

        // A normal map: a flat color (0.75, 0.5, 1) is (0.5, 0, 1) in tangent
        // space, half a unit toward +u (here +x) for each unit out.
    Check( oScene.FromJson( PlaneWithBump( "{ \"mode\": \"normal\", \"texture\": { \"type\": \"checker\", "
                                          "\"colors\": [[0.75, 0.5, 1], [0.75, 0.5, 1]] } }" ) ) == 0, "a plane with a normal map loads" );
    {
        CVector oExpected( 0.5, 0.0, -1.0 );

        oExpected.Normalize();
        Check( Near( BumpedNormal( oScene, CVector( 0.3, 0.2, 0.0 ), CVector( 0, 0, 1 ), CVector( 0, 0, -1 ) ), oExpected, 1e-6 ),
               "a normal map tilts the normal toward the surface's u direction by its red" );

        CVector oGrazing( 1.0, 0.0, 0.2 );
        oGrazing.Normalize();
        Check( Near( BumpedNormal( oScene, CVector( 0.3, 0.2, 0.0 ), oGrazing, CVector( 0, 0, -1 ) ), CVector( 0, 0, -1 ), 1e-9 ),
               "a tilted normal that would face away from the ray is passed over" );
    }
    Check( oScene.FromJson( PlaneWithBump( "{ \"mode\": \"normal\", \"strength\": 2, \"texture\": { \"type\": \"checker\", "
                                          "\"colors\": [[0.5, 0.75, 1], [0.5, 0.75, 1]] } }" ) ) == 0, "a normal map with a strength loads" );
    {
        CVector oExpected( 0.0, 1.0, -1.0 );

        oExpected.Normalize();
        Check( Near( BumpedNormal( oScene, CVector( 0.3, 0.2, 0.0 ), CVector( 0, 0, 1 ), CVector( 0, 0, -1 ) ), oExpected, 1e-6 ),
               "green tilts toward v, scaled by strength" );
    }

        // On a sphere the u and v directions are found from its coordinates:
        // a flat normal map leaves the normal as it was.
    Check( oScene.FromJson( Minimal( "\"radius\": 1", "\"radius\": 1, \"bump\": { \"mode\": \"normal\", \"texture\": { \"type\": \"checker\", "
                                     "\"colors\": [[0.5, 0.5, 1], [0.5, 0.5, 1]] } }" ) ) == 0, "a sphere with a normal map loads" );
    {
        CVector oPoint( 0.6, 0.0, -0.8 );

        Check( Near( BumpedNormal( oScene, oPoint, CVector( 0, 0, 1 ), oPoint ), oPoint, 1e-6 ), "a flat normal map changes nothing" );
    }

        // Saved and read back; every object type takes one; a mesh's faces share it.
    const char *astrObjects[] = {
        "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1",
        "\"type\": \"box\", \"center\": [0, 0, 0], \"size\": 1",
        "\"type\": \"cone\", \"base\": [0, 0, 0], \"top\": [0, 1, 0], \"baseRadius\": 1",
        "\"type\": \"cylinder\", \"base\": [0, 0, 0], \"top\": [0, 1, 0], \"radius\": 1",
        "\"type\": \"triangle\", \"corners\": [[0, 0, 0], [1, 0, 0], [0, 1, 0]]",
        "\"type\": \"mesh\", \"vertices\": [[0, 0, 0], [1, 0, 0], [0, 1, 0]], \"faces\": [[0, 1, 2]]",
        "\"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [0, 1, 0]" };

    for ( int nIndex = 0; nIndex < 7; nIndex++ )
    {
        std::string strTo = std::string( astrObjects[ nIndex ] ) +
                            ", \"bump\": { \"texture\": { \"type\": \"noise\", \"seed\": 2 }, \"depth\": 0.02, \"step\": 0.002 }";
        CScene      oBumped;

        Check( oBumped.FromJson( Minimal( "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1", strTo.c_str() ) ) == 0,
               "an object with a bump map loads" );

        std::string strSaved = oBumped.ToJson();
        CScene      oAgain;

        Check( strSaved.find( "\"bump\"" ) != std::string::npos && strSaved.find( "\"depth\": 0.02" ) != std::string::npos &&
               strSaved.find( "\"step\": 0.002" ) != std::string::npos, "a bump map is saved" );
        Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "a bump map reads back as it was saved" );

        CMesh *pMesh = dynamic_cast<CMesh *>( oBumped.GetObjectAt( 0 ) );
        if ( pMesh )
            Check( pMesh->GetFaceCount() == 1 && pMesh->FaceHasBump( 0 ), "a mesh's faces share its bump map" );
    }
    Check( oScene.FromJson( s_strMinimal ) == 0 && oScene.ToJson().find( "\"bump\"" ) == std::string::npos,
           "an object without a bump map doesn't write one" );

    ExpectError( PlaneWithBump( "{ \"texture\": { \"type\": \"noise\" }, \"mode\": \"bumpy\" }" ), BAD_FILE,
                 "objects[0].bump.mode: must be \"height\" or \"normal\"" );
    ExpectError( PlaneWithBump( "{ \"mode\": \"normal\", \"depth\": 1, \"texture\": { \"type\": \"noise\" } }" ), BAD_FILE,
                 "objects[0].bump.depth: is for a height map" );
    ExpectError( PlaneWithBump( "{ \"strength\": 1, \"texture\": { \"type\": \"noise\" } }" ), BAD_FILE,
                 "objects[0].bump.strength: is for a normal map" );
    ExpectError( PlaneWithBump( "{ \"depth\": 0.1 }" ), BAD_FILE, "objects[0].bump: needs a \"texture\"" );
    ExpectError( PlaneWithBump( "{ \"step\": 0, \"texture\": { \"type\": \"noise\" } }" ), BAD_FILE, "objects[0].bump.step: must be more than 0" );
    ExpectError( PlaneWithBump( "{ \"texture\": { \"type\": \"plaid\" } }" ), BAD_FILE, "objects[0].bump.texture: unknown texture type \"plaid\"" );
    ExpectError( PlaneWithBump( "{ \"texture\": { \"type\": \"noise\" }, \"height\": 1 }" ), BAD_FILE, "height" );
    ExpectError( PlaneWithBump( "3" ), BAD_FILE, "objects[0].bump: must be an object" );
}

// --------------------------------------------------------------------------
// Spot and directional lights, and the environment
// --------------------------------------------------------------------------

    // The color a ray from oFrom along oDirection brings back.
static CColor TraceFrom( CScene &oScene, CVector oFrom, CVector oDirection )
{
    CRay    oRay;
    CColor  oColor( 0, 0, 0 );

    oDirection.Normalize();
    oRay.Set( oFrom.GetX(), oFrom.GetY(), oFrom.GetZ(), oDirection.GetX(), oDirection.GetY(), oDirection.GetZ(), 1e100, 1.0, 0, 0 );
    oScene.TraceRay( oRay, oColor, NULL );
    return oColor;
}

    // A scene of a white diffuse floor, under strLights, with strExtra objects.
static std::string FloorUnder( const char *strLights, const char *strExtra = "", const char *strEnvironment = "" )
{
    std::string strText =
        "{ \"format\": \"Ray scene\", \"version\": 3, \"background\": [0, 0, 0],\n";

    strText += strEnvironment;
    strText +=
        "  \"camera\": { \"position\": [0, 1, -10], \"lookAt\": [0, 0, 0] },\n"
        "  \"objects\": [ { \"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [0, 1, 0],\n"
        "                   \"materials\": [ { \"type\": \"diffuse\", \"color\": [1, 1, 1] } ] }";
    strText += strExtra;
    strText += " ],\n  \"lights\": [ ";
    strText += strLights;
    strText += " ] }\n";
    return strText;
}

    // The light reaching the floor at (dX, 0, dZ), seen from straight above.
static double FloorAt( CScene &oScene, double dX, double dZ )
{
    return TraceFrom( oScene, CVector( dX, 0.5, dZ ), CVector( 0.0, -1.0, 0.0 ) ).GetR();
}

static void LightChecks( void )
{
        // A spot light lights its cone, fully inside the fading edge and not
        // at all outside it, along a smoothstep between.
    CScene  oSpotScene;

    Check( oSpotScene.FromJson( FloorUnder( "{ \"type\": \"spot\", \"position\": [0, 10, 0], \"direction\": [0, -1, 0], "
                                            "\"color\": [1, 1, 1], \"angle\": 30, \"falloff\": 10 }" ) ) == 0,
           "a scene with a spot light loads" );

    CSpotLight *pSpot = dynamic_cast<CSpotLight *>( oSpotScene.GetLight( 0 ) );

    Check( pSpot != NULL && pSpot->GetSampleCount() == 0, "a spot light without a radius is a point" );
    if ( pSpot )
    {
        bool bEdge = true;

        for ( int nDegrees = 0; nDegrees <= 40; nDegrees++ )
        {
            double  dAngle = nDegrees * PI / 180.0;
            CVector oToLight( -sin( dAngle ), cos( dAngle ), 0.0 );     // from the lit point up to the light
            double  dFactor = pSpot->ConeFactor( oToLight );
            double  dExpected;

            if ( nDegrees <= 20 )
                dExpected = 1.0;
            else if ( nDegrees >= 30 )
                dExpected = 0.0;
            else
            {
                double dT = ( cos( dAngle ) - cos( 30.0 * PI / 180.0 ) ) / ( cos( 20.0 * PI / 180.0 ) - cos( 30.0 * PI / 180.0 ) );

                dExpected = dT * dT * ( 3.0 - 2.0 * dT );
            }
            bEdge = bEdge && ABS( dFactor - dExpected ) < 1e-12;
        }
        Check( bEdge, "a spot light's cone is full inside its falloff, empty outside it, and a smoothstep between" );
    }

        // On the floor: 10 up, a 30 degree cone reaches tan( 30 ) x 10 = 5.77.
    double dCenter = FloorAt( oSpotScene, 0.0, 0.0 );
    double dEdge = FloorAt( oSpotScene, 10.0 * tan( 25.0 * PI / 180.0 ), 0.0 );
    double dOutside = FloorAt( oSpotScene, 6.0, 0.0 );

    Check( ABS( dCenter - 1.0 ) < 1e-9, "a spot light lights the middle of its pool fully" );
    Check( dEdge > 0.05 && dEdge < 0.95, "a spot light's pool fades at its edge" );
    Check( dOutside == 0.0, "a spot light leaves the floor outside its cone dark" );

        // With a radius, a spot light is a disc with soft shadows: a post's
        // shadow has a penumbra.
    CScene oSoftSpot;

    Check( oSoftSpot.FromJson( FloorUnder( "{ \"type\": \"spot\", \"position\": [0, 10, 0], \"direction\": [0, -1, 0], "
                                           "\"color\": [1, 1, 1], \"angle\": 40, \"falloff\": 0, \"radius\": 1, \"samples\": 64 }",
                                           ", { \"type\": \"box\", \"center\": [-5, 5, 0], \"size\": [10, 0.2, 10] }" ) ) == 0,
           "a spot light with a radius loads" );
    Check( oSoftSpot.GetLight( 0 ) && oSoftSpot.GetLight( 0 )->GetSampleCount() == 64, "a spot light with a radius is sampled" );

    double dHalf = FloorAt( oSoftSpot, 0.0, 0.0 );

    Check( ABS( dHalf - 0.5 ) < 0.05, "half a spot light's disc hidden gives about half its light" );

        // The new lights read, save and read back the same; a spot light
        // without a radius writes none.
    const char *strLights =
        "{ \"type\": \"spot\", \"position\": [1, 2, 3], \"direction\": [0, -2, 0], \"color\": [1, 0.5, 0.25], \"angle\": 25, \"falloff\": 5 },\n"
        "{ \"type\": \"spot\", \"position\": [1, 2, 3], \"direction\": [0, -1, 0], \"color\": [1, 1, 1], \"radius\": 0.5, \"samples\": 3 },\n"
        "{ \"type\": \"directional\", \"direction\": [1, -1, 0], \"color\": [0.9, 0.9, 0.8] },\n"
        "{ \"type\": \"directional\", \"direction\": [0, -1, 1], \"color\": [1, 1, 1], \"angle\": 0.53, \"samples\": 4 }";
    CScene      oLightScene;

    Check( oLightScene.FromJson( FloorUnder( strLights ) ) == 0, "a scene with spot and directional lights loads" );

    std::string strSaved = oLightScene.ToJson();
    CScene      oReread;

    Check( oReread.FromJson( strSaved ) == 0 && oReread.ToJson() == strSaved, "spot and directional lights read back the same" );
    Check( strSaved.find( "\"spot\"" ) != std::string::npos && strSaved.find( "\"falloff\": 5.0" ) != std::string::npos &&
           strSaved.find( "\"angle\": 25.0" ) != std::string::npos && strSaved.find( "\"angle\": 30.0" ) != std::string::npos,
           "a spot light saves its angle and falloff, with the defaults" );
    Check( strSaved.find( "\"radius\": 0.5" ) != std::string::npos && strSaved.find( "\"samples\": 3" ) != std::string::npos,
           "a spot light with a radius saves it" );
    Check( strSaved.find( "\"directional\"" ) != std::string::npos && strSaved.find( "\"angle\": 0.53" ) != std::string::npos &&
           strSaved.find( "\"samples\": 4" ) != std::string::npos, "a directional light saves its angle and samples" );

    CScene oPlainSpot;

    Check( oPlainSpot.FromJson( FloorUnder( "{ \"type\": \"spot\", \"position\": [0, 1, 0], \"direction\": [0, -1, 0], \"color\": [1, 1, 1] }" ) ) == 0 &&
           oPlainSpot.ToJson().find( "\"radius\"" ) == std::string::npos && oPlainSpot.ToJson().find( "\"samples\"" ) == std::string::npos,
           "a spot light without a radius saves no radius or samples" );

    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"spot\", \"direction\": [0, 0, 0]" ), BAD_FILE, "direction must not be [0, 0, 0]" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"spot\"" ), BAD_FILE, "direction" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"spot\", \"direction\": [0, -1, 0], \"angle\": 90" ), BAD_FILE,
                 "angle must be more than 0 and less than 90 degrees" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"spot\", \"direction\": [0, -1, 0], \"angle\": 20, \"falloff\": 25" ), BAD_FILE,
                 "falloff must be from 0 to the angle" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"spot\", \"direction\": [0, -1, 0], \"radius\": -1" ), BAD_FILE,
                 "radius must be 0 or more" );
    ExpectError( Minimal( "\"type\": \"point\", \"position\": [5, 5, -5]", "\"type\": \"directional\", \"direction\": [0, 0, 0]" ), BAD_FILE,
                 "direction must not be [0, 0, 0]" );
    ExpectError( Minimal( "\"type\": \"point\", \"position\": [5, 5, -5]", "\"type\": \"directional\", \"direction\": [0, -1, 0], \"angle\": 90" ),
                 BAD_FILE, "angle must be from 0 up to 90 degrees" );
    ExpectError( Minimal( "\"type\": \"point\"", "\"type\": \"directional\", \"direction\": [0, -1, 0]" ), BAD_FILE, "position" );

        // Sunlight: every point gets the same light, from the same direction,
        // and a shadow the same size as what casts it, however far away.
    CScene oSun;

    Check( oSun.FromJson( FloorUnder( "{ \"type\": \"directional\", \"direction\": [1, -1, 0], \"color\": [1, 1, 1] }",
                                      ", { \"type\": \"box\", \"center\": [0, 50, 0], \"size\": [2, 0.2, 2] }" ) ) == 0,
           "a scene with a directional light loads" );

    CLight *pSun = oSun.GetLight( 0 );
    CVector oFrom( 3.0, 0.0, 4.0 );
    CVector oToSun = pSun ? pSun->ToLight( oFrom ) : CVector( 0, 0, 0 );

    Check( ABS( oToSun.Magnitude() - RAY_FAR_AWAY ) < 1e-6 && ABS( oToSun.GetX() + oToSun.GetY() ) < 1e-6 && oToSun.GetY() > 0.0,
           "a directional light is RAY_FAR_AWAY back along its direction" );

    double dNear = FloorAt( oSun, 0.0, 0.0 );
    double dFar = FloorAt( oSun, 1000.0, 3000.0 );

    Check( ABS( dNear - cos( PI / 4.0 ) ) < 1e-9 && ABS( dFar - dNear ) < 1e-9, "sunlight is the same everywhere" );
        // The box, 50 up, shades the floor 50 along the light: around (50, 0, 0).
    Check( FloorAt( oSun, 50.0, 0.0 ) == 0.0 && FloorAt( oSun, 50.0, 1.5 ) > 0.5 && FloorAt( oSun, 47.0, 0.0 ) > 0.5,
           "a directional light's shadow falls along it, the size of what casts it" );

        // A sun with an angle casts soft shadows: blurred more the farther
        // the shadow falls from what casts it.
    CScene oSoftSun;

    Check( oSoftSun.FromJson( FloorUnder( "{ \"type\": \"directional\", \"direction\": [0, -1, 0], \"color\": [1, 1, 1], \"angle\": 10, \"samples\": 64 }",
                                          ", { \"type\": \"box\", \"center\": [-50, 10, 0], \"size\": [100, 0.2, 100] }" ) ) == 0,
           "a directional light with an angle loads" );
    Check( ABS( FloorAt( oSoftSun, 0.0, 0.0 ) - 0.5 ) < 0.05 && FloorAt( oSoftSun, 0.5, 0.0 ) > 0.55 &&
           FloorAt( oSoftSun, 0.5, 0.0 ) < 0.98 && FloorAt( oSoftSun, -0.5, 0.0 ) < 0.45 && FloorAt( oSoftSun, -0.5, 0.0 ) > 0.02 &&
           FloorAt( oSoftSun, 3.0, 0.0 ) > 0.99, "a directional light with an angle casts a soft shadow edge" );
}

    // A Radiance HDR file of nWidth x nHeight, flat or run-length encoded,
    // whose pixel (x, y) is RGBE ( x * 16 + 8, y * 32 + 16, 100, 129 ).
static std::string MakeHdr( int nWidth, int nHeight, bool bEncoded, const char *strSize = NULL )
{
    std::string strData = "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n";
    char        achSize[ 64 ];

    snprintf( achSize, sizeof( achSize ), "-Y %d +X %d\n", nHeight, nWidth );
    strData += strSize ? strSize : achSize;
    for ( int nY = 0; nY < nHeight; nY++ )
    {
        std::vector<unsigned char> anRgbe;

        for ( int nX = 0; nX < nWidth; nX++ )
        {
            anRgbe.push_back( (unsigned char)( nX * 16 + 8 ) );
            anRgbe.push_back( (unsigned char)( nY * 32 + 16 ) );
            anRgbe.push_back( 100 );
            anRgbe.push_back( 129 );
        }
        if ( ! bEncoded )
        {
            strData.append( anRgbe.begin(), anRgbe.end() );
            continue;
        }
        strData += (char)2;
        strData += (char)2;
        strData += (char)( nWidth >> 8 );
        strData += (char)( nWidth & 0xFF );
        for ( int nChannel = 0; nChannel < 4; nChannel++ )
        {
                // Literals for red, runs for the rest.
            if ( nChannel == 0 )
            {
                strData += (char)nWidth;
                for ( int nX = 0; nX < nWidth; nX++ )
                    strData += (char)anRgbe[ (size_t)nX * 4 ];
            }
            else
            {
                strData += (char)( 128 + nWidth );
                strData += (char)anRgbe[ nChannel ];
            }
        }
    }
    return strData;
}

    // An image environment made from a file's bytes, as a scene copy carries them.
static bool LoadEnvironment( CEnvironment &oEnvironment, const std::string &strFile, const char *strMore = NULL )
{
    IASLib::CObjectNode         oNode;
    std::vector<unsigned char>  anData( strFile.begin(), strFile.end() );

    oNode.put( "type", "image" );
    oNode.put( "file", "test.hdr" );
    oNode.putBinary( "data", anData );
    if ( strMore && strcmp( strMore, "rotate" ) == 0 )
        oNode.put( "rotation", 90.0 );
    if ( strMore && strcmp( strMore, "bright" ) == 0 )
        oNode.put( "intensity", 2.0 );
    RayJson::ClearError();
    return oEnvironment.FromJson( oNode ) ? true : false;
}

    // The direction the middle of pixel (nX, nY) of an nWidth x nHeight
    // equirectangular image looks along, unrotated.
static CVector PixelDirection( int nX, int nY, int nWidth, int nHeight )
{
    double dLongitude = ( ( nX + 0.5 ) / nWidth - 0.5 ) * 2.0 * PI;
    double dPolar = ( nY + 0.5 ) / nHeight * PI;

    return CVector( sin( dLongitude ) * sin( dPolar ), cos( dPolar ), cos( dLongitude ) * sin( dPolar ) );
}

static bool SameColor( CColor oA, CColor oB, double dTolerance )
{
    return ABS( oA.GetR() - oB.GetR() ) <= dTolerance && ABS( oA.GetG() - oB.GetG() ) <= dTolerance &&
           ABS( oA.GetB() - oB.GetB() ) <= dTolerance;
}

static void EnvironmentChecks( void )
{
        // HDR: RGBE (m + 0.5) x 2^(e - 136), flat or run-length encoded.
    int                 nWidth = 0;
    int                 nHeight = 0;
    std::vector<float>  anFlat;
    std::vector<float>  anEncoded;
    std::string         strError;

    Check( CEnvironment::ParseHdr( MakeHdr( 8, 4, false ), nWidth, nHeight, anFlat, strError ) && nWidth == 8 && nHeight == 4,
           "a flat HDR file reads" );
    Check( anFlat.size() == 8 * 4 * 3 && anFlat[ ( ( 1 * 8 ) + 3 ) * 3 ] == (float)( ( 3 * 16 + 8 + 0.5 ) / 128.0 ) &&
           anFlat[ ( ( 1 * 8 ) + 3 ) * 3 + 1 ] == (float)( ( 1 * 32 + 16 + 0.5 ) / 128.0 ) &&
           anFlat[ ( ( 1 * 8 ) + 3 ) * 3 + 2 ] == (float)( 100.5 / 128.0 ), "an HDR pixel is ( m + 0.5 ) x 2^( e - 136 )" );
    Check( CEnvironment::ParseHdr( MakeHdr( 8, 4, true ), nWidth, nHeight, anEncoded, strError ) && anEncoded == anFlat,
           "a run-length encoded HDR file reads the same as a flat one" );
    Check( ! CEnvironment::ParseHdr( "P6\n1 1\n255\nabc", nWidth, nHeight, anFlat, strError ) &&
           strError.find( "isn't a Radiance HDR file" ) != std::string::npos, "a file that isn't HDR is turned down" );
    Check( ! CEnvironment::ParseHdr( MakeHdr( 8, 4, false, "+Y 4 +X 8\n" ), nWidth, nHeight, anFlat, strError ) &&
           strError.find( "-Y height +X width" ) != std::string::npos, "an HDR file stored another way up is turned down" );

    std::string strShort = MakeHdr( 8, 4, true );

    strShort.resize( strShort.size() - 5 );
    Check( ! CEnvironment::ParseHdr( strShort, nWidth, nHeight, anFlat, strError ) && strError.find( "ends early" ) != std::string::npos,
           "a cut-off HDR file is turned down" );

    std::string strBadRun = MakeHdr( 8, 4, true );
    size_t      nData = strBadRun.find( "+X 8\n" ) + 5;

    strBadRun[ nData + 4 ] = (char)( 128 + 9 );     // a run past the end of the row
    Check( ! CEnvironment::ParseHdr( strBadRun, nWidth, nHeight, anFlat, strError ) && strError.find( "bad HDR run" ) != std::string::npos,
           "an HDR run past the end of its row is turned down" );

        // An image environment: the middle of each pixel looks along its
        // direction, and the sides wrap.
    CEnvironment oImage;

    Check( LoadEnvironment( oImage, MakeHdr( 8, 4, false ) ), "an image environment loads from the file's bytes" );

    bool bPixels = true;

    for ( int nY = 0; nY < 4; nY++ )
    {
        for ( int nX = 0; nX < 8; nX++ )
        {
            CColor oSeen = oImage.ColorIn( PixelDirection( nX, nY, 8, 4 ) );
            CColor oPixel( ( nX * 16 + 8 + 0.5 ) / 128.0, ( nY * 32 + 16 + 0.5 ) / 128.0, 100.5 / 128.0 );

            bPixels = bPixels && SameColor( oSeen, oPixel, 1e-6 );
        }
    }
    Check( bPixels, "an image environment shows each pixel along its direction" );

    FLT nU;
    FLT nV;

    oImage.ImageCoords( 0.0, 0.0, 1.0, nU, nV );
        // (PI is Ray's own, to eleven places, so these are near, not exact.)
    Check( ABS( nU - 0.5 ) < 1e-9 && ABS( nV - 0.5 ) < 1e-9, "the middle of the image lies along +z" );
    oImage.ImageCoords( 1.0, 0.0, 0.0, nU, nV );
    Check( ABS( nU - 0.75 ) < 1e-9, "+x is a quarter of the way right of the middle" );
    oImage.ImageCoords( 0.0, 1.0, 0.0, nU, nV );
    Check( ABS( nV ) < 1e-9, "straight up is the top of the image" );

        // Looking along -z, straight back, is the left and right edges at
        // once: half the first column and half the last.
    CColor oBack = oImage.ColorIn( CVector( -1e-15, 0.0, -1.0 ) );
    CColor oSeam( ( ( 8 + 0.5 ) / 128.0 + ( 7 * 16 + 8 + 0.5 ) / 128.0 ) / 2.0, ( ( 1 * 32 + 16 + 0.5 ) / 128.0 + ( 2 * 32 + 16 + 0.5 ) / 128.0 ) / 2.0,
                  100.5 / 128.0 );

    Check( SameColor( oBack, oSeam, 1e-6 ), "an image environment wraps around its sides" );

    CEnvironment oTurned;
    CEnvironment oBright;

    Check( LoadEnvironment( oTurned, MakeHdr( 8, 4, false ), "rotate" ) &&
           SameColor( oTurned.ColorIn( CVector( 1.0, 0.0, 0.0 ) ), oImage.ColorIn( CVector( 0.0, 0.0, 1.0 ) ), 1e-9 ),
           "an environment's rotation turns it about +y" );
    Check( LoadEnvironment( oBright, MakeHdr( 8, 4, false ), "bright" ) &&
           SameColor( oBright.ColorIn( CVector( 0.3, 0.2, 1.0 ) ), oImage.ColorIn( CVector( 0.3, 0.2, 1.0 ) ) * 2.0, 1e-12 ),
           "an environment's intensity scales it" );

        // A sky: the zenith overhead, the horizon level, the ground below,
        // blended by height to the exponent.
    CScene oSky;

    Check( oSky.FromJson( FloorUnder( "", "", "  \"environment\": { \"type\": \"sky\", \"zenith\": [0, 0, 1], \"horizon\": [1, 1, 1], "
                                       "\"ground\": [0.5, 0.25, 0], \"exponent\": 1 },\n" ) ) == 0 && oSky.GetEnvironment() != NULL,
           "a scene with a sky loads" );
    if ( oSky.GetEnvironment() )
    {
        const CEnvironment *pSky = oSky.GetEnvironment();

        Check( SameColor( pSky->ColorIn( CVector( 0, 1, 0 ) ), CColor( 0, 0, 1 ), 1e-12 ) &&
               SameColor( pSky->ColorIn( CVector( 1, 0, 0 ) ), CColor( 1, 1, 1 ), 1e-12 ) &&
               SameColor( pSky->ColorIn( CVector( 0, -1, 0 ) ), CColor( 0.5, 0.25, 0 ), 1e-12 ) &&
               SameColor( pSky->ColorIn( CVector( 0, 1, 1 ) ), CColor( 1.0 - sqrt( 0.5 ), 1.0 - sqrt( 0.5 ), 1.0 ), 1e-12 ),
               "a sky blends from the horizon to the zenith and the ground by height" );
        Check( SameColor( pSky->IrradianceIn( CVector( 0, 1, 0 ) ), CColor( 0, 0, 0 ), 0.0 ), "a sky without samples lights nothing" );
    }

        // A ray that leaves the scene sees the sky, not the background.
    Check( SameColor( TraceFrom( oSky, CVector( 0, 1, 0 ), CVector( 0, 1, 0 ) ), CColor( 0, 0, 1 ), 1e-12 ) &&
           SameColor( TraceFrom( oSky, CVector( 0, 1, 0 ), CVector( 1, 0, 0 ) ), CColor( 1, 1, 1 ), 1e-12 ),
           "a ray leaving the scene sees the environment" );

        // The sky's light: a uniform sky of color c lights a surface facing
        // any way with c; a sky white above the horizon and black below
        // lights the floor with 1, a wall with half, and a ceiling not at all.
    CScene oUniform;

    Check( oUniform.FromJson( FloorUnder( "", "", "  \"environment\": { \"type\": \"sky\", \"zenith\": [0.5, 0.5, 0.5], \"horizon\": [0.5, 0.5, 0.5], "
                                           "\"ground\": [0.5, 0.5, 0.5], \"samples\": 16 },\n" ) ) == 0 && oUniform.GetEnvironment(),
           "a scene lit by a uniform sky loads" );
    if ( oUniform.GetEnvironment() )
    {
        bool bEven = true;

        for ( int nIndex = 0; nIndex < 20; nIndex++ )
        {
            CVector oNormal( sin( nIndex * 0.7 ) * cos( nIndex * 1.3 ), cos( nIndex * 0.7 ), sin( nIndex * 0.7 ) * sin( nIndex * 1.3 ) );

            bEven = bEven && SameColor( oUniform.GetEnvironment()->IrradianceIn( oNormal ), CColor( 0.5, 0.5, 0.5 ), 0.005 );
        }
        Check( bEven, "a uniform sky lights a surface facing any way the same" );
    }
    Check( ABS( FloorAt( oUniform, 0.0, 0.0 ) - 0.5 ) < 0.005, "a floor under a uniform sky of 0.5 is lit to 0.5" );

    CScene oHalfSky;

    Check( oHalfSky.FromJson( FloorUnder( "", "", "  \"environment\": { \"type\": \"sky\", \"zenith\": [1, 1, 1], \"horizon\": [1, 1, 1], "
                                           "\"ground\": [0, 0, 0], \"exponent\": 0.000001, \"samples\": 64 },\n" ) ) == 0 &&
           oHalfSky.GetEnvironment(), "a scene lit by a half sky loads" );
    if ( oHalfSky.GetEnvironment() )
    {
        const CEnvironment *pHalf = oHalfSky.GetEnvironment();

        Check( ABS( pHalf->IrradianceIn( CVector( 0, 1, 0 ) ).GetR() - 1.0 ) < 0.01 &&
               ABS( pHalf->IrradianceIn( CVector( 1, 0, 0 ) ).GetR() - 0.5 ) < 0.01 &&
               pHalf->IrradianceIn( CVector( 0, -1, 0 ) ).GetR() < 0.01, "the sky's light depends on how much of it a surface faces" );
    }

        // At the foot of a wall, half the sky is hidden: half the light.
    CScene oWalled;
    std::string strWalled = FloorUnder( "", ", { \"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [-1, 0, 0], "
                                        "\"materials\": [ { \"type\": \"diffuse\", \"color\": [1, 1, 1] } ] }",
                                        "  \"environment\": { \"type\": \"sky\", \"zenith\": [1, 1, 1], \"horizon\": [1, 1, 1], "
                                        "\"ground\": [1, 1, 1], \"samples\": 64 },\n" );

    Check( oWalled.FromJson( strWalled ) == 0, "a floor beside a wall under a sky loads" );

    double dOpen = FloorAt( oUniform, 0.0, 0.0 ) * 2.0;     // a white sky of 1
    double dFoot = TraceFrom( oWalled, CVector( -1e-6, 0.5, 0.0 ), CVector( 0.0, -1.0, 0.0 ) ).GetR();

    Check( ABS( dFoot / dOpen - 0.5 ) < 0.05, "a point at the foot of a wall gets about half the sky's light" );

        // Shiny materials take the sky as their reflections, not as light.
    CScene oShiny;

    Check( oShiny.FromJson( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"specular\" } ]" )
                            .replace( 0, 1, "{ \"environment\": { \"type\": \"sky\", \"samples\": 4 }, \"background\": [0, 0, 0], " ) ) == 0,
           "a shiny ball under a sky loads" );

    CScene oShinyDark;

    Check( oShinyDark.FromJson( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"specular\" } ]" ) ) == 0,
           "a shiny ball loads" );
    Check( SameColor( TraceFrom( oShiny, CVector( 0, 0, -5 ), CVector( 0, 0, 1 ) ), TraceFrom( oShinyDark, CVector( 0, 0, -5 ), CVector( 0, 0, 1 ) ), 0.0 ),
           "a specular material isn't lit by the sky" );

        // Read, save, read back; errors.
    std::string strSaved = oSky.ToJson();
    CScene      oReread;

    Check( oReread.FromJson( strSaved ) == 0 && oReread.ToJson() == strSaved, "a sky reads back the same" );
    Check( strSaved.find( "\"environment\"" ) != std::string::npos && strSaved.find( "\"exponent\": 1.0" ) != std::string::npos &&
           strSaved.find( "\"samples\": 0" ) != std::string::npos, "a sky saves its settings" );

    CScene oNone;

    Check( oNone.FromJson( s_strMinimal ) == 0 && oNone.ToJson().find( "environment" ) == std::string::npos &&
           oNone.GetEnvironment() == NULL, "a scene without an environment saves none" );

    ExpectError( Minimal( "\"camera\"", "\"environment\": { \"type\": \"cloud\" }, \"camera\"" ), BAD_FILE, "unknown environment type \"cloud\"" );
    ExpectError( Minimal( "\"camera\"", "\"environment\": { \"type\": \"sky\", \"intensity\": -1 }, \"camera\"" ), BAD_FILE,
                 "intensity must be 0 or more" );
    ExpectError( Minimal( "\"camera\"", "\"environment\": { \"type\": \"sky\", \"exponent\": 0 }, \"camera\"" ), BAD_FILE,
                 "exponent must be more than 0" );
    ExpectError( Minimal( "\"camera\"", "\"environment\": { \"type\": \"sky\", \"samples\": 300 }, \"camera\"" ), BAD_FILE, "samples" );
    ExpectError( Minimal( "\"camera\"", "\"environment\": { \"type\": \"sky\", \"file\": \"x.hdr\" }, \"camera\"" ), BAD_FILE, "file" );
    ExpectError( Minimal( "\"camera\"", "\"environment\": { \"type\": \"image\" }, \"camera\"" ), BAD_FILE, "file" );
    ExpectError( Minimal( "\"camera\"", "\"environment\": { \"type\": \"image\", \"file\": \"no_such_sky.hdr\" }, \"camera\"" ), BAD_FILE,
                 "can't open the image \"no_such_sky.hdr\"" );
    ExpectError( Minimal( "\"camera\"", "\"environment\": [ 1 ], \"camera\"" ), BAD_FILE, "must be an object" );
}

    // The sky image in scenes/textures reads, its sun far brighter than white.
static void SkyTextureChecks( const char *strScenes )
{
    std::string         strSky = ReadFile( std::string( strScenes ) + "/textures/sky.hdr" );
    int                 nWidth = 0;
    int                 nHeight = 0;
    std::vector<float>  anPixels;
    std::string         strError;
    float               nBrightest = 0.0f;

    Check( CEnvironment::ParseHdr( strSky, nWidth, nHeight, anPixels, strError ) && nWidth == 512 && nHeight == 256,
           "textures/sky.hdr reads as a 512 x 256 HDR image" );
    for ( size_t nIndex = 0; nIndex < anPixels.size(); nIndex++ )
        nBrightest = std::max( nBrightest, anPixels[ nIndex ] );
    Check( nBrightest > 10.0f, "textures/sky.hdr's sun is far brighter than white" );
}

// --------------------------------------------------------------------------
// Constructive solid geometry, dispersive glass and the sample clamp
// --------------------------------------------------------------------------

    // The ray hits the csg at t = dT, where its outward normal (as Shade
    // shades it) is (dNX, dNY, dNZ). pIgnore is passed on.
static bool CsgHitsAt( CObjects &oObject, CRay oRay, double dT, double dNX, double dNY, double dNZ, CObjects *pIgnore = NULL )
{
    CVector oHit;
    CVector oNormal;

    if ( ! oObject.Intersect( oRay, oHit, pIgnore ) || ABS( oRay.GetT() - dT ) > 1e-9 || ! oObject.SurfaceNormal( oHit, oNormal ) )
        return false;
    return ABS( oNormal.GetX() - dNX ) < 1e-9 && ABS( oNormal.GetY() - dNY ) < 1e-9 && ABS( oNormal.GetZ() - dNZ ) < 1e-9;
}

    // A csg's JSON (an object of the "objects" array) with strParts.
static std::string CsgOf( const char *strOperation, const char *strParts, const char *strMore = "" )
{
    return std::string( "{ \"type\": \"csg\", \"operation\": \"" ) + strOperation + "\", \"objects\": [ " + strParts + " ]" +
           strMore + " }";
}

static void CsgChecks( void )
{
    CScene      oScene;
    CObjects   *pObject;
    CVector     oHit;

        // A lens: where two unit spheres, at z = -0.5 and 0.5, overlap.
    std::string strLens = CsgOf( "intersection", "{ \"type\": \"sphere\", \"center\": [0, 0, 0.5], \"radius\": 1 },"
                                                 "{ \"type\": \"sphere\", \"center\": [0, 0, -0.5], \"radius\": 1 }" );

    pObject = FirstObject( oScene, strLens.c_str() );
    if ( pObject )
    {
        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 4.5, 0, 0, -1 ), "a ray hits an intersection where both shapes are" );
        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, 0, 0, 0, 1 ), 0.5, 0, 0, 1 ), "a ray from inside an intersection hits its far side" );
        Check( Misses( *pObject, RayFrom( 0, 0.95, -5, 0, 0, 1 ) ), "a ray through only one of an intersection's shapes misses it" );
        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, -0.5, 0, 0, 1 ), 1.0, 0, 0, 1, pObject ),
               "a ray leaving a csg's surface inward, ignoring the csg, hits its far side" );

        CRay oOut = RayFrom( 0, 0, -0.5, 0, 0, -1 );
        Check( ! pObject->Intersect( oOut, oHit, pObject ), "a ray leaving a csg's surface outward, ignoring the csg, misses it" );

        CVector oLow, oHigh;
        Check( pObject->GetBounds( oLow, oHigh ) && oLow.GetZ() > -0.6 && oHigh.GetZ() < 0.6 && oLow.GetX() < -0.99 && oHigh.GetX() > 0.99,
               "an intersection's box is where its shapes' boxes overlap" );
    }

        // Two unit spheres at x = -0.5 and 0.5, as one: no surface inside.
    pObject = FirstObject( oScene, CsgOf( "union", "{ \"type\": \"sphere\", \"center\": [-0.5, 0, 0], \"radius\": 1 },"
                                                   "{ \"type\": \"sphere\", \"center\": [0.5, 0, 0], \"radius\": 1 }" ).c_str() );
    if ( pObject )
    {
        double dZ = sqrt( 0.75 );

        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 5.0 - dZ, 0.5, 0, -dZ ),
               "a ray hits a union where its shapes meet (the first shape's surface)" );
        Check( CsgHitsAt( *pObject, RayFrom( -0.5, 0, 0, 1, 0, 0 ), 2.0, 1, 0, 0 ),
               "a ray inside a union passes the surfaces inside it and leaves by the far one" );
        Check( Misses( *pObject, RayFrom( 0, 1.2, -5, 0, 0, 1 ) ), "a ray past a union misses it" );
    }

        // A cube of side 2 with a ball of radius 1.2 taken out: only its
        // corners are left.
    pObject = FirstObject( oScene, CsgOf( "difference", "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 2 },"
                                                        "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1.2 }" ).c_str() );
    if ( pObject )
    {
        double dS = 1.0 / sqrt( 3.0 );

        Check( Misses( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ) ), "a ray through the hole a difference takes out passes through" );
        Check( CsgHitsAt( *pObject, RayFrom( 0.9, 0.9, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a ray hits what a difference leaves" );
        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, 0, dS, dS, dS ), 1.2, -dS, -dS, -dS ),
               "a difference's hollow is the taken-out shape turned inside out" );

        CVector oLow, oHigh;
        Check( pObject->GetBounds( oLow, oHigh ) && ABS( oHigh.GetX() - 1.0 ) < 1e-3, "a difference's box is its first shape's" );
    }

        // A hollow ball (a shell from radius 0.9 to 1): a ray leaving its
        // inside surface, ignoring the csg, crosses the hollow to the far
        // side, so a bowl shadows and reflects its own inside.
    pObject = FirstObject( oScene, CsgOf( "difference", "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 },"
                                                        "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 0.9 }" ).c_str() );
    if ( pObject )
        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, -0.9, 0, 0, 1 ), 1.8, 0, 0, -1, pObject ),
               "a ray from a csg's inside surface, ignoring the csg, hits its own far side" );

        // A die: a rounded cube (a csg) with a pip drilled in its front; the
        // pip is black and the body white (childMaterials).
    std::string strDie = CsgOf( "difference",
        "{ \"type\": \"csg\", \"operation\": \"intersection\", \"objects\": ["
        "  { \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 2, \"materials\": [ { \"type\": \"ambient\", \"color\": [1, 1, 1] } ] },"
        "  { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1.36, \"materials\": [ { \"type\": \"ambient\", \"color\": [1, 1, 1] } ] } ] },"
        "{ \"type\": \"sphere\", \"center\": [0, 0, -1.05], \"radius\": 0.3, \"materials\": [ { \"type\": \"ambient\", \"color\": [0, 0, 0.5] } ] }",
        ", \"childMaterials\": true" );

    pObject = FirstObject( oScene, strDie.c_str() );
    if ( pObject )
    {
        double dRound = sqrt( 1.36 * 1.36 - 1.28 );

        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 4.25, 0, 0, -1 ), "a ray into a drilled pip hits its bottom" );
        Check( CsgHitsAt( *pObject, RayFrom( 0.8, 0.8, -5, 0, 0, 1 ), 5.0 - dRound, 0.8 / 1.36, 0.8 / 1.36, -dRound / 1.36 ),
               "a ray hits a nested csg's rounded corner" );
        Check( CsgHitsAt( *pObject, RayFrom( 0.5, 0.2, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a ray hits a nested csg's flat face" );

        CColor oPip = TraceFrom( oScene, CVector( 0, 0, -5 ), CVector( 0, 0, 1 ) );
        CColor oBody = TraceFrom( oScene, CVector( 0.5, 0.2, -5 ), CVector( 0, 0, 1 ) );

        Check( ABS( oPip.GetR() ) < 1e-12 && ABS( oPip.GetB() - 0.5 ) < 1e-12 && ABS( oBody.GetR() - 1.0 ) < 1e-12,
               "with childMaterials each surface is shaded with its own shape's materials" );
    }

        // Without childMaterials the csg's own materials shade it all.
    std::string strOwn = CsgOf( "difference",
        "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 2, \"materials\": [ { \"type\": \"ambient\", \"color\": [1, 0, 0] } ] },"
        "{ \"type\": \"sphere\", \"center\": [0, 0, -1.05], \"radius\": 0.3, \"materials\": [ { \"type\": \"ambient\", \"color\": [0, 0, 1] } ] }",
        ", \"materials\": [ { \"type\": \"ambient\", \"color\": [0, 1, 0] } ]" );

    pObject = FirstObject( oScene, strOwn.c_str() );
    if ( pObject )
    {
        CColor oPip = TraceFrom( oScene, CVector( 0, 0, -5 ), CVector( 0, 0, 1 ) );
        CColor oFace = TraceFrom( oScene, CVector( 0.5, 0.5, -5 ), CVector( 0, 0, 1 ) );

        Check( ABS( oPip.GetG() - 1.0 ) < 1e-12 && ABS( oFace.GetG() - 1.0 ) < 1e-12 && ABS( oPip.GetR() ) < 1e-12 && ABS( oPip.GetB() ) < 1e-12,
               "a csg shades with its own materials" );
    }

        // A cylinder and two balls make a capsule, which round-trips, bump
        // map and all.
    std::string strCapsule = CsgOf( "union",
        "{ \"type\": \"cylinder\", \"base\": [-1, 0, 0], \"top\": [1, 0, 0], \"radius\": 0.5 },"
        "{ \"type\": \"sphere\", \"center\": [-1, 0, 0], \"radius\": 0.5 },"
        "{ \"type\": \"sphere\", \"center\": [1, 0, 0], \"radius\": 0.5 }",
        ", \"bump\": { \"texture\": { \"type\": \"noise\", \"scale\": 0.1 }, \"depth\": 0.01 }" );
    CScene oAgain;

    pObject = FirstObject( oScene, strCapsule.c_str() );
    if ( pObject )
    {
        Check( CsgHitsAt( *pObject, RayFrom( 1.2, 0, -5, 0, 0, 1 ), 5.0 - sqrt( 0.25 - 0.04 ), 0.2 / 0.5, 0, -sqrt( 0.21 ) / 0.5 ),
               "a three-part union's rounded end" );
        Check( oAgain.FromJson( oScene.ToJson() ) == 0 && oAgain.ToJson() == oScene.ToJson() && oAgain.GetObjectAt( 0 ) &&
               oAgain.GetObjectAt( 0 )->GetBump() != nullptr, "a csg round-trips through JSON" );
    }

    std::string strTwo = "\"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 } ]";

    ExpectError( Minimal( strTwo.c_str(), ( "\"objects\": [ " + CsgOf( "xor", "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 },"
                                                                               "{ \"type\": \"sphere\", \"center\": [1, 0, 0], \"radius\": 1 }" ) + " ]" ).c_str() ),
                 BAD_FILE, "objects[0]: operation must be \"union\", \"intersection\" or \"difference\"" );
    ExpectError( Minimal( strTwo.c_str(), ( "\"objects\": [ " + CsgOf( "union", "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }" ) + " ]" ).c_str() ),
                 BAD_FILE, "objects[0].objects: must be an array of at least two objects" );
    ExpectError( Minimal( strTwo.c_str(), ( "\"objects\": [ " + CsgOf( "union", "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 },"
                                                                                 "{ \"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [0, 1, 0] }" ) + " ]" ).c_str() ),
                 BAD_FILE, "objects[0].objects[1]: a csg combines only closed shapes" );
    ExpectError( Minimal( strTwo.c_str(), ( "\"objects\": [ " + CsgOf( "union", "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 },"
                                                                                 "{ \"type\": \"cylinder\", \"base\": [0, 0, 0], \"top\": [0, 1, 0], \"radius\": 1, \"capped\": false }" ) + " ]" ).c_str() ),
                 BAD_FILE, "objects[0].objects[1]: a csg combines only closed shapes" );
    ExpectError( Minimal( strTwo.c_str(), ( "\"objects\": [ " + CsgOf( "union", "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 },"
                                                                                 "{ \"type\": \"teapot\" }" ) + " ]" ).c_str() ),
                 BAD_FILE, "objects[0].objects[1]: unknown object type \"teapot\"" );
}

    // A dispersive glass ball (index 1.5, Abbe number 20, unless strGlass
    // says otherwise) before a checkered wall, and the scene's first object.
static std::string GlassBallBeforeWall( const char *strGlass )
{
    std::string strText =
        "{ \"format\": \"Ray scene\", \"version\": 3, \"background\": [0, 0, 0],\n"
        "  \"camera\": { \"position\": [0, 0, -10], \"lookAt\": [0, 0, 0] },\n"
        "  \"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1, \"materials\": [ { \"type\": \"transparent\", "
        "\"color\": [1, 1, 1], ";

    strText += strGlass;
    strText +=
        " } ] },\n"
        "    { \"type\": \"plane\", \"position\": [0, 0, 3], \"normal\": [0, 0, -1], \"materials\": [ { \"type\": \"ambient\", \"color\": [1, 1, 1],"
        "      \"texture\": { \"type\": \"checker\", \"colors\": [ [1, 1, 1], [0, 0, 0] ], \"size\": 0.05 } } ] } ],\n"
        "  \"lights\": [ ] }\n";
    return strText;
}

static CColor TraceChannel( CScene &oScene, double dX, double dY, int nChannel )
{
    CRay    oRay;
    CColor  oColor( 0, 0, 0 );

    oRay.Set( dX, dY, -5.0, 0.0, 0.0, 1.0, 1e100, 1.0, 0, 0 );
    oRay.SetChannel( nChannel );
    oScene.TraceRay( oRay, oColor, NULL );
    return oColor;
}

static void DispersionChecks( void )
{
        // Cauchy through n_d and the Abbe number: n_F - n_C = ( n_d - 1 ) / V.
    FLT nRed = CLens::DispersedIndex( 1.5, 20.0, CLens::CHANNEL_RED );
    FLT nGreen = CLens::DispersedIndex( 1.5, 20.0, CLens::CHANNEL_GREEN );
    FLT nBlue = CLens::DispersedIndex( 1.5, 20.0, CLens::CHANNEL_BLUE );

    Check( nGreen == 1.5 && nRed < 1.5 && nBlue > 1.5 && ABS( ( nBlue - nRed ) - 0.5 / 20.0 ) < 1e-12,
           "dispersed indices follow the Abbe number, green at the d line" );
    Check( CLens::DispersedIndex( 1.5, 0.0, CLens::CHANNEL_RED ) == 1.5 && CLens::DispersedIndex( 1.5, 20.0, CLens::CHANNEL_ALL ) == 1.5,
           "no Abbe number, or every channel at once, is the index as given" );

    CScene oScene;

    Check( oScene.FromJson( GlassBallBeforeWall( "\"indexOfRefraction\": 1.5, \"abbe\": 20" ) ) == 0, "a dispersive glass ball loads" );

    CObjects               *pBall = oScene.GetObjectAt( 0 );
    CTransparentMaterial   *pGlass = pBall ? dynamic_cast<CTransparentMaterial *>( pBall->GetMaterials()[ 0 ] ) : NULL;

    Check( pGlass && pGlass->IndexFor( CLens::CHANNEL_RED ) == nRed && pGlass->IndexFor( CLens::CHANNEL_BLUE ) == nBlue &&
           pGlass->IndexFor( CLens::CHANNEL_ALL ) == 1.5, "transparent materials disperse as lens glass does" );

    if ( pGlass )
    {
            // Blue bends more than red, going in at 45 degrees.
        CVector oView( 0.0, -1.0, 1.0 );
        CVector oNormal( 0.0, 0.0, -1.0 );
        CVector oRedWay, oBlueWay;

        CTransparentMaterial::Refract( oView, oNormal, pGlass->IndexFor( CLens::CHANNEL_RED ), oRedWay );
        CTransparentMaterial::Refract( oView, oNormal, pGlass->IndexFor( CLens::CHANNEL_BLUE ), oBlueWay );
        Check( AngleFromNormal( oBlueWay, CVector( 0, 0, 1 ) ) < AngleFromNormal( oRedWay, CVector( 0, 0, 1 ) ) - 0.1,
               "dispersive glass bends blue more than red" );
    }

        // A ray in every channel splits at the glass: each of its channels
        // is what a ray traced in that channel alone sees.
    bool bSplits = true;
    bool bDiffers = false;

    for ( int nStep = 0; nStep < 40; nStep++ )
    {
        double  dX = -0.9 + nStep * 0.045;
        CColor  oWhite = TraceChannel( oScene, dX, 0.31, CLens::CHANNEL_ALL );
        CColor  oRed = TraceChannel( oScene, dX, 0.31, CLens::CHANNEL_RED );
        CColor  oGreen = TraceChannel( oScene, dX, 0.31, CLens::CHANNEL_GREEN );
        CColor  oBlue = TraceChannel( oScene, dX, 0.31, CLens::CHANNEL_BLUE );

        if ( oWhite.GetR() != oRed.GetR() || oWhite.GetG() != oGreen.GetG() || oWhite.GetB() != oBlue.GetB() )
            bSplits = false;
        if ( oRed.GetR() != oBlue.GetR() )
            bDiffers = true;
    }
    Check( bSplits, "a ray in every channel splits into red, green and blue at dispersive glass" );
    Check( bDiffers, "red and blue rays see different things through dispersive glass" );

        // Through the middle nothing bends, in any channel.
    CColor oMiddle = TraceChannel( oScene, 0.0, 0.0, CLens::CHANNEL_ALL );
    CScene oClear;

    Check( oClear.FromJson( GlassBallBeforeWall( "\"indexOfRefraction\": 1.5" ) ) == 0, "a clear glass ball loads" );

    CColor oClearMiddle = TraceChannel( oClear, 0.0, 0.0, CLens::CHANNEL_ALL );

    Check( ABS( oMiddle.GetR() - oClearMiddle.GetR() ) < 1e-12 && ABS( oMiddle.GetB() - oClearMiddle.GetB() ) < 1e-12,
           "straight through the middle, dispersive glass sees what clear glass does" );

        // Clear glass treats every channel alike.
    bool bSame = true;

    for ( int nStep = 0; nStep < 20; nStep++ )
    {
        double  dX = -0.9 + nStep * 0.09;
        CColor  oRed = TraceChannel( oClear, dX, 0.31, CLens::CHANNEL_RED );
        CColor  oBlue = TraceChannel( oClear, dX, 0.31, CLens::CHANNEL_BLUE );

        if ( oRed.GetR() != oBlue.GetR() )
            bSame = false;
    }
    Check( bSame, "glass without an Abbe number bends every channel alike" );

    std::string strBall = "\"radius\": 1";

    ExpectError( Minimal( strBall.c_str(), "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"abbe\": -3 } ]" ),
                 BAD_FILE, "abbe can't be negative" );
    ExpectError( Minimal( strBall.c_str(), "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"abbe\": 2 } ]" ),
                 BAD_FILE, "abbe must be 0 (no dispersion) or at least 5" );

    CScene oSaved;

    Check( oSaved.FromJson( oScene.ToJson() ) == 0 && oSaved.ToJson() == oScene.ToJson() &&
           oScene.ToJson().find( "\"abbe\": 20" ) != std::string::npos && oClear.ToJson().find( "abbe" ) == std::string::npos,
           "an Abbe number round-trips, and none is written when there is none" );
}

static void SpectralChecks( void )
{
        // An even spread of wavelengths weighs up to exactly white.
    const int   STEPS = 34000;
    double      adSum[ 3 ] = { 0.0, 0.0, 0.0 };

    for ( int nStep = 0; nStep < STEPS; nStep++ )
    {
        FLT anWeight[ 3 ];

        RaySpectrum::Weights( RaySpectrum::SHORTEST + ( ( nStep + 0.5 ) * ( RaySpectrum::LONGEST - RaySpectrum::SHORTEST ) / STEPS ), anWeight );
        for ( int nIndex = 0; nIndex < 3; nIndex++ )
            adSum[ nIndex ] += anWeight[ nIndex ] / STEPS;
    }
    Check( fabs( adSum[ 0 ] - 1.0 ) < 1e-9 && fabs( adSum[ 1 ] - 1.0 ) < 1e-9 && fabs( adSum[ 2 ] - 1.0 ) < 1e-9,
           "spectral weights over an even spread of wavelengths add up to white" );

    FLT anRed[ 3 ], anGreen[ 3 ], anBlue[ 3 ];

    RaySpectrum::Weights( 650.0, anRed );
    RaySpectrum::Weights( 530.0, anGreen );
    RaySpectrum::Weights( 450.0, anBlue );
    Check( anRed[ 0 ] > anRed[ 1 ] && anRed[ 0 ] > anRed[ 2 ] && anGreen[ 1 ] > anGreen[ 0 ] && anGreen[ 1 ] > anGreen[ 2 ] &&
           anBlue[ 2 ] > anBlue[ 0 ] && anBlue[ 2 ] > anBlue[ 1 ], "650 nm looks red, 530 nm green and 450 nm blue" );
    Check( RaySpectrum::PartWavelength( 0, 0.0 ) == RaySpectrum::SHORTEST && RaySpectrum::PartWavelength( RaySpectrum::PARTS - 1, 1.0 ) == RaySpectrum::LONGEST,
           "a split's parts cover the range" );

        // Wavelength channels, and the same index as the lens's lines.
    int nChannel = CLens::WavelengthChannel( 500.0 );

    Check( CLens::IsSpectral( nChannel ) && ! CLens::IsSpectral( CLens::CHANNEL_BLUE ) && CLens::ChannelWavelength( nChannel ) == 500.0,
           "a wavelength channel carries its wavelength" );
    Check( CLens::IndexAt( 1.5, 20.0, 0.65627 ) == CLens::DispersedIndex( 1.5, 20.0, CLens::CHANNEL_RED ) &&
           CLens::IndexAt( 1.5, 20.0, 0.48613 ) == CLens::DispersedIndex( 1.5, 20.0, CLens::CHANNEL_BLUE ) &&
           ABS( CLens::IndexAt( 1.5, 20.0, 0.58756 ) - 1.5 ) < 1e-12, "the index at a wavelength matches the C, d and F lines" );
    Check( ABS( CLens::DispersedIndex( 1.5, 20.0, CLens::WavelengthChannel( 656.27 ) ) - CLens::DispersedIndex( 1.5, 20.0, CLens::CHANNEL_RED ) ) < 1e-12 &&
           CLens::DispersedIndex( 1.5, 0.0, nChannel ) == 1.5, "a wavelength channel disperses as its line does" );

    {
        CScene oScene;

        Check( oScene.FromJson( WithLens( "{ \"prescription\": \"double-gauss-50mm\" }" ) ) == 0, "a lens scene loads" );

        CLens *pLens = oScene.GetCamera().GetLens();

        Check( pLens && ABS( pLens->FocalLengthIn( CLens::WavelengthChannel( 656.27 ) ) - pLens->FocalLengthIn( CLens::CHANNEL_RED ) ) < 1e-6 &&
               ABS( pLens->FocalLengthIn( CLens::WavelengthChannel( 486.13 ) ) - pLens->FocalLengthIn( CLens::CHANNEL_BLUE ) ) < 1e-6,
               "a lens traced at a wavelength focuses as in that line's channel" );
    }

        // A spectral camera: white light splits at dispersive glass into
        // single wavelengths, the same each time; without dispersive glass
        // nothing changes.
    std::string strSpectral = GlassBallBeforeWall( "\"indexOfRefraction\": 1.5, \"abbe\": 20" );
    std::string strClear = GlassBallBeforeWall( "\"indexOfRefraction\": 1.5" );

    strSpectral.replace( strSpectral.find( "\"lookAt\": [0, 0, 0] }" ), 21, "\"lookAt\": [0, 0, 0], \"dispersion\": \"spectral\" }" );
    strClear.replace( strClear.find( "\"lookAt\": [0, 0, 0] }" ), 21, "\"lookAt\": [0, 0, 0], \"dispersion\": \"spectral\" }" );

    CScene oSpectral, oRgb, oClearSpectral, oClearRgb;

    Check( oSpectral.FromJson( strSpectral ) == 0 && oRgb.FromJson( GlassBallBeforeWall( "\"indexOfRefraction\": 1.5, \"abbe\": 20" ) ) == 0 &&
           oClearSpectral.FromJson( strClear ) == 0 && oClearRgb.FromJson( GlassBallBeforeWall( "\"indexOfRefraction\": 1.5" ) ) == 0,
           "spectral and RGB glass scenes load" );
    Check( oSpectral.GetCamera().IsSpectral() && ! oRgb.GetCamera().IsSpectral(), "\"dispersion\": \"spectral\" is read" );

    bool bSame = true, bRepeats = true, bDiffers = false;

    for ( int nStep = 0; nStep < 40; nStep++ )
    {
        double  dX = -0.9 + nStep * 0.045;
        CColor  oFirst = TraceChannel( oSpectral, dX, 0.31, CLens::CHANNEL_ALL );
        CColor  oAgain = TraceChannel( oSpectral, dX, 0.31, CLens::CHANNEL_ALL );
        CColor  oRgbSeen = TraceChannel( oRgb, dX, 0.31, CLens::CHANNEL_ALL );
        CColor  oClearOne = TraceChannel( oClearSpectral, dX, 0.31, CLens::CHANNEL_ALL );
        CColor  oClearTwo = TraceChannel( oClearRgb, dX, 0.31, CLens::CHANNEL_ALL );

        if ( oFirst.GetR() != oAgain.GetR() || oFirst.GetG() != oAgain.GetG() || oFirst.GetB() != oAgain.GetB() )
            bRepeats = false;
        if ( oFirst.GetR() != oRgbSeen.GetR() || oFirst.GetB() != oRgbSeen.GetB() )
            bDiffers = true;
        if ( oClearOne.GetR() != oClearTwo.GetR() || oClearOne.GetG() != oClearTwo.GetG() || oClearOne.GetB() != oClearTwo.GetB() )
            bSame = false;
    }
    Check( bRepeats, "spectral dispersion picks the same wavelengths every time" );
    Check( bDiffers, "spectral dispersion sees differently from red, green and blue" );
    Check( bSame, "without dispersive glass, a spectral camera sees exactly what an RGB one does" );

    CScene oSaved;

    Check( oSaved.FromJson( oSpectral.ToJson() ) == 0 && oSaved.GetCamera().IsSpectral() &&
           oSpectral.ToJson().find( "\"dispersion\": \"spectral\"" ) != std::string::npos &&
           oRgb.ToJson().find( "dispersion" ) == std::string::npos, "\"dispersion\" round-trips, and rgb isn't written" );
    ExpectError( Minimal( "\"lookAt\": [0, 0, 0] }", "\"lookAt\": [0, 0, 0], \"dispersion\": \"rainbow\" }" ),
                 BAD_FILE, "unknown dispersion \"rainbow\" (rgb or spectral)" );
}

static void LeanMeshChecks( void )
{
        // Two scenes with the same mesh share one geometry, which goes when
        // the last mesh using it does.
    const char *strMesh = "{ \"type\": \"mesh\", \"doubleSided\": true, \"vertices\": [[-1, -1, 0], [1, -1, 0], [0, 1, 0], [0, 0, -1]], "
                          "\"faces\": [[0, 2, 1], [0, 1, 3], [1, 2, 3], [2, 0, 3]] }";
    int         nBefore = CMesh::SharedGeometries();

    {
        CScene  oFirst, oSecond, oOther;
        CMesh  *pFirst = dynamic_cast<CMesh *>( FirstObject( oFirst, strMesh ) );
        CMesh  *pSecond = dynamic_cast<CMesh *>( FirstObject( oSecond, strMesh ) );
        CMesh  *pOther = dynamic_cast<CMesh *>( FirstObject( oOther, "{ \"type\": \"mesh\", \"vertices\": [[-1, -1, 0], [1, -1, 0], [0, 1, 0]], "
                                                                       "\"faces\": [[0, 2, 1]] }" ) );

        Check( pFirst && pSecond && pOther && pFirst->GetGeometry() == pSecond->GetGeometry() &&
               pFirst->GetGeometry() != pOther->GetGeometry(), "meshes with the same shape share one geometry, others don't" );
        Check( CMesh::SharedGeometries() == nBefore + 2, "each shape is built once" );

            // Threads loading copies of a scene at once share it too.
        std::string             strText = oFirst.ToJson();
        const CMesh::SGeometry *apSeen[ 8 ] = { NULL };
        std::vector<std::thread> aoThreads;
        std::vector<std::unique_ptr<CScene>> apScenes( 8 );

        for ( int nThread = 0; nThread < 8; nThread++ )
        {
            aoThreads.push_back( std::thread( [ &, nThread ]()
            {
                apScenes[ (size_t)nThread ].reset( new CScene );
                if ( apScenes[ (size_t)nThread ]->FromJson( strText ) == 0 )
                {
                    CMesh *pMesh = dynamic_cast<CMesh *>( apScenes[ (size_t)nThread ]->GetObjectAt( 0 ) );

                    apSeen[ nThread ] = pMesh ? pMesh->GetGeometry() : NULL;
                }
            } ) );
        }
        for ( size_t nThread = 0; nThread < aoThreads.size(); nThread++ )
            aoThreads[ nThread ].join();

        bool bShared = true;

        for ( int nThread = 0; nThread < 8; nThread++ )
            bShared = bShared && ( apSeen[ nThread ] == pFirst->GetGeometry() );
        Check( bShared, "every thread's copy of a scene shares the mesh's geometry" );

        if ( pFirst )
        {
                // A face's stand-in: the same one each time a csg asks, rays
                // leaving it skip only that face, and two stand-ins for one
                // face are one surface.
            CRay    oRay = RayFrom( 0.0, -0.2, -5.0, 0.0, 0.0, 1.0 );
            CVector oHit;
            BOOL    bFlip;

            Check( pFirst->Intersect( oRay, oHit ) == TRUE, "a ray hits the lean mesh" );

            CObjects *pPart = pFirst->HitPart( bFlip );

            Check( pPart != pFirst && pFirst->HitPart( bFlip ) == pPart && pPart->MeshFaceOf( pFirst ) >= 0 &&
                   pPart->MeshFaceOf( pSecond ) == -1, "a mesh's hit part is a stand-in for its face, the same each time" );

            CRay oOn = RayFrom( 0.0, -0.2, -5.0, 0.0, 0.0, 1.0 );

            Check( pFirst->Intersect( oOn, oHit, pPart ) == TRUE && oOn.GetT() > oRay.GetT(),
                   "a ray ignoring a face goes on to the mesh's next face" );
            Check( pFirst->Intersect( oOn, oHit, pFirst ) == FALSE, "a ray ignoring the mesh misses it" );
        }
    }
    Check( CMesh::SharedGeometries() == nBefore, "a geometry goes when the last mesh using it does" );
}

static void ClampChecks( void )
{
    CScene oScene;

    Check( oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"clamp\": 2" ) ) == 0, "a camera with a clamp loads" );

    CCamera &oCamera = oScene.GetCamera();
    CColor   oBright( 5.0, 0.5, 2.0 );
    CColor   oClamped = oCamera.ClampSample( oBright );

    Check( oCamera.GetClamp() == 2.0 && oClamped.GetR() == 2.0 && oClamped.GetG() == 0.5 && oClamped.GetB() == 2.0,
           "the sample clamp caps each channel" );
    Check( oScene.ToJson().find( "\"clamp\": 2" ) != std::string::npos, "the clamp is saved" );

    CScene oPlain;

    Check( oPlain.FromJson( s_strMinimal ) == 0, "the minimal scene loads" );

    CColor oKept = oPlain.GetCamera().ClampSample( oBright );

    Check( oPlain.GetCamera().GetClamp() == 0.0 && oKept.GetR() == 5.0 && oPlain.ToJson().find( "clamp" ) == std::string::npos,
           "without a clamp samples are left as they are, and none is saved" );

    CCamera oCopy( oCamera );

    Check( oCopy.GetClamp() == 2.0, "a copied camera keeps its clamp" );
    ExpectError( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"clamp\": -1" ), BAD_FILE, "camera: clamp can't be negative" );
}

// --------------------------------------------------------------------------
// Model files (STL, OBJ), their placement, and the layers and faceColors
// textures
// --------------------------------------------------------------------------

static void PutLittle32( std::string &strOut, uint32_t nValue )
{
    for ( int nShift = 0; nShift < 32; nShift += 8 )
        strOut += (char)( ( nValue >> nShift ) & 0xFF );
}

static void PutLittleFloat( std::string &strOut, float fValue )
{
    uint32_t nBits;

    memcpy( &nBits, &fValue, sizeof( nBits ) );
    PutLittle32( strOut, nBits );
}

    // A binary STL of the triangles (nine numbers each) and attributes.
static std::string MakeStl( const std::string &strHeader, const std::vector<float> &afCorners, const std::vector<unsigned> &anAttributes )
{
    std::string strOut = strHeader;

    strOut.resize( 80, '\0' );
    PutLittle32( strOut, (uint32_t)anAttributes.size() );
    for ( size_t nFace = 0; nFace < anAttributes.size(); nFace++ )
    {
        for ( int nIndex = 0; nIndex < 3; nIndex++ )
            PutLittleFloat( strOut, 0.0f );
        for ( int nIndex = 0; nIndex < 9; nIndex++ )
            PutLittleFloat( strOut, afCorners[ ( nFace * 9 ) + (size_t)nIndex ] );
        strOut += (char)( anAttributes[ nFace ] & 0xFF );
        strOut += (char)( ( anAttributes[ nFace ] >> 8 ) & 0xFF );
    }
    return strOut;
}

static std::string Base64( const std::string &strData )
{
    static const char *strDigits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string        strOut;

    for ( size_t nAt = 0; nAt < strData.size(); nAt += 3 )
    {
        uint32_t nGroup = ( (BYTE)strData[ nAt ] ) << 16;
        size_t   nHave = ( std::min )( (size_t)3, strData.size() - nAt );

        if ( nHave > 1 )
            nGroup |= ( (BYTE)strData[ nAt + 1 ] ) << 8;
        if ( nHave > 2 )
            nGroup |= (BYTE)strData[ nAt + 2 ];
        strOut += strDigits[ ( nGroup >> 18 ) & 63 ];
        strOut += strDigits[ ( nGroup >> 12 ) & 63 ];
        strOut += ( nHave > 1 ) ? strDigits[ ( nGroup >> 6 ) & 63 ] : '=';
        strOut += ( nHave > 2 ) ? strDigits[ nGroup & 63 ] : '=';
    }
    return strOut;
}

static bool Near( double dA, double dB, double dTolerance = 1e-9 )
{
    return fabs( dA - dB ) <= dTolerance;
}

static bool NearVector( CVector oA, CVector oB, double dTolerance = 1e-9 )
{
    return Near( oA.GetX(), oB.GetX(), dTolerance ) && Near( oA.GetY(), oB.GetY(), dTolerance ) && Near( oA.GetZ(), oB.GetZ(), dTolerance );
}

    // A scene with one mesh, the model file inside it as "data", and the
    // mesh's other fields strFields.
static std::string ModelScene( const std::string &strName, const std::string &strData, const std::string &strFields )
{
    return Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                    ( "{ \"type\": \"mesh\", \"file\": \"" + strName + "\", \"data\": \"" + Base64( strData ) + "\"" +
                      ( strFields.empty() ? "" : ", " + strFields ) + " }" ).c_str() );
}

static void ModelChecks( void )
{
        // Two triangles sharing an edge: a unit square in z = 0, facing +z.
    std::vector<float>  afSquare = { 0, 0, 0,  1, 0, 0,  1, 1, 0,    0, 0, 0,  1, 1, 0,  0, 1, 0 };
    std::string         strError;

    {
            // A binary file whose header starts "solid", as many do.
        ModelFile::SModel oModel;
        std::string       strStl = MakeStl( "solid but binary", afSquare, { 0, 0 } );

        Check( ModelFile::Read( strStl, "square.stl", oModel, strError ) == TRUE, "a binary STL reads" );
        Check( oModel.anFaces.size() == 6 && oModel.aoVertices.size() == 4, "a binary STL beginning \"solid\" is read as binary, and its shared corners joined" );
        Check( oModel.aoFaceColors.empty(), "a binary STL without colors has none" );
        Check( ModelFile::Read( strStl, "square.model", oModel, strError ) == TRUE && oModel.anFaces.size() == 6,
               "a binary STL is recognized by its size whatever its name" );
        Check( ModelFile::Read( strStl.substr( 0, strStl.size() - 1 ), "square.stl", oModel, strError ) == FALSE &&
               strError.find( "size doesn't match" ) != std::string::npos, "a binary STL a byte short is refused" );
    }
    {
        ModelFile::SModel oModel;
        std::string       strAscii = "solid square\n"
                                     " facet normal 0 0 1\n  outer loop\n   vertex 0 0 0\n   vertex 1 0 0\n   vertex 1 1 0\n  endloop\n endfacet\n"
                                     " FACET NORMAL 0 0 1\n  OUTER LOOP\n   VERTEX 0 0 0\n   VERTEX 1 1 0\n   VERTEX 0 1 0\n  ENDLOOP\n ENDFACET\n"
                                     "endsolid square\n";

        Check( ModelFile::Read( strAscii, "square.stl", oModel, strError ) == TRUE && oModel.anFaces.size() == 6 && oModel.aoVertices.size() == 4,
               "an ASCII STL reads, in either case, its shared corners joined" );
        Check( ModelFile::Read( "solid x\n facet normal 0 0 1\n outer loop\n vertex 0 0 0\n vertex 1 0 0\n endloop\n endfacet\nendsolid\n",
                                "bad.stl", oModel, strError ) == FALSE && strError.find( "doesn't have three vertices" ) != std::string::npos,
               "an ASCII facet with two vertices is refused" );
        Check( ModelFile::Read( "solid x\n facet normal 0 0 1\n outer loop\n vertex 0 zero 0\n", "bad.stl", oModel, strError ) == FALSE &&
               strError.find( "doesn't have three numbers" ) != std::string::npos, "an ASCII vertex that isn't numbers is refused" );
        Check( ModelFile::Read( "solid empty\nendsolid empty\n", "empty.stl", oModel, strError ) == FALSE &&
               strError.find( "no triangles" ) != std::string::npos, "an STL with no triangles is refused" );
    }
    {
            // VisCAM and SolidView: bit 15 set, red in the high bits.
        ModelFile::SModel oModel;
        std::string       strStl = MakeStl( "colored", afSquare, { 0x8000u | ( 31u << 10 ) | ( 16u << 5 ), 0 } );

        Check( ModelFile::Read( strStl, "c.stl", oModel, strError ) == TRUE && oModel.aoFaceColors.size() == 2, "a VisCAM-colored STL reads" );
        Check( oModel.abHasColor[ 0 ] && Near( oModel.aoFaceColors[ 0 ].GetR(), 1.0 ) && Near( oModel.aoFaceColors[ 0 ].GetG(), 16.0 / 31.0 ) &&
               Near( oModel.aoFaceColors[ 0 ].GetB(), 0.0 ), "a VisCAM color: red in bits 10-14, green 5-9, blue 0-4" );
        Check( ! oModel.abHasColor[ 1 ], "a VisCAM face with bit 15 clear has no color" );
    }
    {
            // Materialise Magics: COLOR= in the header, bit 15 clear for a
            // face's own color, red in the low bits.
        ModelFile::SModel oModel;
        std::string       strHeader = std::string( "Magics COLOR=" ) + (char)255 + (char)102 + (char)0 + (char)255;
        std::string       strStl = MakeStl( strHeader, afSquare, { 31u | ( 8u << 10 ), 0x8000u } );

        Check( ModelFile::Read( strStl, "m.stl", oModel, strError ) == TRUE && oModel.aoFaceColors.size() == 2, "a Magics-colored STL reads" );
        Check( oModel.abHasColor[ 0 ] && Near( oModel.aoFaceColors[ 0 ].GetR(), 1.0 ) && Near( oModel.aoFaceColors[ 0 ].GetB(), 8.0 / 31.0 ),
               "a Magics face's own color: red in bits 0-4, blue 10-14" );
        Check( oModel.abHasColor[ 1 ] && Near( oModel.aoFaceColors[ 1 ].GetR(), 1.0 ) && Near( oModel.aoFaceColors[ 1 ].GetG(), 102.0 / 255.0 ) &&
               Near( oModel.aoFaceColors[ 1 ].GetB(), 0.0 ), "a Magics face with bit 15 set takes the header's model color" );
    }
    {
        ModelFile::SModel oModel;
        std::string       strObj = "# a square\nmtllib x.mtl\no square\ng face\ns 1\nusemtl red\n"
                                   "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0 1.0\n"
                                   "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 2\n"
                                   "f 1/1/1 2/2/1 3/3/1 4/4/1\n";

        Check( ModelFile::Read( strObj, "square.obj", oModel, strError ) == TRUE, "an OBJ reads" );
        Check( oModel.anFaces.size() == 6 && oModel.aoVertices.size() == 4, "an OBJ quad becomes two triangles" );
        Check( oModel.anUVs.size() == 8 && oModel.aoNormals.size() == 4 && NearVector( oModel.aoNormals[ 0 ], CVector( 0, 0, 1 ) ),
               "an OBJ's texture coordinates and normals (made unit) come with its vertices" );

        ModelFile::SModel oRelative;
        std::string       strNegative = "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 1\n"
                                        "f -4/-4/-1 -3/-3/-1 -2/-2/-1 -1/-1/-1\n";

        Check( ModelFile::Read( strNegative, "square.obj", oRelative, strError ) == TRUE && oRelative.anFaces == oModel.anFaces &&
               oRelative.anUVs == oModel.anUVs, "negative OBJ indices count back from the last one read" );

        ModelFile::SModel oMixed;

        Check( ModelFile::Read( "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nf 1/1 2/1 3/1\nf 1 3 4\n", "mixed.obj", oMixed, strError ) == TRUE &&
               oMixed.anUVs.empty() && oMixed.aoNormals.empty() && oMixed.aoVertices.size() == 4,
               "texture coordinates only some corners have are dropped" );
        Check( ModelFile::Read( "v 0 0 0\nv 1 0 0\nf 1 2 3\n", "bad.obj", oMixed, strError ) == FALSE &&
               strError.find( "line 3" ) != std::string::npos && strError.find( "not yet given" ) != std::string::npos,
               "an OBJ face naming a vertex not yet given is refused, with its line" );
        Check( ModelFile::Read( "v 0 0\n", "bad.obj", oMixed, strError ) == FALSE && strError.find( "three numbers" ) != std::string::npos,
               "an OBJ vertex without three numbers is refused" );
        Check( ModelFile::Read( "v 0 0 0\n", "bad.obj", oMixed, strError ) == FALSE && strError.find( "no faces" ) != std::string::npos,
               "an OBJ without faces is refused" );
        Check( ModelFile::Read( strObj, "square", oMixed, strError ) == TRUE && oMixed.anFaces.size() == 6,
               "a file named neither .stl nor .obj that isn't an STL is read as OBJ" );
    }

        // Placement: (1, 0, 0) scaled by 2, turned 90 degrees about z, then
        // moved by (1, 2, 3), lands at (1, 4, 3).
    std::string strTriangle = MakeStl( "one", { 0, 0, 0,  1, 0, 0,  0, 1, 0 }, { 0 } );

    {
        CScene oScene;

        Check( oScene.FromJson( ModelScene( "tri.stl", strTriangle, "\"scale\": 2, \"rotation\": [0, 0, 90], \"translation\": [1, 2, 3]" ) ) == 0,
               "a mesh loads a model file carried as data" );

        CMesh *pMesh = dynamic_cast<CMesh *>( oScene.GetObjectAt( 0 ) );

        Check( pMesh && pMesh->IsFromFile() && pMesh->GetVertices().size() == 3, "the model's vertices load" );
        if ( pMesh && pMesh->GetVertices().size() == 3 )
        {
            Check( NearVector( pMesh->GetVertices()[ 0 ], CVector( 1, 2, 3 ) ) && NearVector( pMesh->GetVertices()[ 1 ], CVector( 1, 4, 3 ) ) &&
                   NearVector( pMesh->GetVertices()[ 2 ], CVector( -1, 2, 3 ) ), "a model is scaled, then turned, then moved" );

                // The faces look textures up back in the file's coordinates.
            CRay    oRay;
            CVector oHit;
            CVector oTarget( 1.0 - ( 2.0 / 3.0 ), 2.0 + ( 2.0 / 3.0 ), 3.0 );      // the file point (1/3, 1/3, 0)

            oRay.Set( oTarget.GetX(), oTarget.GetY(), oTarget.GetZ() + 5.0, 0.0, 0.0, -1.0, 1e100, 1.0, 0, 0 );
            Check( pMesh->Intersect( oRay, oHit ) == TRUE, "a ray hits the placed model" );

            BOOL      bFlip;
            CObjects *pFace = pMesh->HitPart( bFlip );

            Check( NearVector( pFace->TextureSpace( oHit ), CVector( 1.0 / 3.0, 1.0 / 3.0, 0.0 ), 1e-9 ),
                   "a placed model's textures are looked up at the point in its file" );
        }

        std::string strSaved = oScene.ToJson();

        Check( strSaved.find( "\"file\": \"tri.stl\"" ) != std::string::npos && strSaved.find( "\"vertices\"" ) == std::string::npos &&
               strSaved.find( "\"rotation\": [0.0, 0.0, 90.0]" ) != std::string::npos, "a model saves as its file and placement, not its vertices" );
    }
    {
        CScene      oScene;
        std::string strHigh = MakeStl( "high", { 0, 5, 0,  4, 5, 0,  0, 7, 2 }, { 0 } );

        Check( oScene.FromJson( ModelScene( "h.stl", strHigh, "\"align\": \"bottom\"" ) ) == 0, "a model aligned to the bottom loads" );

        CMesh *pMesh = dynamic_cast<CMesh *>( oScene.GetObjectAt( 0 ) );

        if ( pMesh && pMesh->GetVertices().size() == 3 )
            Check( NearVector( pMesh->GetVertices()[ 0 ], CVector( -2, 0, -1 ) ) && NearVector( pMesh->GetVertices()[ 2 ], CVector( -2, 2, 1 ) ),
                   "align bottom centers a model in x and z and stands it on y = 0" );
        Check( oScene.FromJson( ModelScene( "h.stl", strHigh, "\"align\": \"center\", \"fitSize\": 2" ) ) == 0, "a fitted model loads" );
        pMesh = dynamic_cast<CMesh *>( oScene.GetObjectAt( 0 ) );
        if ( pMesh && pMesh->GetVertices().size() == 3 )
            Check( NearVector( pMesh->GetVertices()[ 0 ], CVector( -1, -0.5, -0.5 ) ) && NearVector( pMesh->GetVertices()[ 1 ], CVector( 1, -0.5, -0.5 ) ),
                   "fitSize scales a model evenly to that size across its widest, and align center centers it" );

            // Mirrored, the corners are put back the other way round, so the
            // face still faces the way it did.
        Check( oScene.FromJson( ModelScene( "tri.stl", strTriangle, "\"scale\": [-1, 1, 1]" ) ) == 0, "a mirrored model loads" );
        pMesh = dynamic_cast<CMesh *>( oScene.GetObjectAt( 0 ) );
        if ( pMesh && pMesh->GetFaceCount() == 1 )
        {
            CVector oPoint( -0.25, 0.25, 0.0 );

            Check( pMesh->FaceNormalAt( 0, oPoint ).GetZ() > 0.99, "a mirroring scale keeps a model's faces facing out" );
        }
    }
    {
        CScene      oScene;
        std::string strColored = MakeStl( "c", afSquare, { 0x8000u | ( 31u << 5 ), 0 } );

        Check( oScene.FromJson( ModelScene( "c.stl", strColored, "" ) ) == 0, "a colored model loads" );

        CMesh *pMesh = dynamic_cast<CMesh *>( oScene.GetObjectAt( 0 ) );
        CColor oColor;

        Check( pMesh && pMesh->GetFaceCount() == 2 && pMesh->FaceColor( 0, oColor ) && Near( oColor.GetG(), 1.0 ) && Near( oColor.GetR(), 0.0 ) &&
               ! pMesh->FaceColor( 1, oColor ), "a colored model's faces carry their colors" );
    }

    ExpectError( Minimal( "\"radius\": 1 }", "\"radius\": 1 }, { \"type\": \"mesh\", \"vertices\": [[0,0,0],[1,0,0],[0,1,0]], \"faces\": [[0,1,2]], \"scale\": 2 }" ),
                 BAD_FILE, "\"scale\" places a model loaded from a \"file\"" );
    ExpectError( ModelScene( "tri.stl", strTriangle, "\"faces\": [[0, 1, 2]]" ), BAD_FILE, "not \"faces\" with a file" );
    ExpectError( ModelScene( "tri.stl", strTriangle, "\"scale\": 2, \"fitSize\": 1" ), BAD_FILE, "a \"scale\" or a \"fitSize\", not both" );
    ExpectError( ModelScene( "tri.stl", strTriangle, "\"fitSize\": 0" ), BAD_FILE, "fitSize must be more than 0" );
    ExpectError( ModelScene( "tri.stl", strTriangle, "\"scale\": [1, 0, 1]" ), BAD_FILE, "can't be 0" );
    ExpectError( ModelScene( "tri.stl", strTriangle, "\"align\": \"top\"" ), BAD_FILE, "unknown align \"top\"" );
    ExpectError( ModelScene( "tri.stl", "solid nothing\n", "" ), BAD_FILE, "the model \"tri.stl\" has no triangles" );
    ExpectError( Minimal( "\"radius\": 1 }", "\"radius\": 1 }, { \"type\": \"mesh\", \"file\": \"no/such/model.stl\" }" ),
                 BAD_FILE, "can't open the model \"no/such/model.stl\"" );

        // The layers texture: rounded beads, 0 at the seams and 1 between.
    Check( CLayersTexture::Bead( 0.0 ) == 0.0 && CLayersTexture::Bead( 0.5 ) == 1.0 && Near( CLayersTexture::Bead( 0.25 ), 0.75 ) &&
           Near( CLayersTexture::Bead( 1.0 ), 0.0 ), "a layer is a parabolic bead, 0 at its seams and 1 in its middle" );
    {
        CLayersTexture  oLayers;
        STexturePoint   oAt;
        CColor          oMiddle, oSeam;

        oAt.nU = oAt.nV = 0.0;
        oAt.bHasUV = FALSE;
        oAt.bHasColor = FALSE;
        oAt.oPoint.Set( 3.0, -2.0, 0.05 );
        oMiddle = oLayers.ColorAt( oAt );
        oAt.oPoint.Set( 3.0, -2.0, 0.2 );
        oSeam = oLayers.ColorAt( oAt );
        Check( Near( oMiddle.GetR(), 1.0 ) && Near( oSeam.GetR(), 0.7 ), "the default layers are 0.1 apart along z, light gray darkening to the seams" );

        std::unique_ptr<CTexture> pRead( CTexture::Create( "layers" ) );
        IASLib::CObjectNode       oNode;

        oLayers.ToJson( oNode );
        Check( pRead && pRead->FromJson( oNode ), "layers round-trip through JSON" );
        ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"diffuse\", \"texture\": { \"type\": \"layers\", \"axis\": [0, 0, 0] } } ]" ),
                     BAD_FILE, "axis can't be [0, 0, 0]" );

        CFaceColorTexture oFaces;

        oAt.bHasColor = TRUE;
        oAt.oColor.Set( 0.1, 0.2, 0.3 );
        Check( Near( oFaces.ColorAt( oAt ).GetB(), 0.3 ), "faceColors shows a face's own color" );
        oAt.bHasColor = FALSE;
        Check( Near( oFaces.ColorAt( oAt ).GetR(), 0.8 ), "faceColors shows its default where a face has no color" );
    }
}

    // Editing one part of a scene as JSON, as the scene editor does.
static void SceneEditChecks( void )
{
    CScene oScene;

    Check( oScene.FromJson( s_strMinimal ) == 0, "the minimal scene loads" );

    std::string strSphere = oScene.PartToJson( "objects/0" );

    Check( strSphere.find( "\"type\": \"sphere\"" ) != std::string::npos && strSphere.find( "\"radius\": 1.0" ) != std::string::npos,
           "an object's part of the scene comes out as its JSON" );
    Check( oScene.PartToJson( "objects/1" ).empty() && oScene.PartToJson( "environment" ).empty() && oScene.PartToJson( "nonsense" ).empty(),
           "a part the scene doesn't have comes out empty" );

    Check( oScene.EditPart( "objects/0", "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 2 }", "" ) == 0 &&
           oScene.GetObjectCount() == 1 && dynamic_cast<CBox *>( oScene.GetObjectAt( 0 ) ) != NULL, "an object can be replaced by one of another type" );
    Check( oScene.EditPart( "objects/+", strSphere, "" ) == 0 && oScene.GetObjectCount() == 2 && dynamic_cast<CSphere *>( oScene.GetObjectAt( 1 ) ) != NULL,
           "an object can be added" );
    Check( oScene.IndexOfObject( oScene.GetObjectAt( 1 ) ) == 1 && oScene.IndexOfObject( NULL ) == -1, "an object's place is found" );

    std::string strBefore = oScene.ToJson();

    Check( oScene.EditPart( "objects/0", "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": -1 }", "" ) == BAD_FILE &&
           strstr( oScene.GetLoadError(), "size" ) != NULL && oScene.ToJson() == strBefore, "a bad edit is reported and changes nothing" );
    Check( oScene.EditPart( "objects/0", "{ \"type\": \"spot\", \"position\": [0, 0, 0], \"direction\": [0, -1, 0], \"color\": [1, 1, 1] }", "" ) == BAD_FILE &&
           oScene.ToJson() == strBefore, "a light can't go in the objects" );
    Check( oScene.EditPart( "objects/7", strSphere, "" ) == BAD_FILE && oScene.EditPart( "objects/x", strSphere, "" ) == BAD_FILE &&
           oScene.ToJson() == strBefore, "an object the scene doesn't have can't be edited" );
    Check( oScene.EditPart( "objects/0", "{ \"type\": ", "" ) == BAD_FILE && oScene.ToJson() == strBefore, "text that isn't JSON is refused" );

    Check( oScene.EditPart( "lights/+", "{ \"type\": \"directional\", \"direction\": [0, -1, 0], \"color\": [0.5, 0.5, 0.5] }", "" ) == 0 &&
           oScene.GetLightCount() == 2, "a light can be added" );
    Check( oScene.EditPart( "lights/0", "", "" ) == 0 && oScene.GetLightCount() == 1, "empty text removes a light" );
    Check( oScene.EditPart( "objects/0", "", "" ) == 0 && oScene.GetObjectCount() == 1 && dynamic_cast<CSphere *>( oScene.GetObjectAt( 0 ) ) != NULL,
           "empty text removes an object" );

    Check( oScene.EditPart( "camera", "{ \"position\": [0, 0, -5], \"lookAt\": [0, 0, 0], \"sampler\": \"corners\", \"clamp\": 2 }", "" ) == 0 &&
           strcmp( oScene.GetCamera().GetSampler().GetName(), "corners" ) == 0 && oScene.GetCamera().GetClamp() == 2.0,
           "the camera can be edited, sampler and all" );
    Check( oScene.EditPart( "camera", "", "" ) == BAD_FILE, "the camera can't be left out" );
    Check( oScene.EditPart( "environment", "{ \"type\": \"sky\" }", "" ) == 0 && oScene.GetEnvironment() != NULL, "an environment can be added" );
    Check( oScene.EditPart( "environment", " \n", "" ) == 0 && oScene.GetEnvironment() == NULL, "empty text removes the environment" );
    Check( oScene.EditPart( "settings", "{ \"imageWidth\": 99 }", "" ) == 0 && oScene.GetSettings().GetImageWidth() == 99, "the settings can be edited" );
}

    // The model files in scenes/models: Benchy, the colored STLs and the OBJ
    // ring, the mesh tree on Benchy, and a scene's models going into an
    // archive and out beside a JSON file.
static void ModelFileChecks( const char *strScenes, const char *strDir )
{
    CScene oBenchy;

    Check( oBenchy.Load( ( std::string( strScenes ) + "/benchy_test.ray" ).c_str() ) == 0, "the Benchy scene loads" );

    CMesh *pBenchy = dynamic_cast<CMesh *>( oBenchy.GetObjectAt( 1 ) );

    Check( pBenchy != NULL && pBenchy->GetFaceCount() > 225000, "Benchy's 225,706 triangles load (less any with no area)" );
    if ( pBenchy )
    {
        CVector oMinimum, oMaximum;

        pBenchy->GetBounds( oMinimum, oMaximum );
        Check( Near( oMinimum.GetY(), 0.0, 1e-3 ) && Near( oMaximum.GetY(), 48.0, 1e-3 ),
               "Benchy, turned from z up to y up and aligned to the bottom, stands 48 mm tall on y = 0" );

            // The tree finds exactly what testing every face finds.
        unsigned nSeed = 777;
        int      nHits = 0;
        int      nDiffer = 0;

        for ( int nRay = 0; nRay < 60; nRay++ )
        {
            double adValue[ 4 ];

            for ( int nIndex = 0; nIndex < 4; nIndex++ )
            {
                nSeed = ( nSeed * 1103515245u ) + 12345u;
                adValue[ nIndex ] = ( ( nSeed >> 8 ) & 0xFFFF ) / 65535.0;
            }

            double  dAngle = adValue[ 0 ] * 6.283185307179586;
            CVector oFrom( 100.0 * cos( dAngle ), 60.0 * adValue[ 1 ], 100.0 * sin( dAngle ) );
            CVector oTo( ( adValue[ 2 ] * 50.0 ) - 25.0, adValue[ 3 ] * 45.0, 0.0 );
            CVector oDirection = oTo - oFrom;
            CRay    oTree, oEach;
            CVector oTreeHit, oEachHit;

            oDirection.Normalize();
            oTree.Set( oFrom.GetX(), oFrom.GetY(), oFrom.GetZ(), oDirection.GetX(), oDirection.GetY(), oDirection.GetZ(), 1e100, 1.0, 0, 0 );
            oEach = oTree;

            BOOL bTree = pBenchy->Intersect( oTree, oTreeHit );
            BOOL bEach = pBenchy->IntersectEachFace( oEach, oEachHit );

            if ( bTree )
                nHits++;
            if ( bTree != bEach || oTree.GetT() != oEach.GetT() || ! NearVector( oTreeHit, oEachHit, 0.0 ) )
                nDiffer++;
        }
        Check( nHits > 20, "many rays at Benchy hit it" );
        Check( nDiffer == 0, "Benchy's tree finds exactly the hit testing every face finds" );
    }

    CScene oModels;

    Check( oModels.Load( ( std::string( strScenes ) + "/models_test.ray" ).c_str() ) == 0, "the models scene loads" );

    CMesh *pCube = dynamic_cast<CMesh *>( oModels.GetObjectAt( 1 ) );
    CMesh *pGem = dynamic_cast<CMesh *>( oModels.GetObjectAt( 2 ) );
    CMesh *pRing = dynamic_cast<CMesh *>( oModels.GetObjectAt( 3 ) );
    CColor oColor;

    Check( pCube && pCube->GetFaceCount() == 588 && pCube->FaceColor( 0, oColor ), "the puzzle cube's 588 faces load with their VisCAM colors" );
    Check( pGem && pGem->GetFaceCount() == 8 && pGem->FaceColor( 1, oColor ) && Near( oColor.GetR(), 230.0 / 255.0 ),
           "the gem's faces without colors of their own take its Magics model color" );
    Check( pRing && pRing->GetFaceCount() == 32 * 16 * 2 && pRing->HasUVs(), "the OBJ ring's quads load as triangles, with texture coordinates" );

    std::string strArchive = std::string( strDir ) + "/models_archive.ray";
    std::string strJsonDir = std::string( strDir ) + "/models_json";
    std::string strJson = strJsonDir + "/scene.ray";

    Check( oModels.Save( strArchive.c_str() ) == 0, "a scene with model files saves as an archive" );
    Check( oModels.SaveJson( strJson.c_str() ) == 0, "it saves as JSON" );
    Check( ReadFile( strJsonDir + "/models/color_cube.stl" ) == ReadFile( std::string( strScenes ) + "/models/color_cube.stl" ) &&
           ReadFile( strJsonDir + "/models/ring.obj" ) == ReadFile( std::string( strScenes ) + "/models/ring.obj" ),
           "the model files are extracted beside the JSON, unchanged" );

    CScene oArchived;

        // The archive is away from scenes/, so its models must come from inside it.
    Check( oArchived.Load( strArchive.c_str() ) == 0, "the archive loads with its model files from inside it" );
    Check( oArchived.ToJson() == oModels.ToJson(), "the archived scene reads back the same" );
    remove( strArchive.c_str() );
}

int main( int argc, char **argv )
{
    if ( argc >= 4 && strcmp( argv[ 1 ], "--engines" ) == 0 )
        EngineChecks( argv[ 2 ], argv[ 3 ] );
    else if ( argc >= 4 && strcmp( argv[ 1 ], "--textures" ) == 0 )
    {
        TextureChecks( argv[ 2 ] );
        BrickTextureChecks( argv[ 2 ] );
        SkyTextureChecks( argv[ 2 ] );
        ArchiveChecks( argv[ 2 ], argv[ 3 ] );
        MeshTreeChecks( argv[ 2 ] );
        SceneTreeChecks( argv[ 2 ] );
        LensTableChecks( argv[ 2 ] );
        ModelFileChecks( argv[ 2 ], argv[ 3 ] );
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
        SoftChecks();
        PrimitiveChecks();
        HdrChecks();
        PngChecks();
        BumpChecks();
        LightChecks();
        EnvironmentChecks();
        CsgChecks();
        DispersionChecks();
        SpectralChecks();
        LeanMeshChecks();
        ClampChecks();
        ModelChecks();
        SceneEditChecks();
    }

    if ( g_nFailures == 0 )
        printf( "All checks passed\n" );
    return ( g_nFailures == 0 ) ? 0 : 1;
}
