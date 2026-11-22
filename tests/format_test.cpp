// Checks that rays, colors and scenes cross the wire exactly, that JSON
// scenes are read and checked properly, and that scene files survive a load
// and save unchanged.
//
//   format_test                   the wire and JSON checks
//   format_test <scene.ray> <dir> load, save, load, save; the two saves must match
#include "Globals.h"
#include "Decompress.h"
#include "DracoDecoder.h"
#include "ModelFile.h"
#include "WireProtocol.h"
#include "Mesh.h"
#include "Cone.h"
#include "Box.h"
#include "Sphere.h"
#include "ImageExport.h"
#include "HdrColorModel.h"
#include "Lens.h"
#include "FilmColorModel.h"
#include "RegolithMaterial.h"
#include "SrgbColorModel.h"
#include "Camera.h"
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
#include "JpegImage.h"
#include "WebpImage.h"
#include "BumpMap.h"
#include "Csg.h"
#include "ModelFile.h"
#include "LayersTexture.h"
#include "FaceColorTexture.h"
#include "Spectrum.h"
#include "Maps.h"
#include "Presets.h"
#include "CellsTexture.h"
#include "CausticsTexture.h"
#include "CheckerTexture.h"
#include "Light.h"
#include "LocalShape.h"
#include "FontFile.h"
#include "Outline.h"
#include "Text.h"
#include "Bezier.h"
#include "BSpline.h"
#include "Lathe.h"
#include "Expression.h"
#include "Implicit.h"
#include "Blob.h"
#include "MetalMaterial.h"
#include "RayCountMap.h"
#include "Fractal.h"
#include "Extrusion.h"
#include "Sweep.h"
#include "LensElements.h"
#include <map>


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
    CRayCounts          oCounts, oReadCounts;
    std::vector<double> anColors, anRead;

    for ( int nKind = 0; nKind < CRayCounts::KINDS; nKind++ )
        oCounts.anCounts[ nKind ] = 0x123456789ull * (uint64_t)( nKind + 1 );

    Check( RayWire::DecodeHello( RayWire::EncodeHello() ), "the hello reads back" );

    Check( RayWire::DecodeTile( RayWire::EncodeTile( 42, oTile ), nTrace, oRead ) && nTrace == 42 && oRead.nLeft == 64 &&
           oRead.nTop == 96 && oRead.nWidth == 7 && oRead.nHeight == 5, "a tile arrives exactly" );

    for ( int nIndex = 0; nIndex < 7 * 5 * 3; nIndex++ )
        anColors.push_back( adValues[ nIndex % nValues ] + nIndex * 1e-9 );
    Check( RayWire::DecodePixels( RayWire::EncodePixels( 43, oTile, oCounts, anColors ), nTrace, oRead, oReadCounts, anRead ) &&
           nTrace == 43 && oReadCounts == oCounts && oRead.nWidth == 7 && anRead.size() == anColors.size(),
           "a tile's pixels and its ray counts by kind decode" );

    bool bExact = anRead.size() == anColors.size();

    for ( size_t nIndex = 0; bExact && nIndex < anColors.size(); nIndex++ )
        bExact = ( memcmp( &anRead[ nIndex ], &anColors[ nIndex ], sizeof( double ) ) == 0 );
    Check( bExact, "every pixel arrives exactly, bit for bit" );

    std::string strReadable = RayWire::ToReadable( MESSAGE_PIXELS, RayWire::EncodePixels( 43, oTile, oCounts, anColors ) );
    Check( strReadable.find( "\"pixels\"" ) != std::string::npos && strReadable.find( "\"left\": 64" ) != std::string::npos &&
           strReadable.find( "\"shadowArea\"" ) != std::string::npos,
           "a pixels message can be read as JSON" );

    std::string strShort = RayWire::EncodePixels( 43, oTile, oCounts, anColors );
    strShort.resize( strShort.size() - 8 );
    Check( ! RayWire::DecodePixels( strShort, nTrace, oRead, oReadCounts, anRead ), "a short pixels message is refused" );
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
        virtual BOOL PixelsToMaster( uint32_t nTrace, const STile &oTile, const CRayCounts &oCounts, const std::vector<double> &anColors )
        {
            (void)nTrace; (void)oTile; (void)oCounts; (void)anColors;
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

        // A pixel's samples spread its wavelengths over the range: one each
        // of as many strata, shuffled by the pixel, whatever their order.
    {
        BOOL bPermutes = TRUE;

        for ( uint32_t nCount = 1; nCount <= 300 && bPermutes; nCount += ( nCount < 20 ) ? 1 : 37 )
            for ( uint32_t nSeed = 0; nSeed < 5 && bPermutes; nSeed++ )
            {
                std::vector<int> anSeen( nCount, 0 );

                for ( uint32_t nIndex = 0; nIndex < nCount; nIndex++ )
                {
                    uint32_t nTo = RaySampling::Permute( nIndex, nCount, nSeed * 0x9E3779B9U );

                    if ( nTo >= nCount || anSeen[ nTo ]++ )
                        bPermutes = FALSE;
                }
            }
        Check( bPermutes, "the shuffle of strata puts every index in a different place" );

        CScene oScene;

        Check( oScene.FromJson( GlassBallBeforeWall( "\"indexOfRefraction\": 1.5, \"abbe\": 20" ) ) == 0, "a scene for wavelength strata loads" );

        BOOL    bStrata = TRUE;
        BOOL    bSame = TRUE;
        BOOL    bShuffled = FALSE;
        int     anFirst[ 2 ] = { -1, -1 };

        for ( int nPixel = 0; nPixel < 2; nPixel++ )
        {
            const int           COUNT = 25;
            std::vector<int>    anStratum( COUNT, 0 );
            std::vector<int>    anChildStratum( COUNT, 0 );

            for ( int nSample = 0; nSample < COUNT; nSample++ )
            {
                CRay oRay;
                CRay oAgain;
                CRay oChild;

                oRay.CreateRayAt( oScene, 10 + nPixel + ( ( nSample % 5 ) + 0.5 ) / 5.0, 7 + ( ( nSample / 5 ) + 0.5 ) / 5.0, 10 + nPixel, 7 );
                oRay.SetSampleIndex( nSample, COUNT );
                oAgain = oRay;
                oChild.InheritSample( oRay, 3 );

                FLT nOffset = oRay.WavelengthOffset();
                int nStratum = (int)floor( nOffset * COUNT );

                if ( ! ( nOffset >= 0.0 && nOffset < 1.0 ) || nStratum < 0 || nStratum >= COUNT || anStratum[ nStratum ]++ )
                    bStrata = FALSE;
                FLT nChild = oChild.WavelengthOffset();
                int nChildStratum = (int)floor( nChild * COUNT );

                if ( oAgain.WavelengthOffset() != nOffset || nChild != oChild.WavelengthOffset() )
                    bSame = FALSE;
                    // A ray made from it (a reflection, say) shuffles the
                    // strata again, but still takes one each.
                if ( nChildStratum < 0 || nChildStratum >= COUNT || anChildStratum[ nChildStratum ]++ )
                    bStrata = FALSE;
                if ( nSample == 0 )
                    anFirst[ nPixel ] = nStratum;
            }
        }
        if ( anFirst[ 0 ] != anFirst[ 1 ] )
            bShuffled = TRUE;
        Check( bStrata, "a pixel's 25 samples, and the rays made from them, take one each of 25 wavelength strata" );
        Check( bSame, "a ray's wavelength offset is the same every time, and in its copies" );
        Check( bShuffled, "neighboring pixels shuffle their strata differently" );

        CRay oNoIndex;

        oNoIndex.CreateRayAt( oScene, 10.3, 7.6, 10, 7 );

        FLT nHashed = oNoIndex.WavelengthOffset();

        Check( nHashed >= 0.0 && nHashed < 1.0 && oNoIndex.GetSampleCount() == 0 && nHashed == oNoIndex.WavelengthOffset(),
               "a ray without a sample index takes a hashed wavelength offset" );
    }

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
// Indirect light (light bounced between surfaces) and haze
// --------------------------------------------------------------------------

    // A closed sphere of radius 5 round the origin whose inside glows
    // dGlow and scatters dAlbedo of what reaches it, with no lights, and the
    // given "indirect" settings (or none).
static std::string GlowingRoom( double dGlow, double dAlbedo, const char *strIndirect )
{
    char achText[ 1024 ];

    snprintf( achText, sizeof( achText ),
              "{ \"format\": \"Ray scene\", \"version\": 3, \"background\": [0, 0, 0],\n"
              "  \"settings\": { \"maximumDepth\": 16, \"depthThreshold\": 0.0001 %s },\n"
              "  \"camera\": { \"position\": [0, 0, -1], \"lookAt\": [0, 0, 0] },\n"
              "  \"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 5,\n"
              "                   \"materials\": [ { \"type\": \"ambient\", \"influence\": 1, \"color\": [%g, %g, %g] },\n"
              "                                  { \"type\": \"diffuse\", \"influence\": 1, \"color\": [%g, %g, %g] } ] } ],\n"
              "  \"lights\": [] }\n",
              strIndirect, dGlow, dGlow, dGlow, dAlbedo, dAlbedo, dAlbedo );
    return achText;
}

    // A white floor with a red wall beside the point (1, 0, 0) and a light
    // over it, with the given "indirect" settings (or none).
static std::string RedWallFloor( const char *strIndirect )
{
    std::string strText =
        "{ \"format\": \"Ray scene\", \"version\": 3, \"background\": [0, 0, 0],\n"
        "  \"settings\": { \"maximumDepth\": 16, \"depthThreshold\": 0.0001 ";

    strText += strIndirect;
    strText +=
        " },\n"
        "  \"camera\": { \"position\": [0, 1, -10], \"lookAt\": [0, 0, 0] },\n"
        "  \"objects\": [ { \"type\": \"plane\", \"position\": [0, 0, 0], \"normal\": [0, 1, 0],\n"
        "                   \"materials\": [ { \"type\": \"diffuse\", \"influence\": 1, \"color\": [1, 1, 1] } ] },\n"
        "                 { \"type\": \"box\", \"center\": [1.6, 1.5, 0], \"size\": [0.2, 3, 6],\n"
        "                   \"materials\": [ { \"type\": \"diffuse\", \"influence\": 1, \"color\": [1, 0, 0] } ] } ],\n"
        "  \"lights\": [ { \"type\": \"point\", \"position\": [-3, 6, 0], \"color\": [1, 1, 1] } ] }\n";
    return strText;
}

static void IndirectChecks( void )
{
    CScene oPlain;
    CScene oOff;

        // Off (the default, or bounces 0), nothing changes and nothing is saved.
    Check( oPlain.FromJson( RedWallFloor( "" ) ) == 0 &&
           oOff.FromJson( RedWallFloor( ", \"indirect\": { \"bounces\": 0, \"samples\": 4 }" ) ) == 0,
           "scenes without indirect light load" );
    Check( oPlain.GetSettings().GetIndirectBounces() == 0 && oPlain.ToJson().find( "indirect" ) == std::string::npos,
           "indirect light is off by default, and isn't saved" );

    CColor oPlainColor = TraceFrom( oPlain, CVector( 0.8, 0.5, 0.3 ), CVector( 0.0, -1.0, 0.0 ) );
    CColor oOffColor = TraceFrom( oOff, CVector( 0.8, 0.5, 0.3 ), CVector( 0.0, -1.0, 0.0 ) );

    Check( memcmp( &oPlainColor, &oOffColor, sizeof( CColor ) ) == 0, "bounces 0 traces exactly what no indirect light does" );

        // In a closed room that glows g and scatters a, every gathering ray
        // sees the same light, so the answer is exact whatever the samples:
        // g ( 1 + a + ... + a^bounces ).
    const double adAlbedo[] = { 0.5, 0.8 };
    const char  *astrMethod[] = { "objects", "hemisphere" };

    for ( int nCase = 0; nCase < 4; nCase++ )
    {
        int         nAlbedo = nCase % 2;
        const char *strMethod = astrMethod[ nCase / 2 ];

        for ( int nBounces = 1; nBounces <= 4; nBounces++ )
        {
            CScene  oRoom;
            char    achIndirect[ 128 ];
            double  dExpected = 0.0;
            double  dTerm = 0.1;

            snprintf( achIndirect, sizeof( achIndirect ), ", \"indirect\": { \"method\": \"%s\", \"bounces\": %d, \"samples\": 3, \"distance\": 20 }",
                      strMethod, nBounces );
            for ( int nTerm = 0; nTerm <= nBounces; nTerm++ )
            {
                dExpected += dTerm;
                dTerm *= adAlbedo[ nAlbedo ];
            }
            Check( oRoom.FromJson( GlowingRoom( 0.1, adAlbedo[ nAlbedo ], achIndirect ) ) == 0, "the glowing room loads" );

            CColor oSeen = TraceFrom( oRoom, CVector( 0.3, -0.2, 0.1 ), CVector( 0.2, 0.1, 1.0 ) );

            if ( ABS( oSeen.GetR() - dExpected ) > 1e-9 || ABS( oSeen.GetB() - dExpected ) > 1e-9 )
            {
                fprintf( stderr, "FAILED: glowing room, %s, albedo %g, %d bounces: %.12f, expected %.12f\n", strMethod,
                         adAlbedo[ nAlbedo ], nBounces, (double)oSeen.GetR(), dExpected );
                g_nFailures++;
            }
        }
    }

        // Gathering over the half above, a shorter reach than the room brings
        // nothing back; aimed at objects, one farther than the distance is
        // left out.
    CScene oShort;
    CScene oFar;

    Check( oShort.FromJson( GlowingRoom( 0.1, 0.5, ", \"indirect\": { \"method\": \"hemisphere\", \"bounces\": 2, \"distance\": 0.5 }" ) ) == 0 &&
           ABS( TraceFrom( oShort, CVector( 0, 0, 0 ), CVector( 0, 0, 1 ) ).GetR() - 0.1 ) < 1e-12,
           "gathering rays bring back nothing from beyond their distance" );
    Check( oFar.FromJson( RedWallFloor( ", \"indirect\": { \"bounces\": 1, \"distance\": 0.5 }" ) ) == 0,
           "a scene with a short reach loads" );

    CColor oFarColor = TraceFrom( oFar, CVector( 0.8, 0.5, 0.3 ), CVector( 0.0, -1.0, 0.0 ) );

    Check( memcmp( &oFarColor, &oPlainColor, sizeof( CColor ) ) == 0, "an object farther than the distance sends no light" );

        // Beside a red wall, the floor picks up red; and the same ray always
        // gives the same color.
    CScene oBounce;
    CScene oAgain;
    CScene oHemisphere;
    const char *strBounce = ", \"indirect\": { \"bounces\": 1, \"samples\": 16, \"distance\": 10 }";

    Check( oBounce.FromJson( RedWallFloor( strBounce ) ) == 0 && oAgain.FromJson( RedWallFloor( strBounce ) ) == 0 &&
           oHemisphere.FromJson( RedWallFloor( ", \"indirect\": { \"method\": \"hemisphere\", \"bounces\": 1, \"samples\": 64, \"distance\": 10 }" ) ) == 0,
           "a scene with indirect light loads" );

    CColor oBounceColor = TraceFrom( oBounce, CVector( 0.8, 0.5, 0.3 ), CVector( 0.0, -1.0, 0.0 ) );
    CColor oAgainColor = TraceFrom( oAgain, CVector( 0.8, 0.5, 0.3 ), CVector( 0.0, -1.0, 0.0 ) );
    CColor oRepeat = TraceFrom( oBounce, CVector( 0.8, 0.5, 0.3 ), CVector( 0.0, -1.0, 0.0 ) );
    CColor oHemisphereColor = TraceFrom( oHemisphere, CVector( 0.8, 0.5, 0.3 ), CVector( 0.0, -1.0, 0.0 ) );

    Check( oBounceColor.GetR() > oPlainColor.GetR() + 0.01 && ABS( oBounceColor.GetG() - oPlainColor.GetG() ) < 1e-12,
           "light bounced off a red wall adds red to the floor beside it, and only red" );
    Check( oHemisphereColor.GetR() > oPlainColor.GetR() + 0.01 && ABS( oHemisphereColor.GetG() - oPlainColor.GetG() ) < 1e-12,
           "gathered over the half above, too" );
    Check( memcmp( &oBounceColor, &oAgainColor, sizeof( CColor ) ) == 0 && memcmp( &oBounceColor, &oRepeat, sizeof( CColor ) ) == 0,
           "indirect light is the same every time, in every copy of the scene" );

        // Both ways estimate the same light: averaged over many rays and
        // points, the red each adds beside the wall agrees.
    {
        double adRed[ 2 ] = { 0.0, 0.0 };
        int    nPoints = 0;

        for ( int nMethod = 0; nMethod < 2; nMethod++ )
        {
            CScene oMany;

            Check( oMany.FromJson( RedWallFloor( nMethod ? ", \"indirect\": { \"method\": \"hemisphere\", \"bounces\": 1, \"samples\": 256, \"distance\": 20 }"
                                                         : ", \"indirect\": { \"bounces\": 1, \"samples\": 256, \"distance\": 20 }" ) ) == 0,
                   "the comparison scenes load" );
            nPoints = 0;
            for ( double dX = 0.2; dX < 1.4; dX += 0.3 )
            {
                for ( double dZ = -1.0; dZ <= 1.0; dZ += 0.5 )
                {
                    adRed[ nMethod ] += TraceFrom( oMany, CVector( dX, 0.5, dZ ), CVector( 0.0, -1.0, 0.0 ) ).GetR() -
                                        TraceFrom( oPlain, CVector( dX, 0.5, dZ ), CVector( 0.0, -1.0, 0.0 ) ).GetR();
                    nPoints++;
                }
            }
        }
        if ( ! ( ABS( adRed[ 0 ] - adRed[ 1 ] ) < 0.05 * adRed[ 1 ] ) )
        {
            fprintf( stderr, "FAILED: aimed at objects (%.5f) and over the half above (%.5f) gather different light\n",
                     adRed[ 0 ] / nPoints, adRed[ 1 ] / nPoints );
            g_nFailures++;
        }
    }

        // The cap on how many objects one point gathers from.
    CScene oCapped;

    Check( oCapped.FromJson( RedWallFloor( ", \"indirect\": { \"bounces\": 1, \"objects\": 1 }" ) ) == 0 &&
           oCapped.GetSettings().GetIndirectObjects() == 1, "the objects cap is read" );

        // Saved and read back.
    std::string strSaved = oBounce.ToJson();
    CScene      oRead;

    Check( strSaved.find( "\"indirect\"" ) != std::string::npos && oRead.FromJson( strSaved ) == 0 && oRead.ToJson() == strSaved &&
           oRead.GetSettings().GetIndirectSamples() == 16 && oRead.GetSettings().GetIndirectDistance() == 10.0,
           "indirect settings are saved and read back" );

    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 9 } }," ),
                 BAD_FILE, "settings.indirect: bounces must be a whole number from 0 to 8" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 1, \"samples\": 0 } }," ),
                 BAD_FILE, "settings.indirect: samples must be a whole number from 1 to 256" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 1, \"distance\": 0 } }," ),
                 BAD_FILE, "distance must be more than 0" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 1, \"strength\": -1 } }," ),
                 BAD_FILE, "strength must be from 0 to 100" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"samples\": 2 } }," ),
                 BAD_FILE, "bounces" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 1, \"rays\": 2 } }," ),
                 BAD_FILE, "rays" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 1, \"method\": \"cache\" } }," ),
                 BAD_FILE, "unknown method \"cache\" (objects or hemisphere)" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 1, \"method\": \"hemisphere\", \"objects\": 2 } }," ),
                 BAD_FILE, "objects is only for the objects method" );
    ExpectError( Minimal( "\"format\": \"Ray scene\",", "\"format\": \"Ray scene\", \"settings\": { \"indirect\": { \"bounces\": 1, \"objects\": 0 } }," ),
                 BAD_FILE, "objects must be a whole number from 1 to 64" );
}


// ---------------------------------------------------------------------------
// Material presets, light profiles, attenuation and filters
// ---------------------------------------------------------------------------

    // A scene whose one sphere is made of strMaterials (a JSON array).
static std::string WithMaterials( const std::string &strMaterials )
{
    return Minimal( "\"radius\": 1 }", ( "\"radius\": 1, \"materials\": " + strMaterials + " }" ).c_str() );
}

static void PresetChecks( void )
{
    int                         nCount;
    const RayPresets::SPreset  *aoPresets = RayPresets::GetPresets( nCount );
    int                         nMaterials = 0;
    bool                        bAllTextures = true;
    bool                        bAllMaterials = true;

    Check( nCount >= 25, "the library has at least 25 presets" );
    for ( int nIndex = 0; nIndex < nCount; nIndex++ )
    {
        std::string                         strName( aoPresets[ nIndex ].strName );
        std::unique_ptr<IASLib::CJsonNode>  pGiven( RayJson::Parse( "{ \"preset\": \"" + strName + "\" }" ) );
        std::unique_ptr<CTexture>           pTexture( pGiven ? CTexture::Read( *pGiven ) : NULL );

        if ( ! pTexture || pTexture->GetPreset() != strName )
        {
            fprintf( stderr, "preset texture %s: %s\n", strName.c_str(), RayJson::GetError().c_str() );
            bAllTextures = false;
        }
        if ( aoPresets[ nIndex ].strMaterials )
        {
            CScene oScene;

            nMaterials++;
            if ( oScene.FromJson( WithMaterials( "[ { \"preset\": \"" + strName + "\" } ]" ) ) != 0 )
            {
                fprintf( stderr, "preset materials %s: %s\n", strName.c_str(), oScene.GetLoadError() );
                bAllMaterials = false;
            }
        }
    }
    Check( bAllTextures, "every preset's texture loads, and knows its preset" );
    Check( bAllMaterials && nMaterials >= 25, "every preset's materials load" );
    Check( RayPresets::Find( "carrara-marble" ) != NULL && RayPresets::Find( "no-such-stone" ) == NULL, "presets are found by name" );

        // A texture preset with a field laid over it.
    {
        std::unique_ptr<IASLib::CJsonNode>  pGiven( RayJson::Parse( "{ \"preset\": \"carrara-marble\", \"scale\": 2.5, \"seed\": 4 }" ) );
        std::unique_ptr<CTexture>           pTexture( CTexture::Read( *pGiven ) );
        IASLib::CObjectNode                 oFull;

        Check( pTexture && strcmp( pTexture->GetName(), "marble" ) == 0, "a texture preset gives the preset's type" );
        if ( pTexture )
        {
            pTexture->ToJson( oFull );

            std::string strFull = RayJson::Write( oFull );

            Check( strFull.find( "\"scale\": 2.5" ) != std::string::npos && strFull.find( "\"seed\": 4" ) != std::string::npos &&
                   strFull.find( "\"turbulence\": 1.2" ) != std::string::npos && strFull.find( "0.42" ) != std::string::npos,
                   "fields given beside a preset replace its own; the rest are the preset's" );

            IASLib::CObjectNode oSaved;

            pTexture->Write( oSaved, "texture" );

            std::string strSaved = RayJson::Write( oSaved );

            Check( strSaved.find( "\"preset\": \"carrara-marble\"" ) != std::string::npos &&
                   strSaved.find( "\"type\"" ) == std::string::npos && strSaved.find( "colors" ) == std::string::npos,
                   "a texture preset is saved as its name and the fields given beside it" );
        }
    }

        // A materials preset, its overlays, and how it is saved.
    {
        CScene      oScene;
        std::string strScene = WithMaterials( "[ { \"preset\": \"carrara-marble\", \"texture\": { \"scale\": 2.0 }, "
                                              "\"specular\": { \"power\": 0.3 }, \"reflective\": null } ]" );

        Check( oScene.FromJson( strScene ) == 0, "a materials preset with overlays loads" );

        CObjects *pSphere = oScene.GetObjectAt( 0 );

        if ( pSphere )
        {
            CMaterialList      &oList = pSphere->GetMaterials();
            IASLib::CArrayNode  oFull;

            Check( oList.GetSize() == 3, "a preset stands for its materials; null takes one out" );
            oList.ToJsonInFull( oFull );

            std::string strFull = RayJson::Write( oFull );

            Check( strFull.find( "\"power\": 0.3" ) != std::string::npos && strFull.find( "\"reflective\"" ) == std::string::npos &&
                   strFull.find( "\"scale\": 2.0" ) != std::string::npos,
                   "a preset's material and texture take the fields laid over them" );
        }

        std::string strSaved = oScene.ToJson();
        CScene      oReread;

        Check( strSaved.find( "\"preset\": \"carrara-marble\"" ) != std::string::npos &&
               strSaved.find( "\"type\": \"marble\"" ) == std::string::npos &&
               strSaved.find( "\"reflective\": null" ) != std::string::npos,
               "a materials preset is saved as the entry the scene gave" );
        Check( oReread.FromJson( strSaved ) == 0 && oReread.ToJson() == strSaved, "a scene with presets reads back the same" );

            // Changed after loading, it is saved in full.
        if ( pSphere )
        {
            pSphere->GetMaterials().SetInfluence( 1, 0.5 );

            std::string strChanged = oScene.ToJson();

            Check( strChanged.find( "\"preset\": \"carrara-marble\" }" ) == std::string::npos &&
                   strChanged.find( "\"type\": \"diffuse\"" ) != std::string::npos &&
                   strChanged.find( "\"influence\": 0.5" ) != std::string::npos,
                   "a preset's materials changed since loading are saved in full" );
        }
    }

        // An added material, and a preset among other materials.
    {
        CScene oScene;

        Check( oScene.FromJson( WithMaterials( "[ { \"type\": \"ambient\", \"color\": [0.1, 0.1, 0.1] }, "
                                               "{ \"preset\": \"oak\", \"transparent\": { \"influence\": 0.2 } } ]" ) ) == 0 &&
               oScene.GetObjectAt( 0 )->GetMaterials().GetSize() == 1 + 3 + 1,
               "a preset can sit among other materials, and an overlay of a type it lacks adds one" );

        CScene oReread;

        Check( oReread.FromJson( oScene.ToJson() ) == 0 && oReread.ToJson() == oScene.ToJson(),
               "a preset among other materials reads back the same" );
    }

        // Presets in a bump map and a light's filter.
    {
        CScene oScene;

        Check( oScene.FromJson( Minimal( "\"radius\": 1 }",
                   "\"radius\": 1, \"bump\": { \"texture\": { \"preset\": \"slate\" }, \"depth\": 0.02 } }" ) ) == 0 &&
               oScene.ToJson().find( "\"preset\": \"slate\"" ) != std::string::npos,
               "a bump map's texture can be a preset" );
    }

    ExpectError( WithMaterials( "[ { \"preset\": \"no-such-stone\" } ]" ), 1, "there is no preset \"no-such-stone\"" );
    ExpectError( WithMaterials( "[ { \"preset\": \"caustics\" } ]" ), 1, "is a texture, not a material" );
    ExpectError( WithMaterials( "[ { \"preset\": \"oak\", \"shine\": 1 } ]" ), 1, "isn't a field of a preset" );
    ExpectError( WithMaterials( "[ { \"preset\": \"oak\", \"specular\": 3 } ]" ), 1, "must be an object" );
    ExpectError( WithMaterials( "[ { \"type\": \"diffuse\", \"texture\": { \"preset\": \"marble\" } } ]" ), 1, "there is no preset" );
    ExpectError( WithMaterials( "[ { \"type\": \"diffuse\", \"texture\": { \"preset\": \"oak\", \"stretch\": [1, 0, 1] } } ]" ), 1,
                 "stretch must be positive" );
}

static void CellChecks( void )
{
    CCellsTexture       oCells;
    CCausticsTexture    oCaustics;
    STexturePoint       oAt;
    bool                bInRange = true;
    bool                bSame = true;
    FLT                 nMost = 0.0;
    FLT                 nLeast = 1.0;

    oAt.bHasUV = FALSE;
    oAt.bHasColor = FALSE;
    oAt.nU = oAt.nV = 0.0;
    for ( int nI = 0; nI < 40; nI++ )
    {
        for ( int nJ = 0; nJ < 40; nJ++ )
        {
            oAt.oPoint = CVector( nI * 0.137, 0.31, nJ * 0.113 );

            CColor oCell = oCells.ColorAt( oAt );
            CColor oCaustic = oCaustics.ColorAt( oAt );
            CColor oAgain = oCaustics.ColorAt( oAt );

                // The caustics ramp runs from 0.35 to 1.4 in red.
            FLT nValue = ( oCaustic.GetR() - 0.35 ) / ( 1.4 - 0.35 );

            bInRange = bInRange && oCell.GetR() >= 0.0 && oCell.GetR() <= 1.0 && nValue >= -1e-12 && nValue <= 1.0 + 1e-12;
            bSame = bSame && oCaustic.GetR() == oAgain.GetR();
            nMost = std::max( nMost, nValue );
            nLeast = std::min( nLeast, nValue );
        }
    }
    Check( bInRange, "cells and caustics stay within their ramps" );
    Check( bSame, "caustics are the same at a point every time" );
    Check( nMost > 0.9 && nLeast == 0.0, "caustics are bright lines on darker ground" );

        // Flecks: one value all over a cell (points near a feature point).
    {
        std::unique_ptr<IASLib::CJsonNode>  pGiven( RayJson::Parse( "{ \"type\": \"cells\", \"mode\": \"flecks\" }" ) );
        std::unique_ptr<CTexture>           pFlecks( CTexture::Read( *pGiven ) );
        STexturePoint                       oNear( oAt );

        oAt.oPoint = CVector( 10.5, 3.5, 7.5 );
        oNear.oPoint = CVector( 10.5001, 3.5001, 7.5001 );
        Check( pFlecks && pFlecks->ColorAt( oAt ).GetR() == pFlecks->ColorAt( oNear ).GetR(), "flecks are one color across a cell" );
    }
    ExpectError( WithMaterials( "[ { \"type\": \"diffuse\", \"texture\": { \"type\": \"cells\", \"mode\": \"blobs\" } } ]" ), 1,
                 "must be \"flecks\", \"distance\" or \"edges\"" );
    ExpectError( WithMaterials( "[ { \"type\": \"diffuse\", \"texture\": { \"type\": \"caustics\", \"layers\": 4 } } ]" ), 1,
                 "layers" );
}

    // The first light of a scene whose lights are strLights.
static std::string WithLights( const std::string &strLights )
{
    return Minimal( "\"lights\": [ { \"type\": \"point\", \"position\": [5, 5, -5], \"color\": [1, 1, 1] } ]",
                    ( "\"lights\": [ " + strLights + " ]" ).c_str() );
}

    // The red of the light a spot (at the origin, along +z) sends theta
    // degrees off its axis.
static FLT SpotAt( CLight &oLight, double dDegrees )
{
    double  dRadians = dDegrees * PI / 180.0;
    CVector oPoint( 10.0 * sin( dRadians ), 0.0, 10.0 * cos( dRadians ) );
    CVector oToLight( oPoint * -1.0 );

    return oLight.ShadeAt( oPoint, oToLight ).GetR();
}

static void LightOptionChecks( void )
{
    const char *strSpot = "{ \"type\": \"spot\", \"position\": [0, 0, 0], \"direction\": [0, 0, 1], \"color\": [1, 1, 1], "
                          "\"angle\": 20, \"falloff\": 5";

        // The default profile is the old smoothstep edge, exactly.
    {
        CScene oScene;

        Check( oScene.FromJson( WithLights( std::string( strSpot ) + " }" ) ) == 0, "a spot light loads" );

        CSpotLight *pSpot = dynamic_cast<CSpotLight *>( oScene.GetLight( 0 ) );

        if ( pSpot )
        {
            double  dOuter = cos( 20.0 * PI / 180.0 );
            double  dInner = cos( 15.0 * PI / 180.0 );
            double  dCos = cos( 17.0 * PI / 180.0 );
            double  dT = ( dCos - dOuter ) / ( dInner - dOuter );

            Check( SpotAt( *pSpot, 5.0 ) == 1.0 && SpotAt( *pSpot, 25.0 ) == 0.0 &&
                   ABS( SpotAt( *pSpot, 17.0 ) - dT * dT * ( 3.0 - 2.0 * dT ) ) < 1e-12,
                   "the default profile is the smoothstep edge it always was" );
            Check( oScene.ToJson().find( "\"profile\"" ) == std::string::npos, "the default profile isn't saved" );
        }
    }

    struct SProfileCase
    {
        const char *strExtra;
        double      dDegrees;
        double      dExpected;
        const char *strWhat;
    };
    const SProfileCase aoCases[] =
    {
        { ", \"profile\": \"linear\"", 17.0, -1.0, "a linear profile fades evenly across the edge" },
        { ", \"profile\": \"cosine\", \"exponent\": 10", 10.0, pow( cos( 10.0 * PI / 180.0 ), 10.0 ), "a cosine profile is cos^exponent" },
        { ", \"profile\": \"gaussian\", \"spread\": 5", 5.0, exp( -0.5 ), "a gaussian profile falls to exp(-1/2) at its spread" },
        { ", \"profile\": \"fresnel\", \"hotspot\": 4, \"rim\": 0.3", 2.0, 1.0, "a fresnel profile is full in its center" },
        { ", \"profile\": \"fresnel\", \"hotspot\": 4, \"rim\": 0.3", 10.0, 0.3, "a fresnel profile falls to its rim" },
        { ", \"profile\": \"table\", \"table\": [1, 0.5]", 10.0, 0.75, "a table profile blends between its entries" },
    };

    for ( size_t nCase = 0; nCase < sizeof( aoCases ) / sizeof( aoCases[ 0 ] ); nCase++ )
    {
        CScene oScene;

        if ( oScene.FromJson( WithLights( std::string( strSpot ) + aoCases[ nCase ].strExtra + " }" ) ) != 0 )
        {
            Check( false, aoCases[ nCase ].strWhat );
            continue;
        }

        double dExpected = aoCases[ nCase ].dExpected;

        if ( dExpected < 0.0 )
        {
            double dOuter = cos( 20.0 * PI / 180.0 );
            double dInner = cos( 15.0 * PI / 180.0 );

            dExpected = ( cos( 17.0 * PI / 180.0 ) - dOuter ) / ( dInner - dOuter );
        }
        Check( ABS( SpotAt( *oScene.GetLight( 0 ), aoCases[ nCase ].dDegrees ) - dExpected ) < 1e-12, aoCases[ nCase ].strWhat );

        CScene oReread;

        Check( oReread.FromJson( oScene.ToJson() ) == 0 && oReread.ToJson() == oScene.ToJson(), "a beam profile reads back the same" );
    }

        // Attenuation.
    {
        const char *astrKinds[] = { "none", "inverse", "inverseSquare" };
        const double adExpected[] = { 1.0, 0.5, 0.25 };

        for ( int nKind = 0; nKind < 3; nKind++ )
        {
            CScene oScene;

            Check( oScene.FromJson( WithLights( std::string( "{ \"type\": \"point\", \"position\": [0, 0, 0], \"color\": [1, 1, 1], "
                                                             "\"attenuation\": \"" ) + astrKinds[ nKind ] + "\", \"distance\": 2 }" ) ) == 0,
                   "an attenuated point light loads" );

            CVector oPoint( 0.0, 4.0, 0.0 );
            CVector oToLight( 0.0, -4.0, 0.0 );

            Check( oScene.GetLight( 0 ) && ABS( oScene.GetLight( 0 )->ShadeAt( oPoint, oToLight ).GetR() - adExpected[ nKind ] ) < 1e-15,
                   "attenuation weakens a light by 1, d0/d or (d0/d)^2" );
        }
    }

        // A filter projected across a spot's cone (angle 45: tan 1).
    {
        CScene oScene;

        Check( oScene.FromJson( WithLights( "{ \"type\": \"spot\", \"position\": [0, 0, 0], \"direction\": [0, 0, 1], "
                                            "\"color\": [1, 1, 1], \"angle\": 45, \"falloff\": 0, \"filter\": { \"texture\": "
                                            "{ \"type\": \"checker\", \"colors\": [[1, 0, 0], [0, 0, 1]], \"size\": 0.5 } } }" ) ) == 0,
               "a spot light with a filter loads" );

        CLight *pLight = oScene.GetLight( 0 );
        CVector oFirst;
        CVector oSecond;

        RaySampling::Basis( CVector( 0.0, 0.0, 1.0 ), oFirst, oSecond );
        if ( pLight )
        {
                // x / z = 0.2: u = 0.6, v = 0.5 -> cells 1 + 1, even: the
                // first color. x / z = -0.2: u = 0.4 -> 0 + 1, odd: the second.
            CVector oForward( 0.0, 0.0, 1.0 );
            CVector oRightward( oFirst * 0.2 );
            CVector oLeftward( oFirst * -0.2 );
            CVector oRight( oRightward + oForward );
            CVector oLeft( oLeftward + oForward );
            CVector oToRight( oRight * -1.0 );
            CVector oToLeft( oLeft * -1.0 );
            CColor  oOne = pLight->ShadeAt( oRight, oToRight );
            CColor  oOther = pLight->ShadeAt( oLeft, oToLeft );

            Check( oOne.GetR() == 1.0 && oOne.GetB() == 0.0 && oOther.GetR() == 0.0 && oOther.GetB() == 1.0,
                   "a spot light's filter is projected across its cone" );
        }
        Check( oScene.ToJson().find( "\"filter\"" ) != std::string::npos, "a filter is saved" );
    }

        // Sunlight's filter, straight along its direction: moving across it
        // by the size moves one whole image over.
    {
        CScene oScene;

        Check( oScene.FromJson( WithLights( "{ \"type\": \"directional\", \"direction\": [0, -1, 0], \"color\": [1, 1, 1], "
                                            "\"filter\": { \"texture\": { \"preset\": \"caustics\" }, \"size\": 2 } }" ) ) == 0,
               "sunlight with a caustics filter loads" );

        CLight *pSun = oScene.GetLight( 0 );

        if ( pSun )
        {
            CVector oHigh( 1.3, 5.0, 0.7 );
            CVector oLow( 1.3, -2.0, 0.7 );
            CVector oToSun( 0.0, 1.0e6, 0.0 );

            Check( pSun->ShadeAt( oHigh, oToSun ).GetR() == pSun->ShadeAt( oLow, oToSun ).GetR(),
                   "sunlight's filter is the same all along its direction" );
        }
    }

    ExpectError( WithLights( std::string( strSpot ) + ", \"profile\": \"spiky\" }" ), 1, "must be \"smoothstep\"" );
    ExpectError( WithLights( std::string( strSpot ) + ", \"profile\": \"table\" }" ), 1, "needs a \"table\"" );
    ExpectError( WithLights( std::string( strSpot ) + ", \"rim\": 2 }" ), 1, "must be from 0 to 1" );
    ExpectError( WithLights( "{ \"type\": \"point\", \"position\": [0, 0, 0], \"color\": [1, 1, 1], \"attenuation\": \"cubic\" }" ), 1,
                 "\"inverseSquare\"" );
    ExpectError( WithLights( "{ \"type\": \"directional\", \"direction\": [0, -1, 0], \"color\": [1, 1, 1], \"attenuation\": \"inverse\" }" ),
                 1, "attenuation" );
    ExpectError( WithLights( "{ \"type\": \"point\", \"position\": [0, 0, 0], \"color\": [1, 1, 1], \"filter\": { \"size\": 1 } }" ), 1,
                 "needs a \"texture\"" );
    ExpectError( WithLights( "{ \"type\": \"point\", \"position\": [0, 0, 0], \"color\": [1, 1, 1], \"profile\": \"cosine\" }" ), 1,
                 "profile" );
}

static void HazeChecks( void )
{
    CHaze oHaze;

    Check( oHaze.Transmittance( CVector( 0, 1, 0 ), CVector( 1, 0, 0 ), 10.0 ) == 1.0, "haze of density 0 lets everything through" );

        // Constant density: T = exp( -d L ).
    CScene oScene;

    Check( oScene.FromJson( Minimal( "\"lights\"", "\"haze\": { \"color\": [0.5, 0.6, 0.7], \"density\": 0.2 }, \"lights\"" ) ) == 0 &&
           oScene.GetHaze() != NULL, "a scene with haze loads" );

    const CHaze *pHaze = oScene.GetHaze();

    if ( pHaze )
    {
        Check( ABS( pHaze->Transmittance( CVector( 3, -2, 1 ), CVector( 0.6, 0.0, 0.8 ), 7.5 ) - exp( -0.2 * 7.5 ) ) < 1e-14,
               "constant haze lets exp( -density length ) through" );

        CColor oThrough = pHaze->Through( CColor( 1.0, 0.0, 0.0 ), 0.25 );

        Check( ABS( oThrough.GetR() - ( 0.25 + 0.75 * 0.5 ) ) < 1e-14 && ABS( oThrough.GetB() - 0.75 * 0.7 ) < 1e-14,
               "a color seen through haze blends toward the haze's color" );
        Check( pHaze->Transmittance( CVector( 0, 0, 0 ), CVector( 0, 0, 1 ), RAY_FAR_AWAY ) == 0.0,
               "a ray leaving the scene through constant haze sees only haze" );
    }

        // Thinning with height: the closed form matches a fine numerical sum.
    CScene oMist;

    Check( oMist.FromJson( Minimal( "\"lights\"", "\"haze\": { \"density\": 0.5, \"heightFalloff\": 0.7, \"baseHeight\": 1 }, \"lights\"" ) ) == 0,
           "a scene with ground mist loads" );

    const CHaze *pMist = oMist.GetHaze();

    if ( pMist )
    {
        const double adDirection[][ 3 ] = { { 0.0, 0.6, 0.8 }, { 0.0, -0.28, 0.96 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 } };
        bool         bExact = true;

        for ( int nDirection = 0; nDirection < 4; nDirection++ )
        {
            CVector oFrom( 0.5, 2.0, -1.0 );
            CVector oAlong( adDirection[ nDirection ][ 0 ], adDirection[ nDirection ][ 1 ], adDirection[ nDirection ][ 2 ] );
            double  dLength = 6.0;
            double  dSum = 0.0;
            int     nSteps = 200000;

            for ( int nStep = 0; nStep < nSteps; nStep++ )
            {
                double dT = ( nStep + 0.5 ) * dLength / nSteps;
                double dY = 2.0 + dT * adDirection[ nDirection ][ 1 ];

                dSum += 0.5 * exp( -0.7 * ( dY - 1.0 ) ) * ( dLength / nSteps );
            }
            if ( ABS( pMist->OpticalDepth( oFrom, oAlong, dLength ) - dSum ) > 1e-8 ||
                 ABS( pMist->Transmittance( oFrom, oAlong, dLength ) - exp( -dSum ) ) > 1e-8 )
                bExact = false;
        }
        Check( bExact, "haze thinning with height lets exactly exp( -integral of the density ) through" );

        CVector oLow( 0, 0, 0 );
        CVector oHigh( 0, 6, 0 );
        CVector oFlat( 1, 0, 0 );

        Check( pMist->Transmittance( oLow, oFlat, 5.0 ) < pMist->Transmittance( oHigh, oFlat, 5.0 ),
               "mist is thicker near the ground than above it" );
        Check( pMist->Transmittance( oHigh, CVector( 0, 1, 0 ), RAY_FAR_AWAY ) > 0.0 &&
               pMist->Transmittance( oLow, CVector( 0, -1, 0 ), RAY_FAR_AWAY ) == 0.0,
               "a ray up out of the mist gets through; one down into it doesn't" );
    }

        // The floor under a light, without and with haze: the camera ray's
        // color and the light's both pass through it.
    CScene      oClear;
    CScene      oHazy;
    std::string strLight = "{ \"type\": \"point\", \"position\": [0, 4, 0], \"color\": [1, 1, 1] }";
    std::string strFloor = FloorUnder( strLight.c_str() );
    std::string strHazy = strFloor;

    strHazy.replace( strHazy.find( "\"camera\"" ), 0, "\"haze\": { \"color\": [0, 0, 0], \"density\": 0.1 },\n  " );
    Check( oClear.FromJson( strFloor ) == 0 && oHazy.FromJson( strHazy ) == 0, "the floor scenes load" );

    double dClear = TraceFrom( oClear, CVector( 0, 0.5, 0 ), CVector( 0, -1, 0 ) ).GetR();
    double dHazy = TraceFrom( oHazy, CVector( 0, 0.5, 0 ), CVector( 0, -1, 0 ) ).GetR();

        // Black haze: the light comes 4 units down to the floor (exp( -0.4 ))
        // and is seen from 0.5 units above it (exp( -0.05 )).
    Check( ABS( dHazy - dClear * exp( -0.4 ) * exp( -0.05 ) ) < 1e-12, "haze dims the light on its way to a surface, and the view of it" );

    CScene      oSun;
    std::string strSun = FloorUnder( "{ \"type\": \"directional\", \"direction\": [0, -1, 0], \"color\": [1, 1, 1] }" );

    strSun.replace( strSun.find( "\"camera\"" ), 0, "\"haze\": { \"color\": [0, 0, 0], \"density\": 0.1 },\n  " );
    Check( oSun.FromJson( strSun ) == 0 &&
           ABS( TraceFrom( oSun, CVector( 0, 0.5, 0 ), CVector( 0, -1, 0 ) ).GetR() - exp( -0.05 ) ) < 1e-12,
           "sunlight isn't dimmed by haze that is the same everywhere" );

        // Saved, read back, edited as a part, and its errors.
    std::string strSaved = oMist.ToJson();
    CScene      oRead;

    Check( strSaved.find( "\"heightFalloff\": 0.7" ) != std::string::npos && oRead.FromJson( strSaved ) == 0 && oRead.ToJson() == strSaved,
           "haze is saved and read back" );
    Check( ! oRead.PartToJson( "haze" ).empty() && oRead.EditPart( "haze", "", "." ) == 0 && oRead.GetHaze() == NULL &&
           oRead.EditPart( "haze", "{ \"density\": 0.3 }", "." ) == 0 && oRead.GetHaze() && oRead.GetHaze()->GetDensity() == 0.3,
           "haze can be edited, removed and added as a part of the scene" );
    Check( oClear.GetHaze() == NULL && oClear.ToJson().find( "haze" ) == std::string::npos, "a scene without haze saves none" );
    ExpectError( Minimal( "\"lights\"", "\"haze\": { \"color\": [1, 1, 1] }, \"lights\"" ), BAD_FILE, "haze: needs \"density\"" );
    ExpectError( Minimal( "\"lights\"", "\"haze\": { \"density\": -1 }, \"lights\"" ), BAD_FILE, "density must be from 0" );
    ExpectError( Minimal( "\"lights\"", "\"haze\": { \"density\": 1, \"heightFalloff\": -1 }, \"lights\"" ), BAD_FILE, "heightFalloff must be" );
    ExpectError( Minimal( "\"lights\"", "\"haze\": { \"density\": 1, \"fog\": 2 }, \"lights\"" ), BAD_FILE, "fog" );
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
    // The new primitives: hits and normals against worked-out answers, from
    // outside and inside, far from the origin, in a csg, and saved.
static bool ShapeHitsAt( CObjects &oObject, CRay oRay, double dT, double dNX, double dNY, double dNZ, double dTolerance = 1e-9 )
{
    CVector oHit, oNormal;

    if ( ! oObject.Intersect( oRay, oHit ) || ABS( oRay.GetT() - dT ) > dTolerance || ! oObject.SurfaceNormal( oHit, oNormal ) )
        return false;
    return ABS( oNormal.GetX() - dNX ) < 1e-6 && ABS( oNormal.GetY() - dNY ) < 1e-6 && ABS( oNormal.GetZ() - dNZ ) < 1e-6;
}

static void ShapeChecks( void )
{
    CScene      oScene;
    CObjects   *pObject;

        // The root finder: (x - 1)(x - 2)(x - 3)(x - 4).
    double anQuartic[ 5 ] = { 24.0, -50.0, 35.0, -10.0, 1.0 };
    double anRoots[ 4 ];
    int    nRoots = CLocalShape::PolynomialRoots( anQuartic, 4, 0.0, 5.0, anRoots );

    Check( nRoots == 4 && ABS( anRoots[ 0 ] - 1 ) < 1e-12 && ABS( anRoots[ 1 ] - 2 ) < 1e-12 && ABS( anRoots[ 2 ] - 3 ) < 1e-12 &&
           ABS( anRoots[ 3 ] - 4 ) < 1e-12, "a quartic's four roots are found exactly, in order" );
    Check( CLocalShape::PolynomialRoots( anQuartic, 4, 4.5, 9.0, anRoots ) == 0, "no root is found outside the range asked" );

        // A ring of radius 1 and tube 0.25, around y.
    const char *strTorus = "{ \"type\": \"torus\", \"center\": [0, 0, 0], \"majorRadius\": 1, \"minorRadius\": 0.25 }";

    pObject = FirstObject( oScene, strTorus );
    if ( pObject )
    {
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 3.75, 0, 0, -1 ), "a ray hits a torus's outer rim" );
        Check( Misses( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ) ), "a ray down a torus's hole misses it" );
        Check( ShapeHitsAt( *pObject, RayFrom( 1, 5, 0, 0, -1, 0 ), 4.75, 0, 1, 0 ), "a ray down onto a torus's tube hits its top" );
        Check( ShapeHitsAt( *pObject, RayFrom( 1, 0, 0, 1, 0, 0 ), 0.25, 1, 0, 0 ), "a ray from inside a torus's tube leaves by its wall" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 3.75, 0, 0, -1 ) && pObject->IsClosed(), "a torus is a solid" );

            // Leaving the outer rim inward, ignoring itself, it crosses the
            // tube and the hole to the tube's far wall.
        CRay    oIn = RayFrom( 0, 0, -1.25, 0, 0, 1 );
        CVector oHit;

        Check( pObject->Intersect( oIn, oHit, pObject ) && ABS( oIn.GetT() - 0.5 ) < 1e-9,
               "a ray leaving a torus, ignoring it, meets the tube again" );
    }

        // The same far from the origin, seen from far away.
    pObject = FirstObject( oScene, "{ \"type\": \"torus\", \"center\": [100000, 200000, -300000], \"majorRadius\": 1, \"minorRadius\": 0.25 }" );
    if ( pObject )
    {
        Check( ShapeHitsAt( *pObject, RayFrom( 100000, 200000, -1300000, 0, 0, 1 ), 999998.75, 0, 0, -1, 1e-6 ),
               "a torus far from the origin, seen from a million units away, is hit where it should be" );
        Check( ShapeHitsAt( *pObject, RayFrom( 100001, 200005, -300000, 0, -1, 0 ), 4.75, 0, 1, 0, 1e-8 ),
               "a torus far from the origin keeps its precision" );
    }

        // A torus taken out of a slab: the slab is cut through where the
        // tube was.
    pObject = FirstObject( oScene, CsgOf( "difference", "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": [3, 0.3, 3] },"
                                                        "{ \"type\": \"torus\", \"center\": [0, 0, 0], \"majorRadius\": 1, \"minorRadius\": 0.25 }" ).c_str() );
    if ( pObject )
    {
        Check( Misses( *pObject, RayFrom( 1, 5, 0, 0, -1, 0 ) ), "a slab with a torus taken out is open where the tube was" );
        Check( CsgHitsAt( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ), 4.85, 0, 1, 0 ), "the slab is whole in the torus's hole" );
    }

        // Discs and rings.
    pObject = FirstObject( oScene, "{ \"type\": \"annulus\", \"center\": [0, 0, 0], \"radius\": 1, \"innerRadius\": 0.5 }" );
    if ( pObject )
    {
        Check( ShapeHitsAt( *pObject, RayFrom( 0.75, 5, 0, 0, -1, 0 ), 5.0, 0, 1, 0 ), "a ray hits a ring between its edges" );
        Check( Misses( *pObject, RayFrom( 0.25, 5, 0, 0, -1, 0 ) ) && Misses( *pObject, RayFrom( 1.2, 5, 0, 0, -1, 0 ) ),
               "a ray through a ring's hole, or past its rim, misses it" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0.75, -5, 0, 0, 1, 0 ), 5.0, 0, 1, 0 ), "a ring is hit from below too" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"disc\", \"center\": [0, 0, 0], \"radius\": 1, \"rotation\": [90, 0, 0] }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0.2, 0.3, -5, 0, 0, 1 ), 5.0, 0, 0, 1 ), "a disc turned 90 degrees about x faces +z" );

        // A bowl of radius 1 and height 1, capped.
    pObject = FirstObject( oScene, "{ \"type\": \"paraboloid\", \"center\": [0, 0, 0], \"radius\": 1, \"height\": 1, \"capped\": true }" );
    if ( pObject )
    {
        double dZ = sqrt( 0.5 ), dL = sqrt( 1.0 + 2.0 );

        Check( ShapeHitsAt( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ), 4.0, 0, 1, 0 ), "a ray hits a capped paraboloid's cap" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, -5, 0, 0, 1, 0 ), 5.0, 0, -1, 0 ), "a ray from below hits a paraboloid's vertex" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0.5, -5, 0, 0, 1 ), 5.0 - dZ, 0, -1.0 / dL, -sqrt( 2.0 ) / dL ),
               "a ray hits a paraboloid's side, with the normal it should have" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0.5, 0, 0, 0, 1 ), dZ, 0, -1.0 / dL, sqrt( 2.0 ) / dL ),
               "a ray from inside a paraboloid leaves by its side" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"paraboloid\", \"center\": [0, 0, 0], \"radius\": 1, \"height\": 1 }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ), 5.0, 0, -1, 0 ) && ! pObject->IsClosed(),
               "an open paraboloid is seen into, down to its vertex" );

        // A hyperboloid with waist 1, ends 2 and height 2 (so k = 3).
    pObject = FirstObject( oScene, "{ \"type\": \"hyperboloid\", \"center\": [0, 0, 0], \"radius\": 1, \"topRadius\": 2, \"height\": 2, \"capped\": true }" );
    if ( pObject )
    {
        double dL = sqrt( 4.0 + 9.0 );

        Check( ShapeHitsAt( *pObject, RayFrom( -5, 0, 0, 1, 0, 0 ), 4.0, -1, 0, 0 ), "a ray hits a hyperboloid at its waist" );
        Check( ShapeHitsAt( *pObject, RayFrom( -5, 0.999999, 0, 1, 0, 0 ), 5.0 - sqrt( 1.0 + ( 3.0 * 0.999999 * 0.999999 ) ),
                            -sqrt( 1.0 + ( 3.0 * 0.999999 * 0.999999 ) ) / sqrt( 1.0 + ( 3.0 * 0.999999 * 0.999999 ) + ( 9.0 * 0.999999 * 0.999999 ) ),
                            -3.0 * 0.999999 / sqrt( 1.0 + ( 3.0 * 0.999999 * 0.999999 ) + ( 9.0 * 0.999999 * 0.999999 ) ), 0 ),
               "a ray near a hyperboloid's end hits its flared side" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ), 4.0, 0, 1, 0 ), "a ray hits a capped hyperboloid's top" );
        (void)dL;
    }

        // A superquadric with both exponents 1 is a sphere.
    pObject = FirstObject( oScene, "{ \"type\": \"superquadric\", \"center\": [0, 0, 0], \"radius\": 1, \"e1\": 1, \"e2\": 1 }" );
    if ( pObject )
    {
        double dS = 1.0 / sqrt( 3.0 );

        Check( ShapeHitsAt( *pObject, RayFrom( 0.3, 0.2, -5, 0, 0, 1 ), 5.0 - sqrt( 0.87 ), 0.3, 0.2, -sqrt( 0.87 ) ),
               "a superquadric with both exponents 1 is a sphere" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, 0, dS, dS, dS ), 1.0, dS, dS, dS ), "a ray from a superquadric's center leaves it" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0.9999, -5, 0, 0, 1 ), 5.0 - sqrt( 1.0 - ( 0.9999 * 0.9999 ) ), 0, 0.9999,
                            -sqrt( 1.0 - ( 0.9999 * 0.9999 ) ), 1e-9 ),
               "a ray grazing a superquadric is found (golden-section search)" );
        Check( Misses( *pObject, RayFrom( 0, 1.0001, -5, 0, 0, 1 ) ), "a ray just past a superquadric misses it" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"superquadric\", \"center\": [0, 0, 0], \"radius\": [1, 2, 1], \"e1\": 0.1, \"e2\": 0.1 }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0.3, 0.4, -5, 0, 0, 1 ), 4.0, 0, 0, -1, 1e-6 ),
               "a superquadric with small exponents is nearly a box, with flat faces" );
    pObject = FirstObject( oScene, "{ \"type\": \"superquadric\", \"center\": [0, 0, 0], \"radius\": 1, \"e1\": 3, \"e2\": 3 }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( -5, 0, 0, 1, 0, 0 ), 4.0, -1, 0, 0, 1e-9 ) && Misses( *pObject, RayFrom( -5, 0.6, 0.6, 1, 0, 0 ) ),
               "a pinched superquadric reaches its tips and is thin between them" );

        // A rounded box of size 2 and radius 0.5.
    pObject = FirstObject( oScene, "{ \"type\": \"roundedBox\", \"center\": [0, 0, 0], \"size\": 2, \"radius\": 0.5 }" );
    if ( pObject )
    {
        double dS = 1.0 / sqrt( 3.0 ), dH = 1.0 / sqrt( 2.0 );

        Check( ShapeHitsAt( *pObject, RayFrom( 0.1, 0.2, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a ray hits a rounded box's flat face" );
        Check( ShapeHitsAt( *pObject, RayFrom( 5, 5, 5, -dS, -dS, -dS ), sqrt( 75.0 ) - ( sqrt( 0.75 ) + 0.5 ), dS, dS, dS ),
               "a ray hits a rounded box's corner, which is round" );
        Check( ShapeHitsAt( *pObject, RayFrom( 5, 5, 0.2, -dH, -dH, 0 ), sqrt( 50.0 ) - ( sqrt( 0.5 ) + 0.5 ), dH, dH, 0 ),
               "a ray hits a rounded box's edge, which is a quarter cylinder" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, 0, 1, 0, 0 ), 1.0, 1, 0, 0 ), "a ray from inside a rounded box leaves by a face" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"roundedBox\", \"center\": [0, 0, 0], \"size\": 2, \"radius\": 1 }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0.3, 0.2, -5, 0, 0, 1 ), 5.0 - sqrt( 0.87 ), 0.3, 0.2, -sqrt( 0.87 ) ),
               "a rounded box rounded to half its size is a sphere" );

        // Each closed new shape inside a large box (a csg intersection) is
        // itself: the same hits, from outside and in.
    const char *astrClosed[] =
    {
        "{ \"type\": \"torus\", \"center\": [0, 0, 0], \"majorRadius\": 1, \"minorRadius\": 0.4 }",
        "{ \"type\": \"paraboloid\", \"center\": [0, -0.5, 0], \"radius\": 1, \"height\": 1, \"capped\": true }",
        "{ \"type\": \"hyperboloid\", \"center\": [0, 0, 0], \"radius\": 0.6, \"topRadius\": 1, \"height\": 1.5, \"capped\": true }",
        "{ \"type\": \"superquadric\", \"center\": [0, 0, 0], \"radius\": [1, 0.8, 1.2], \"e1\": 0.5, \"e2\": 1.5 }",
        "{ \"type\": \"roundedBox\", \"center\": [0, 0, 0], \"size\": [2, 1.5, 1], \"radius\": 0.3, \"rotation\": [10, 20, 30] }",
    };

    for ( const char *strShape : astrClosed )
    {
        CScene      oAlone;
        CObjects   *pAlone = FirstObject( oAlone, strShape );
        CObjects   *pInCsg = FirstObject( oScene, CsgOf( "intersection", ( std::string( strShape ) +
                                                         ", { \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 10 }" ).c_str() ).c_str() );
        bool        bSame = pAlone && pInCsg && pAlone->IsClosed();

        for ( int nRay = 0; bSame && nRay < 40; nRay++ )
        {
            double  dA = nRay * 0.7, dB = nRay * 1.3;
            CRay    oFirst = RayFrom( 3.0 * cos( dA ), 0.4 * sin( dB ), 3.0 * sin( dA ), -cos( dA ), -0.1 * sin( dB ), -sin( dA ) );
            CRay    oSecond = oFirst;
            CVector oHit1, oHit2;
            bool    bHit1 = pAlone->Intersect( oFirst, oHit1 ) ? true : false;
            bool    bHit2 = pInCsg->Intersect( oSecond, oHit2 ) ? true : false;

            bSame = ( bHit1 == bHit2 ) && ( ! bHit1 || ABS( oFirst.GetT() - oSecond.GetT() ) < 1e-9 );
        }
        Check( bSame, "a closed new shape works in a csg, hit where it is hit alone" );
    }

        // Every new shape saves and reads back as it was.
    const char *astrAll[] =
    {
        "{ \"type\": \"torus\", \"center\": [1, 2, 3], \"majorRadius\": 1, \"minorRadius\": 0.25, \"rotation\": [10, 0, 5] }",
        "{ \"type\": \"disc\", \"center\": [0, 1, 0], \"radius\": 2 }",
        "{ \"type\": \"annulus\", \"center\": [0, 1, 0], \"radius\": 2, \"innerRadius\": 1 }",
        "{ \"type\": \"paraboloid\", \"center\": [0, 0, 0], \"radius\": 1, \"height\": 2, \"capped\": true }",
        "{ \"type\": \"hyperboloid\", \"center\": [0, 0, 0], \"radius\": 1, \"topRadius\": 1.5, \"height\": 2 }",
        "{ \"type\": \"superquadric\", \"center\": [0, 0, 0], \"radius\": [1, 2, 3], \"e1\": 0.5, \"e2\": 2 }",
        "{ \"type\": \"roundedBox\", \"center\": [0, 0, 0], \"size\": [1, 2, 3], \"radius\": 0.25 }",
    };

    for ( const char *strShape : astrAll )
    {
        CScene oAgain;

        if ( FirstObject( oScene, strShape ) )
        {
            std::string strSaved = oScene.ToJson();

            Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "a new shape reads back as it was saved" );
        }
    }

        // Bad settings are reported.
    std::string strObjects = "\"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 } ]";

    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"torus\", \"center\": [0, 0, 0], \"majorRadius\": 1, \"minorRadius\": 0 } ]" ),
                 BAD_FILE, "minorRadius must be more than 0" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"annulus\", \"center\": [0, 0, 0], \"radius\": 1 } ]" ),
                 BAD_FILE, "innerRadius" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"disc\", \"center\": [0, 0, 0], \"radius\": 1, \"innerRadius\": 2 } ]" ),
                 BAD_FILE, "innerRadius must be at least 0 and less than the radius" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"hyperboloid\", \"center\": [0, 0, 0], \"radius\": 2, \"topRadius\": 1, \"height\": 1 } ]" ),
                 BAD_FILE, "topRadius must be at least the radius" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"superquadric\", \"center\": [0, 0, 0], \"radius\": 1, \"e1\": 0 } ]" ),
                 BAD_FILE, "e1 and e2 must be from 0.05 to 10" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"roundedBox\", \"center\": [0, 0, 0], \"size\": 1, \"radius\": 0.6 } ]" ),
                 BAD_FILE, "at most half the smallest size" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"torus\", \"center\": [0, 0, 0], \"majorRadius\": 1, \"minorRadius\": 0.2, \"spin\": 1 } ]" ),
                 BAD_FILE, "spin" );
}

    // Fonts and text: glyphs read from the scene's font, outlines filled
    // exactly, extruded text closed, and fonts Ray can't read refused.
static void TextChecks( const char *strScenes )
{
    std::string strFontFile = std::string( strScenes ) + "/fonts/Lato-Bold.ttf";
    std::string strData = ReadFile( strFontFile );
    CFontFile   oFont;
    std::string strError;

    Check( oFont.Load( strData, strError ), "the scene font loads" );
    if ( strError.empty() )
    {
        int nA = oFont.GlyphIndex( 'A' ), nV = oFont.GlyphIndex( 'V' ), nO = oFont.GlyphIndex( 'o' ), nB = oFont.GlyphIndex( 'B' );
        std::vector<CFontFile::Contour> aoContours;

        Check( oFont.UnitsPerEm() == 2000 && nA > 0 && oFont.Advance( nA ) > 0, "Lato's em and the A's advance are read" );
        Check( oFont.GlyphIndex( 0x10FFFF ) == 0, "a character the font lacks gives the missing glyph" );
        Check( oFont.Outline( nO, aoContours, strError ) && aoContours.size() == 2, "an o is two outlines: its bowl and its counter" );
        Check( oFont.Outline( nB, aoContours, strError ) && aoContours.size() == 3, "a B is three outlines" );
        Check( oFont.HasKerning() && oFont.Kerning( nA, nV ) < 0, "A and V are kerned closer (GPOS pair kerning)" );
        Check( oFont.Outline( oFont.GlyphIndex( ' ' ), aoContours, strError ) && aoContours.empty(), "a space has no outline" );

            // Every glyph of a sample fills exactly: its triangles' area is
            // the area its outlines enclose, holes taken out.
        bool bExact = true;

        for ( const char *p = "ABOR8g%@&Qeaj"; *p && bExact; p++ )
        {
            std::vector<Outline::Polygon> aoOutlines;
            Outline::SShape               oShape;
            double                        dLoops = 0.0, dTriangles = 0.0;

            if ( ! oFont.Outline( oFont.GlyphIndex( (uint32_t)*p ), aoContours, strError ) )
            {
                bExact = false;
                break;
            }
            for ( const CFontFile::Contour &oContour : aoContours )
                aoOutlines.push_back( Outline::Flatten( oContour, 2.0 ) );
            if ( ! Outline::Fill( aoOutlines, oShape, strError ) )
            {
                bExact = false;
                break;
            }
            for ( const std::vector<int> &anLoop : oShape.aanLoops )
            {
                Outline::Polygon oLoop;

                for ( int i : anLoop )
                    oLoop.push_back( oShape.aoPoints[ (size_t)i ] );
                dLoops += Outline::SignedArea2( oLoop );
            }
            for ( size_t t = 0; t + 2 < oShape.anTriangles.size(); t += 3 )
            {
                const Outline::SPoint &a = oShape.aoPoints[ (size_t)oShape.anTriangles[ t ] ];
                const Outline::SPoint &b = oShape.aoPoints[ (size_t)oShape.anTriangles[ t + 1 ] ];
                const Outline::SPoint &c = oShape.aoPoints[ (size_t)oShape.anTriangles[ t + 2 ] ];
                double                 dArea = ( ( b.x - a.x ) * ( c.y - a.y ) ) - ( ( b.y - a.y ) * ( c.x - a.x ) );

                if ( dArea < 0.0 )
                    bExact = false;
                dTriangles += dArea;
            }
            if ( ! ( dLoops > 0.0 ) || ABS( dTriangles - dLoops ) > 1e-9 * dLoops )
            {
                fprintf( stderr, "  glyph %c: outlines %.6f, triangles %.6f\n", *p, dLoops, dTriangles );
                bExact = false;
            }
        }
        Check( bExact, "glyphs are cut into triangles that cover exactly the area their outlines enclose" );
    }

        // Where to aim through letters: the left tenth of the R (its stem)
        // and the middle of the I, in ems, from their outlines.
    double dStemR = 0.12, dStemI = 0.15;

    for ( int nLetter = 0; nLetter < 2 && strError.empty(); nLetter++ )
    {
        std::vector<CFontFile::Contour> aoContours;
        double                          dLow = 1e300, dHigh = -1e300;

        if ( oFont.Outline( oFont.GlyphIndex( nLetter ? 'I' : 'R' ), aoContours, strError ) )
        {
            for ( const CFontFile::Contour &oContour : aoContours )
            {
                for ( const CFontFile::SPoint &oPoint : oContour )
                {
                    dLow = ( std::min )( dLow, oPoint.x );
                    dHigh = ( std::max )( dHigh, oPoint.x );
                }
            }
            if ( nLetter )
                dStemI = ( dLow + dHigh ) / 2.0 / oFont.UnitsPerEm();
            else
                dStemR = ( dLow + ( 0.1 * ( dHigh - dLow ) ) ) / oFont.UnitsPerEm();
        }
    }

        // Fonts Ray can't read are refused with a reason.
    CFontFile   oBad;
    std::string strCff( "OTTO\0\0\0\0\0\0\0\0", 12 );

    Check( ! oBad.Load( strCff, strError ) && strError.find( "CFF" ) != std::string::npos, "a font with CFF outlines is refused, saying why" );
    Check( ! oBad.Load( strData.substr( 0, 200 ), strError ), "a cut-off font is refused" );
    Check( ! oBad.Load( "not a font at all", strError ), "something that isn't a font is refused" );

        // Extruded text is a closed mesh: every edge (by position) is shared
        // by exactly two faces.
    CScene      oScene;
    std::string strText = "{ \"type\": \"text\", \"string\": \"Ray 8%\", \"font\": \"" + strFontFile + "\", \"size\": 1, \"depth\": 0.25 }";
    CText      *pText = dynamic_cast<CText *>( FirstObject( oScene, strText.c_str() ) );

    Check( pText != NULL && pText->GetFaceCount() > 100, "text loads, extruded into a mesh" );
    if ( pText && pText->GetGeometry() )
    {
        const CMesh::SGeometry                     &oGeometry = *pText->GetGeometry();
        std::map< std::vector<long long>, int >     oEdges;
        auto                                        Key = [ &oGeometry ]( int a, int b )
        {
            std::vector<long long> anKey;
            CVector                oA( oGeometry.aoVertices[ (size_t)a ] ), oB( oGeometry.aoVertices[ (size_t)b ] );
            long long              anA[ 3 ] = { llround( oA.GetX() * 1e7 ), llround( oA.GetY() * 1e7 ), llround( oA.GetZ() * 1e7 ) };
            long long              anB[ 3 ] = { llround( oB.GetX() * 1e7 ), llround( oB.GetY() * 1e7 ), llround( oB.GetZ() * 1e7 ) };

            if ( std::lexicographical_compare( anB, anB + 3, anA, anA + 3 ) )
                std::swap( anA, anB );
            anKey.assign( anA, anA + 3 );
            anKey.insert( anKey.end(), anB, anB + 3 );
            return anKey;
        };

        for ( size_t f = 0; f + 2 < oGeometry.anKept.size(); f += 3 )
        {
            for ( int k = 0; k < 3; k++ )
                oEdges[ Key( oGeometry.anKept[ f + (size_t)k ], oGeometry.anKept[ f + (size_t)( ( k + 1 ) % 3 ) ] ) ]++;
        }

        size_t nOpen = 0;

        for ( const auto &oEdge : oEdges )
            nOpen += ( oEdge.second != 2 ) ? 1 : 0;
        Check( nOpen == 0, "extruded text is watertight: every edge joins exactly two faces" );

            // A ray through the stem of the R hits its face; one through the
            // counter of the 8 passes through.
        CRay    oStem = RayFrom( dStemR, 0.35, -5, 0, 0, 1 );
        CVector oHit, oNormal;

        BOOL      bFlip = FALSE;
        CObjects *pFace = pText->Intersect( oStem, oHit ) ? pText->HitPart( bFlip ) : NULL;

        Check( pFace && ABS( oStem.GetT() - 5.0 ) < 1e-9 && pFace->SurfaceNormal( oHit, oNormal ) && oNormal.GetZ() < -0.999,
               "a ray hits the face of a letter, facing the viewer" );
    }

        // Text saves and reads back, and it works in a csg.
    if ( pText )
    {
        std::string strSaved = oScene.ToJson();
        CScene      oAgain;

        Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "text reads back as it was saved" );
    }

    std::string strCarved = CsgOf( "difference", ( "{ \"type\": \"box\", \"center\": [0.3, 0.35, 0], \"size\": [1, 1, 0.2] }, "
                                                   "{ \"type\": \"text\", \"string\": \"I\", \"font\": \"" + strFontFile +
                                                   "\", \"size\": 1, \"depth\": 1, \"position\": [0, 0, -0.5] }" ).c_str() );
    CObjects   *pCarved = FirstObject( oScene, strCarved.c_str() );

    if ( pCarved && pText )
    {
            // The I's stem: from its outline, about x = 0.08 to 0.22 em.
        Check( Misses( *pCarved, RayFrom( dStemI, 0.35, -5, 0, 0, 1 ) ), "a letter taken out of a slab with a csg leaves a hole through it" );
        Check( CsgHitsAt( *pCarved, RayFrom( 0.6, 0.35, -5, 0, 0, 1 ), 4.9, 0, 0, -1 ), "the slab is whole beside the letter" );
    }

        // Bad settings are reported.
    std::string strObjects = "\"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 } ]";
    std::string strFont = "\"font\": \"" + strFontFile + "\"";

    ExpectError( Minimal( strObjects.c_str(), ( "\"objects\": [ { \"type\": \"text\", \"string\": \"A\", " + strFont + ", \"align\": \"middle\" } ]" ).c_str() ),
                 BAD_FILE, "unknown align" );
    ExpectError( Minimal( strObjects.c_str(), ( "\"objects\": [ { \"type\": \"text\", \"string\": \"   \", " + strFont + " } ]" ).c_str() ),
                 BAD_FILE, "nothing to draw" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"text\", \"string\": \"A\", \"font\": \"no/such/font.ttf\" } ]" ),
                 BAD_FILE, "can't open the font" );
    ExpectError( Minimal( strObjects.c_str(), ( "\"objects\": [ { \"type\": \"text\", \"string\": \"A\", " + strFont + ", \"bevel\": 0.5, \"depth\": 0.2 } ]" ).c_str() ),
                 BAD_FILE, "bevel must be at least 0 and less than the depth" );
}

    // A solid checker of black and white cells strSize across, as JSON.
static std::string Checker( const char *strSize = "1" )
{
    return std::string( "{ \"type\": \"checker\", \"colors\": [[0, 0, 0], [1, 1, 1]], \"size\": " ) + strSize + ", \"solid\": true }";
}

static void MapChecks( void )
{
        // A map reads its texture, scales and offsets it, and keeps it in range.
    {
        IASLib::CJsonNode  *pNode = RayJson::Parse( "{ \"value\": { \"texture\": " + Checker() + ", \"scale\": 2, \"offset\": 0.5 } }" );
        CScalarMap          oMap( 7.0 );
        STexturePoint       oBlack;
        STexturePoint       oWhite;

        oBlack.bHasUV = oWhite.bHasUV = FALSE;
        oBlack.bHasColor = oWhite.bHasColor = FALSE;
        oBlack.nU = oBlack.nV = oWhite.nU = oWhite.nV = 0.0;
        oBlack.oPoint = CVector( 0.5, 0.5, 0.5 );
        oWhite.oPoint = CVector( 1.5, 0.5, 0.5 );
        Check( pNode && oMap.Read( *pNode, "value", FALSE ) && oMap.IsMapped(), "a number can be a texture" );

        FLT nBlack = oMap.At( oBlack );
        FLT nWhite = oMap.At( oWhite );

        Check( ( nBlack == 0.5 && nWhite == 2.5 ) || ( nBlack == 2.5 && nWhite == 0.5 ), "a map's value is offset + scale x the texture" );
        oMap.SetRange( 0.0, 1.0 );
        Check( oMap.At( oBlack ) <= 1.0 && oMap.At( oWhite ) <= 1.0, "a map's values are kept within the setting's range" );

        CScalarMap oPlain( 3.0 );

        Check( oPlain.At( oBlack ) == 3.0 && oPlain.Get() == 3.0 && ! oPlain.IsMapped(), "a constant is its value everywhere" );
        delete pNode;

        IASLib::CJsonNode  *pRed = RayJson::Parse( "{ \"value\": { \"texture\": { \"type\": \"checker\", \"colors\": "
                                                   "[[0.2, 0.4, 0.9], [0.2, 0.4, 0.9]], \"size\": 1, \"solid\": true }, "
                                                   "\"channel\": \"red\" } }" );
        CScalarMap          oRedMap;

        Check( pRed && oRedMap.Read( *pRed, "value", FALSE ) && ABS( oRedMap.At( oBlack ) - 0.2 ) < 1e-12,
               "a map can read one channel of its texture" );
        delete pRed;
    }

        // Every material setting takes a texture, and saves it back; plain
        // values are saved exactly as before.
    {
        std::string strMaterials =
            "\"radius\": 1, \"materials\": [ "
            "{ \"type\": \"ambient\", \"influence\": 1, \"color\": " + Checker() + " }, "
            "{ \"type\": \"diffuse\", \"influence\": { \"texture\": " + Checker() + ", \"scale\": 0.5 }, \"color\": " + Checker() + " }, "
            "{ \"type\": \"specular\", \"influence\": 0.3, \"power\": { \"texture\": " + Checker() + ", \"scale\": 0.5, \"offset\": 0.1 }, "
            "  \"indexOfRefraction\": " + Checker() + " }, "
            "{ \"type\": \"reflective\", \"influence\": 0.2, \"roughness\": " + Checker() + ", \"diffuseReflectivity\": " + Checker() + " }, "
            "{ \"type\": \"transparent\", \"influence\": 0.4, \"roughness\": " + Checker() + ", \"indexOfRefraction\": { \"texture\": " +
            Checker() + ", \"offset\": 1 }, \"attenuationColor\": " + Checker() + ", \"attenuationDistance\": 2, \"attenuationStep\": 0.1 } ], "
            "\"bump\": { \"texture\": " + Checker() + ", \"depth\": { \"texture\": " + Checker() + ", \"scale\": 0.01 } }";
        CScene      oScene;
        std::string strText = Minimal( "\"radius\": 1", strMaterials.c_str() );

        Check( oScene.FromJson( strText ) == 0, "every material setting can be a texture" );

        std::string strSaved = oScene.ToJson();
        CScene      oAgain;

        Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "textured settings round-trip" );
        Check( strSaved.find( "\"attenuationStep\": 0.1" ) != std::string::npos &&
               strSaved.find( "\"offset\": 1" ) != std::string::npos, "textured settings are saved as they were given" );

        CObjects *pBall = oScene.GetObjectAt( 0 );

        if ( pBall )
        {
            CMaterialList  &oMaterials = pBall->GetMaterials();
            CMapPoint       oBlack( CVector( 0.5, 0.5, 0.5 ), pBall );
            CMapPoint       oWhite( CVector( 1.5, 0.5, 0.5 ), pBall );
            FLT             nBlack = oMaterials.InfluenceAt( 1, oBlack );
            FLT             nWhite = oMaterials.InfluenceAt( 1, oWhite );

            Check( oMaterials.IsInfluenceMapped( 1 ) && ! oMaterials.IsInfluenceMapped( 0 ) && nBlack != nWhite &&
                   ( nBlack == 0.5 || nWhite == 0.5 ), "a material's influence can vary over the surface" );
            Check( oMaterials.InfluenceAt( 2, oBlack ) == 0.3, "a plain influence is its value" );
        }

        CScene oPlain;

        Check( oPlain.FromJson( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"reflective\", \"influence\": 0.5, "
                                                          "\"roughness\": 0.3 } ]" ) ) == 0 &&
               oPlain.ToJson().find( "\"roughness\": 0.3" ) != std::string::npos &&
               oPlain.ToJson().find( "\"influence\": 0.5" ) != std::string::npos, "plain settings are saved as plain numbers" );
    }

        // Absorption inside glass from a texture: a constant one is exactly
        // Beer-Lambert, and a checker through which half the path lies in
        // each color gives the color to the power of half the path.
    {
        std::string strConstant = ", \"attenuationColor\": { \"type\": \"checker\", \"colors\": [[0.5, 0.25, 1], [0.5, 0.25, 1]], "
                                  "\"size\": 1, \"solid\": true }, \"attenuationDistance\": 1";
        CColor      oMiddle = ThroughBall( strConstant.c_str(), 0.0 );
        CColor      oSide = ThroughBall( strConstant.c_str(), 0.6 );

        Check( ABS( oMiddle.GetR() - 0.25 ) < 1e-9 && ABS( oMiddle.GetG() - 0.0625 ) < 1e-9 && ABS( oMiddle.GetB() - 1.0 ) < 1e-9,
               "a constant texture inside glass absorbs exactly as Beer-Lambert" );
        Check( ABS( oSide.GetR() - pow( 0.5, 1.6 ) ) < 1e-9, "a textured absorption follows the path's length" );

        std::string strChecker = ", \"attenuationColor\": { \"type\": \"checker\", \"colors\": [[0.5, 0.5, 0.5], [1, 1, 1]], "
                                 "\"size\": 0.5, \"solid\": true }, \"attenuationDistance\": 1, \"attenuationStep\": ";
        double      dHalf = sqrt( 1.0 - 0.2 * 0.2 );     // half the chord at x = 0.2; half of it lies in each color
        double      dExpected = pow( 0.5, dHalf );
        CColor      oFine = ThroughBall( ( strChecker + "0.0005" ).c_str(), 0.2 );
        CColor      oCoarse = ThroughBall( ( strChecker + "0.1" ).c_str(), 0.2 );

        Check( ABS( oFine.GetR() - dExpected ) < 2e-3, "textured absorption adds up along the path, a step at a time" );
        Check( ABS( oFine.GetR() - dExpected ) <= ABS( oCoarse.GetR() - dExpected ) + 1e-9,
               "smaller steps come closer to the exact absorption" );

        CColor oAgain = ThroughBall( ( strChecker + "0.1" ).c_str(), 0.2 );

        Check( oAgain.GetR() == oCoarse.GetR(), "textured absorption is the same every time" );
    }

        // A light's tint and intensity are read at the point it lights.
    {
        CScene oScene;

        Check( oScene.FromJson( WithLights( "{ \"type\": \"point\", \"position\": [0, 10, 0], \"color\": [1, 0.5, 0.25], \"tint\": " +
                                            Checker() + ", \"intensity\": { \"texture\": " + Checker() + ", \"scale\": 2, \"offset\": 1 } }" ) ) == 0,
               "a light's tint and intensity can be textures" );

        CLight *pLight = oScene.GetLight( 0 );

        if ( pLight )
        {
            CVector oBlackPoint( 0.5, 0.5, 0.5 );
            CVector oWhitePoint( 1.5, 0.5, 0.5 );
            CVector oToBlack( CVector( 0, 10, 0 ) - oBlackPoint );
            CVector oToWhite( CVector( 0, 10, 0 ) - oWhitePoint );
            CColor  oAtBlack = pLight->ShadeAt( oBlackPoint, oToBlack );
            CColor  oAtWhite = pLight->ShadeAt( oWhitePoint, oToWhite );
            CColor  oDark( oAtBlack.GetR() < oAtWhite.GetR() ? oAtBlack : oAtWhite );
            CColor  oLit( oAtBlack.GetR() < oAtWhite.GetR() ? oAtWhite : oAtBlack );

            Check( oDark.GetR() == 0.0 && ABS( oLit.GetR() - 3.0 ) < 1e-12 && ABS( oLit.GetG() - 1.5 ) < 1e-12,
                   "what light arrives depends on where it lands" );
            Check( oScene.ToJson().find( "\"tint\"" ) != std::string::npos, "a light's tint is saved" );
        }

        CScene oPlain;

        Check( oPlain.FromJson( WithLights( "{ \"type\": \"point\", \"position\": [0, 10, 0], \"color\": [1, 1, 1] }" ) ) == 0 &&
               oPlain.ToJson().find( "\"tint\"" ) == std::string::npos &&
               oPlain.ToJson().find( "\"intensity\"" ) == std::string::npos, "a light without them saves neither" );
    }

        // Errors.
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"reflective\", \"roughness\": { \"scale\": 2 } } ]" ),
                 BAD_FILE, "an object with a \"texture\"" );
    ExpectError( Minimal( "\"radius\": 1", ( "\"radius\": 1, \"materials\": [ { \"type\": \"reflective\", \"roughness\": { \"texture\": " +
                                             Checker() + ", \"channel\": \"purple\" } } ]" ).c_str() ),
                 BAD_FILE, "roughness.channel: must be" );
    ExpectError( Minimal( "\"radius\": 1", ( "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"attenuationColor\": " +
                                             Checker() + " } ]" ).c_str() ),
                 BAD_FILE, "a textured attenuationColor needs an attenuationDistance" );
    ExpectError( Minimal( "\"radius\": 1", "\"radius\": 1, \"materials\": [ { \"type\": \"transparent\", \"attenuationStep\": 0 } ]" ),
                 BAD_FILE, "attenuationStep must be more than 0" );
    ExpectError( WithLights( "{ \"type\": \"point\", \"position\": [0, 10, 0], \"color\": [1, 1, 1], \"intensity\": -1 }" ),
                 BAD_FILE, "intensity: must be 0 or more" );
}

    // Draco-compressed glTF (KHR_draco_mesh_compression): NASA's Lunar
    // Module, decoded in Ray, against what Google's decoder makes of it
    // (counts, bounds and sums taken from a copy DracoPy decompressed), and
    // the errors for data that isn't Draco or is cut short.
static void DracoChecks( const char *strScenes )
{
    std::string strLm = ReadFile( std::string( strScenes ) + "/models/apollo/apollo_lunar_module.glb" );
    ModelFile::SModel oModel;
    std::string strError;

    Check( ModelFile::ReadGltf( strLm, NULL, "", "", oModel, strError ) == TRUE, "NASA's Draco-compressed Lunar Module reads" );
    Check( oModel.aoVertices.size() == 64787 && oModel.anFaces.size() == 97588 * 3,
           "the Lunar Module decodes to Google's 64,787 vertices and 97,588 faces" );
    Check( oModel.aoNormals.size() == oModel.aoVertices.size() && oModel.aoFaceColors.size() == 97588,
           "the Lunar Module keeps its normals and every face's material color" );

    double adMin[ 3 ] = { 1e300, 1e300, 1e300 }, adMax[ 3 ] = { -1e300, -1e300, -1e300 }, dSum = 0.0, dNormalSum = 0.0;

    for ( size_t i = 0; i < oModel.aoVertices.size(); i++ )
    {
        CVector v = oModel.aoVertices[ i ];
        double         ad[ 3 ] = { v.GetX(), v.GetY(), v.GetZ() };

        for ( int k = 0; k < 3; k++ )
        {
            adMin[ k ] = (std::min)( adMin[ k ], ad[ k ] );
            adMax[ k ] = (std::max)( adMax[ k ], ad[ k ] );
        }
        dSum += ad[ 0 ] + 2.0 * ad[ 1 ] + 3.0 * ad[ 2 ];
        if ( i < oModel.aoNormals.size() )
            dNormalSum += oModel.aoNormals[ i ].GetX() + 2.0 * oModel.aoNormals[ i ].GetY() + 3.0 * oModel.aoNormals[ i ].GetZ();
    }

    static const double s_adMin[ 3 ] = { -3.21450496, 0.102631003, -3.21093345 };
    static const double s_adMax[ 3 ] = { 3.21288991, 5.1154232, 3.21567702 };
    bool bBounds = true;

    for ( int k = 0; k < 3; k++ )
    {
        if ( fabs( adMin[ k ] - s_adMin[ k ] ) > 1e-6 || fabs( adMax[ k ] - s_adMax[ k ] ) > 1e-6 )
            bBounds = false;
    }
    Check( bBounds, "the decoded Lunar Module has Google's bounds" );
    Check( fabs( dSum - 290219.958138 ) < 1e-3 && fabs( dNormalSum - 2165.48721766 ) < 1e-4,
           "the decoded positions and normals add up to Google's, to the last digits" );

        // Damaged and foreign data are refused, not crashed on.
    Draco::SMesh oMesh;
    std::string  strWhy;

    Check( ! Draco::Decode( (const unsigned char *)"not draco data", 14, oMesh, strWhy ) && strWhy.find( "isn't Draco" ) != std::string::npos,
           "data that isn't Draco is refused" );

    unsigned char anOld[ 16 ] = { 'D', 'R', 'A', 'C', 'O', 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0 };

    Check( ! Draco::Decode( anOld, sizeof anOld, oMesh, strWhy ) && strWhy.find( "1.3" ) != std::string::npos,
           "an older Draco bitstream is refused by its version" );

    size_t nFirst = strLm.find( "DRACO" );
    bool   bCutShort = nFirst != std::string::npos;

    for ( size_t nLength = 11; bCutShort && nLength < 2000; nLength += 37 )
    {
        if ( Draco::Decode( (const unsigned char *)strLm.data() + nFirst, nLength, oMesh, strWhy ) )
            bCutShort = false;
    }
    Check( bCutShort, "Draco data cut short anywhere is refused" );
}

    // Blender .blend files: the two fixtures tools/make_blend_fixtures.py
    // writes, one in each of Blender's mesh layouts, plain and compressed.
static void BlendChecks( const char *strScenes )
{
    std::string strModels = std::string( strScenes ) + "/models/";
    std::string strLegacy = ReadFile( strModels + "blend_legacy.blend" );
    std::string strModern = ReadFile( strModels + "blend_modern.blend" );
    std::string strError;

    auto Has = [ & ]( const ModelFile::SModel &oModel, double x, double y, double z ) -> bool
    {
        for ( CVector v : oModel.aoVertices )
        {
            if ( fabs( v.GetX() - x ) < 1e-5 && fabs( v.GetY() - y ) < 1e-5 && fabs( v.GetZ() - z ) < 1e-5 )
                return true;
        }
        return false;
    };
    auto Colors = [ & ]( const ModelFile::SModel &oModel, double r, double g, double b ) -> int
    {
        int nCount = 0;

        for ( size_t i = 0; i < oModel.aoFaceColors.size(); i++ )
        {
            CColor c = oModel.aoFaceColors[ i ];

            if ( oModel.abHasColor[ i ] && fabs( c.GetR() - r ) < 1e-6 && fabs( c.GetG() - g ) < 1e-6 && fabs( c.GetB() - b ) < 1e-6 )
                nCount++;
        }
        return nCount;
    };

        // Blender 2.93's layout, with stored world matrices.
    ModelFile::SModel oLegacy;

    Check( ModelFile::IsBlend( strLegacy, "x.bin" ) == TRUE && ModelFile::IsBlend( "", "Model.BLEND" ) == TRUE,
           "a .blend file is told by its header, or by its name when compressed" );
    Check( ModelFile::ReadBlend( strLegacy, "", oLegacy, strError ) == TRUE, ".blend in Blender 2.93's layout reads" );
    Check( oLegacy.anFaces.size() == 13 * 3, "the cube's six quads and the triangle make 13 triangles" );
        // Cube corner (1, 1, 1): scaled by 2, turned 90 degrees about z,
        // moved to (1, 2, 3): (-1, 4, 5) in Blender, (-1, 5, 4) in Ray.
    Check( Has( oLegacy, -1.0, 5.0, 4.0 ) && Has( oLegacy, 3.0, 1.0, 0.0 ), "the cube is moved, turned and scaled by its world matrix" );
    Check( Has( oLegacy, 1.0, -5.0, 0.0 ) && Has( oLegacy, 0.0, -5.0, 1.0 ), "Blender's z up becomes Ray's y up" );
    Check( Colors( oLegacy, 0.8, 0.1, 0.1 ) == 4 && Colors( oLegacy, 0.1, 0.2, 0.9 ) == 8,
           "each face takes its material's color (two red faces, four blue)" );
    Check( Colors( oLegacy, 0.1, 0.7, 0.2 ) == 1, "an object's own material overrides its mesh's" );
    Check( oLegacy.anUVs.size() == oLegacy.aoVertices.size() * 2, "the MLoopUV layer gives surface coordinates" );

        // Faces keep their outward side: the cube's top face (y = 5) faces
        // up in a Ray mesh built from what was read.
    {
        CMesh oMesh;
        bool  bUp = false, bAny = false;

        oMesh.SetGeometry( oLegacy.aoVertices, oLegacy.anFaces, FALSE, FALSE );
        for ( size_t f = 0; f + 2 < oLegacy.anFaces.size(); f += 3 )
        {
            CVector a = oLegacy.aoVertices[ (size_t)oLegacy.anFaces[ f ] ];
            CVector b = oLegacy.aoVertices[ (size_t)oLegacy.anFaces[ f + 1 ] ];
            CVector c = oLegacy.aoVertices[ (size_t)oLegacy.anFaces[ f + 2 ] ];

            if ( fabs( a.GetY() - 5.0 ) < 1e-6 && fabs( b.GetY() - 5.0 ) < 1e-6 && fabs( c.GetY() - 5.0 ) < 1e-6 )
            {
                CVector oMiddle( ( a.GetX() + b.GetX() + c.GetX() ) / 3.0, 5.0, ( a.GetZ() + b.GetZ() + c.GetZ() ) / 3.0 );
                CVector oNormal = oMesh.FaceNormalAt( (int)( f / 3 ), oMiddle );

                bAny = true;
                bUp = ( oNormal.GetY() > 0.5 );
            }
        }
        Check( bAny && bUp, "faces keep their outward side through the change of handedness" );
    }

        // Blender 4.2's layout: attributes, face offsets, no world matrix.
    ModelFile::SModel oModern;

    Check( ModelFile::ReadBlend( strModern, "", oModern, strError ) == TRUE, ".blend in Blender 4.2's attribute layout reads" );
    Check( oModern.anFaces.size() == 7 * 3, "the pentagon is cut into three triangles, beside the tetrahedron's four" );
    Check( Has( oModern, 1.0, 1.0, 0.0 ), "the pentagon's object is moved up by its location" );
        // Child: turned 90 degrees about x, moved to (0, 0, 2), parented to
        // Base at (0, 0, 1): vertex (0, 0, 1) goes to (0, -1, 3) in
        // Blender, (0, 3, -1) in Ray.
    Check( Has( oModern, 0.0, 3.0, -1.0 ) && Has( oModern, 0.0, 4.0, 0.0 ) && Has( oModern, 1.0, 3.0, 0.0 ),
           "a world matrix is built from location, rotation, scale and the parent" );
    Check( Colors( oModern, 0.9, 0.7, 0.2 ) == 4 && Colors( oModern, 0.5, 0.5, 0.5 ) == 3,
           "the material_index face attribute picks each face's material" );

    bool bSecondMap = ! oModern.anUVs.empty();

    for ( size_t i = 0; i < oModern.anUVs.size(); i += 2 )
    {
        if ( oModern.anUVs[ i + 1 ] != 0.5 && oModern.anUVs[ i + 1 ] != 0.75 )
            bSecondMap = false;
    }
    Check( bSecondMap, "the active UV map is used, and Blender's hidden float2 layers aren't" );

        // One object by name, and the errors.
    ModelFile::SModel oOne;

    Check( ModelFile::ReadBlend( strLegacy, "Tri", oOne, strError ) == TRUE && oOne.anFaces.size() == 3,
           "\"node\" picks one object of a .blend file by name" );
    Check( ! ModelFile::ReadBlend( strLegacy, "Nope", oOne, strError ) && strError.find( "Nope" ) != std::string::npos,
           "a missing object is reported by name" );
    Check( ! ModelFile::ReadBlend( "not a blend file at all", "", oOne, strError ) && strError.find( "isn't a Blender file" ) != std::string::npos,
           "a file that isn't a .blend file is refused" );
    Check( ! ModelFile::ReadBlend( "BLENDER17-01v0500" + std::string( 64, '\0' ), "", oOne, strError ) &&
           strError.find( "Blender 5" ) != std::string::npos,
           "a Blender 5 file is refused with a reason" );

    bool bCut = true;

    for ( size_t nLength = 12; nLength < strLegacy.size(); nLength += 97 )
    {
        if ( ModelFile::ReadBlend( strLegacy.substr( 0, nLength ), "", oOne, strError ) )
            bCut = false;
    }
    Check( bCut, "a .blend file cut short anywhere is refused" );

        // Compressed: gzip (Blender 2.x) and Zstandard frames (3.0 on).
    for ( const char *strName : { "blend_legacy", "blend_modern" } )
    {
        std::string strRaw = ReadFile( strModels + strName + ".blend" );
        std::string strGz = ReadFile( strModels + strName + "_gz.blend" );
        std::string strZst = ReadFile( strModels + strName + "_zst.blend" );
        std::string strOut, strWhy;
        std::string strWhat = std::string( strName ) + ": ";

        Check( Decompress::IsGzip( strGz ) && Decompress::Gunzip( strGz, strOut, strWhy, (size_t)1 << 30 ) && strOut == strRaw,
               ( strWhat + "gzip unpacks to the file byte for byte" ).c_str() );
        Check( Decompress::IsZstd( strZst ) && Decompress::Unzstd( strZst, strOut, strWhy, (size_t)1 << 30 ) && strOut == strRaw,
               ( strWhat + "Zstandard frames and a skippable seek table unpack to the file byte for byte" ).c_str() );

        ModelFile::SModel oPlain, oPacked;

        Check( ModelFile::ReadBlend( strRaw, "", oPlain, strError ) && ModelFile::ReadBlend( strZst, "", oPacked, strError ) &&
               oPlain.aoVertices.size() == oPacked.aoVertices.size() && oPlain.anFaces == oPacked.anFaces,
               ( strWhat + "a compressed .blend file reads like the plain one" ).c_str() );

        std::string strDamaged( strZst );

        strDamaged.resize( strDamaged.size() / 2 );
        Check( ! Decompress::Unzstd( strDamaged, strOut, strWhy, (size_t)1 << 30 ), ( strWhat + "Zstandard data cut short is refused" ).c_str() );
        Check( ! Decompress::Unzstd( strZst, strOut, strWhy, 100 ), ( strWhat + "Zstandard data larger than the limit is refused" ).c_str() );
        strDamaged = strGz;
        strDamaged[ strDamaged.size() / 2 ] ^= 0x55;
        Check( ! Decompress::Gunzip( strDamaged.substr( 0, strDamaged.size() / 2 ), strOut, strWhy, (size_t)1 << 30 ),
               ( strWhat + "gzip data cut short is refused" ).c_str() );
    }
}


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

    // A small deterministic generator for the patch checks.
static double PatchRandom( unsigned int &nState )
{
    nState = ( nState * 1664525u ) + 1013904223u;
    return ( nState >> 8 ) / 16777216.0;
}

    // A cube of six flat patches (degree 1), [-1, 1] on each axis, every
    // patch's normal (the u slope crossed with the v slope) outward.
static std::string PatchCube( void )
{
    std::string strPatches;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        for ( int nSign = -1; nSign <= 1; nSign += 2 )
        {
            int nU = ( nSign > 0 ) ? ( nAxis + 1 ) % 3 : ( nAxis + 2 ) % 3;
            int nV = ( nSign > 0 ) ? ( nAxis + 2 ) % 3 : ( nAxis + 1 ) % 3;

            strPatches += strPatches.empty() ? "" : ", ";
            strPatches += "{ \"points\": [";
            for ( int t = 0; t <= 1; t++ )
            {
                for ( int u = 0; u <= 1; u++ )
                {
                    int an[ 3 ] = { 0, 0, 0 };

                    an[ nAxis ] = nSign;
                    an[ nU ] = ( 2 * u ) - 1;
                    an[ nV ] = ( 2 * t ) - 1;
                    strPatches += ( u + t > 0 ) ? ", " : "";
                    strPatches += "[" + std::to_string( an[ 0 ] ) + ", " + std::to_string( an[ 1 ] ) + ", " + std::to_string( an[ 2 ] ) + "]";
                }
            }
            strPatches += "] }";
        }
    }
    return "{ \"type\": \"bezier\", \"closed\": true, \"patches\": [ " + strPatches + " ] }";
}

    // A point of a patch turned and moved as the hump below is.
static void HumpToScene( const double anLocal[ 3 ], FLT anScene[ 3 ], bool bDirection )
{
    FLT nToRadians = PI / 180.0;

    anScene[ 0 ] = anLocal[ 0 ];
    anScene[ 1 ] = anLocal[ 1 ];
    anScene[ 2 ] = anLocal[ 2 ];
    CLocalShape::RotateXYZ( anScene, 10 * nToRadians, 25 * nToRadians, -5 * nToRadians );
    if ( ! bDirection )
    {
        anScene[ 0 ] += 0.3;
        anScene[ 1 ] += -0.2;
        anScene[ 2 ] += 0.1;
    }
}

static void PatchChecks( void )
{
    CScene      oScene;
    CObjects   *pObject;

        // Two flat bicubic patches side by side on y = 0, x from 0 to 2: a
        // ray straight down the seam between them must not slip through.
    std::string strFlat = "{ \"type\": \"bezier\", \"patches\": [ { \"points\": [";

    for ( int nPatch = 0; nPatch < 2; nPatch++ )
    {
        if ( nPatch == 1 )
            strFlat += "] }, { \"points\": [";
        for ( int j = 0; j <= 3; j++ )
        {
            for ( int i = 0; i <= 3; i++ )
                strFlat += std::string( ( i + j > 0 ) ? ", " : "" ) + "[" + std::to_string( nPatch + i / 3.0 ) + ", 0, " + std::to_string( j / 3.0 ) + "]";
        }
    }
    strFlat += "] } ] }";
    pObject = FirstObject( oScene, strFlat.c_str() );
    if ( pObject )
    {
        CRay    oDown = RayFrom( 0.5, 5, 0.5, 0, -1, 0 );
        CVector oHit, oNormal;

        Check( pObject->Intersect( oDown, oHit ) && ABS( oDown.GetT() - 5.0 ) < 1e-9 && pObject->SurfaceNormal( oHit, oNormal ) &&
               ABS( ABS( oNormal.GetY() ) - 1.0 ) < 1e-9, "a ray hits a flat patch, where its normal is straight up or down" );

        bool bSeam = true;

        for ( int n = 0; n <= 20 && bSeam; n++ )
        {
            CRay oRay = RayFrom( 1.0, 3.0, n / 20.0, 0.0, -1.0, 0.0 );

            bSeam = pObject->Intersect( oRay, oHit ) && ABS( oRay.GetT() - 3.0 ) < 1e-9;
        }
        Check( bSeam, "rays straight down the seam between two patches, corners included, all hit it" );
        Check( Misses( *pObject, RayFrom( 2.01, 3.0, 0.5, 0, -1, 0 ) ) && Misses( *pObject, RayFrom( 0.5, 3.0, 1.01, 0, -1, 0 ) ),
               "rays just past a patch's edges miss it" );
        Check( ! pObject->IsClosed(), "patches are open unless they say they are closed" );
    }

        // A curved bicubic patch, turned and moved: rays aimed at points
        // worked out straight from the patch, from either side, meet it
        // exactly there, with the normal the patch's own slopes give.
    std::string strHump = "{ \"type\": \"bezier\", \"center\": [0.3, -0.2, 0.1], \"rotation\": [10, 25, -5], \"patches\": [ { \"points\": [";
    double      anHeights[ 16 ] = { 0, 0.2, 0.1, 0, 0.3, 1.2, 0.9, 0.2, 0.1, 1.0, 1.4, 0.3, 0, 0.2, 0.4, 0.1 };

    for ( int n = 0; n < 16; n++ )
        strHump += std::string( n ? ", " : "" ) + "[" + std::to_string( ( n % 4 ) / 3.0 ) + ", " + std::to_string( anHeights[ n ] ) + ", " +
                   std::to_string( ( n / 4 ) / 3.0 ) + "]";
    strHump += "] } ] }";
    pObject = FirstObject( oScene, strHump.c_str() );

    CBezier *pHump = dynamic_cast<CBezier *>( pObject );

    if ( pHump )
    {
        const CBezier::SPatch &oPatch = pHump->GetPatches()[ 0 ];
        unsigned int           nState = 12345u;
        int                    nRight = 0, nTotal = 400;

        for ( int n = 0; n < nTotal; n++ )
        {
            double u = PatchRandom( nState ), v = PatchRandom( nState );
            double anLocal[ 3 ], anDu[ 3 ], anDv[ 3 ], anNormal[ 3 ];
            FLT    anPoint[ 3 ], anTurned[ 3 ];

            CBezier::Evaluate( oPatch, u, v, anLocal, anDu, anDv );
            CBezier::Normal( oPatch, u, v, anNormal );
            HumpToScene( anLocal, anPoint, false );
            HumpToScene( anNormal, anTurned, true );

                // From the side the normal faces, or the other, at a slant.
            double nSide = ( PatchRandom( nState ) < 0.5 ) ? 1.0 : -1.0;
            double anFrom[ 3 ];

            for ( int c = 0; c < 3; c++ )
                anFrom[ c ] = anPoint[ c ] + ( nSide * 3.0 * anTurned[ c ] ) + ( PatchRandom( nState ) - 0.5 );

            CRay    oRay = RayFrom( anFrom[ 0 ], anFrom[ 1 ], anFrom[ 2 ], anPoint[ 0 ] - anFrom[ 0 ], anPoint[ 1 ] - anFrom[ 1 ],
                                    anPoint[ 2 ] - anFrom[ 2 ] );
            CVector oHit, oNormal;

            if ( ! pObject->Intersect( oRay, oHit ) )
                continue;

            double nMiss = sqrt( ( ( oHit.GetX() - anPoint[ 0 ] ) * ( oHit.GetX() - anPoint[ 0 ] ) ) +
                                 ( ( oHit.GetY() - anPoint[ 1 ] ) * ( oHit.GetY() - anPoint[ 1 ] ) ) +
                                 ( ( oHit.GetZ() - anPoint[ 2 ] ) * ( oHit.GetZ() - anPoint[ 2 ] ) ) );

                // The same point with the patch's normal; or the ray met the
                // patch on its way there (a fold), which is right too.
            if ( nMiss < 1e-8 && pObject->SurfaceNormal( oHit, oNormal ) &&
                 ABS( ABS( ( oNormal.GetX() * anTurned[ 0 ] ) + ( oNormal.GetY() * anTurned[ 1 ] ) + ( oNormal.GetZ() * anTurned[ 2 ] ) ) - 1.0 ) < 1e-9 )
                nRight++;
            else if ( oRay.GetT() < 1.0 - 1e-9 )
                nRight++;
        }
        Check( nRight == nTotal, "rays aimed at 400 points of a curved, turned patch meet it there, with its own normal" );

            // Surface coordinates are the patch's (u, v), even for a point
            // it didn't just hit.
        double  anLocal[ 3 ], anDu[ 3 ], anDv[ 3 ];
        FLT     anPoint[ 3 ], nU, nV;
        CVector oPoint;

        CBezier::Evaluate( oPatch, 0.25, 0.75, anLocal, anDu, anDv );
        HumpToScene( anLocal, anPoint, false );
        oPoint.Set( anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] );
        Check( pObject->SurfaceCoords( oPoint, nU, nV ) && ABS( nU - 0.25 ) < 1e-6 && ABS( nV - 0.75 ) < 1e-6,
               "a patch's surface coordinates are its (u, v)" );
    }

        // A patch whose edge shrinks to a point still has a normal there.
    {
        CBezier::SPatch oCone;
        double          anNormal[ 3 ];

        oCone.anPoints.clear();
        for ( int j = 0; j <= 3; j++ )
        {
            for ( int i = 0; i <= 3; i++ )
            {
                double a = ( PI / 2.0 ) * i / 3.0, r = j / 3.0;

                oCone.anPoints.push_back( r * cos( a ) );
                oCone.anPoints.push_back( 1.0 - r );
                oCone.anPoints.push_back( r * sin( a ) );
                oCone.anPoints.push_back( 1.0 );
            }
        }
        CBezier::Normal( oCone, 0.5, 0.0, anNormal );

        double nLength = sqrt( ( anNormal[ 0 ] * anNormal[ 0 ] ) + ( anNormal[ 1 ] * anNormal[ 1 ] ) + ( anNormal[ 2 ] * anNormal[ 2 ] ) );

        Check( nLength == nLength && ABS( nLength - 1.0 ) < 1e-9, "a patch has a finite, unit normal where an edge shrinks to a point" );
    }

        // A closed box of patches is a solid: a csg can use it.
    pObject = FirstObject( oScene, PatchCube().c_str() );
    if ( pObject )
    {
        Check( pObject->IsClosed() && ShapeHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ),
               "a box of six patches, closed, is hit on its near face with an outward normal" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, 0, 1, 0, 0 ), 1.0, 1, 0, 0 ), "a ray from inside the closed patches leaves by a wall" );
    }
    pObject = FirstObject( oScene, CsgOf( "intersection", ( PatchCube() + ", { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1.2 }" ).c_str() ).c_str() );
    if ( pObject )
    {
        Check( CsgHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a csg of closed patches and a sphere meets the patches' face" );
        Check( Misses( *pObject, RayFrom( 0.9, 0.9, -5, 0, 0, 1 ) ), "the corners of the patch box outside the sphere are cut away" );
    }

        // B-spline surfaces become Bezier patches that are the same surface:
        // a bicubic net with uneven knots, and a rational one with
        // unclamped, evenly spaced knots.
    for ( int nCase = 0; nCase < 2; nCase++ )
    {
        CBSpline            oSurface;
        std::vector<double> anNet, anKnotsU, anKnotsV;
        unsigned int        nState = 777u + nCase;
        int                 nU = 7, nV = 6;

        for ( int n = 0; n < nU * nV; n++ )
        {
            double w = ( nCase == 0 ) ? 1.0 : 0.5 + PatchRandom( nState );

            anNet.push_back( ( ( n % nU ) + ( PatchRandom( nState ) * 0.3 ) ) * w );
            anNet.push_back( PatchRandom( nState ) * w );
            anNet.push_back( ( ( n / nU ) + ( PatchRandom( nState ) * 0.3 ) ) * w );
            anNet.push_back( w );
        }
        if ( nCase == 0 )
        {
            double anU[] = { 0, 0, 0, 0, 0.3, 0.5, 0.5, 1, 1, 1, 1 };
            double anV[] = { 0, 0, 0, 0, 0.6, 1, 1, 1, 1, 1 };

            anKnotsU.assign( anU, anU + 11 );
            anKnotsV.assign( anV, anV + 10 );
        }
        else
        {
            for ( int n = 0; n < nU + 4; n++ )
                anKnotsU.push_back( n );
            for ( int n = 0; n < nV + 4; n++ )
                anKnotsV.push_back( n * 0.5 );
        }
        Check( oSurface.SetSurface( 3, 3, nU, nV, anNet, anKnotsU, anKnotsV ) ? true : false,
               nCase ? "a rational B-spline with unclamped knots is cut into patches" : "a B-spline with uneven knots is cut into patches" );

        double                              anRange[ 4 ], nWorst = 0.0;
        const std::vector<CBezier::SPatch> &aoPatches = oSurface.GetPatches();

        oSurface.Range( anRange );
        for ( int n = 0; n < 200; n++ )
        {
            double s = PatchRandom( nState ), t = PatchRandom( nState );
            double anExact[ 3 ], anPoint[ 3 ], anDu[ 3 ], anDv[ 3 ];

            oSurface.SurfacePoint( anRange[ 0 ] + ( s * ( anRange[ 1 ] - anRange[ 0 ] ) ), anRange[ 2 ] + ( t * ( anRange[ 3 ] - anRange[ 2 ] ) ), anExact );
            for ( size_t nPatch = 0; nPatch < aoPatches.size(); nPatch++ )
            {
                const CBezier::SPatch &oPatch = aoPatches[ nPatch ];

                if ( s < oPatch.anUV[ 0 ] || s > oPatch.anUV[ 2 ] || t < oPatch.anUV[ 1 ] || t > oPatch.anUV[ 3 ] )
                    continue;
                CBezier::Evaluate( oPatch, ( s - oPatch.anUV[ 0 ] ) / ( oPatch.anUV[ 2 ] - oPatch.anUV[ 0 ] ),
                                   ( t - oPatch.anUV[ 1 ] ) / ( oPatch.anUV[ 3 ] - oPatch.anUV[ 1 ] ), anPoint, anDu, anDv );
                for ( int c = 0; c < 3; c++ )
                    nWorst = ( ABS( anPoint[ c ] - anExact[ c ] ) > nWorst ) ? ABS( anPoint[ c ] - anExact[ c ] ) : nWorst;
                break;
            }
        }
        Check( nWorst < 1e-12, nCase ? "its patches match the rational B-spline exactly, point for point" :
                                       "its patches match the B-spline exactly, point for point" );
    }

        // A NURBS torus (rational quadratic circles both ways) is a torus:
        // every point is exactly the tube's radius from its center circle.
    {
        CBSpline            oTorus;
        std::vector<double> anNet, anKnots;
        double              s = sqrt( 0.5 );
        double              anCircle[ 9 ][ 3 ] = { { 1, 0, 1 }, { 1, 1, s }, { 0, 1, 1 }, { -1, 1, s }, { -1, 0, 1 },
                                                   { -1, -1, s }, { 0, -1, 1 }, { 1, -1, s }, { 1, 0, 1 } };
        double              anK[] = { 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4 };

        for ( int j = 0; j < 9; j++ )
        {
            for ( int i = 0; i < 9; i++ )
            {
                double nRadius = 2.0 + ( 0.5 * anCircle[ j ][ 0 ] ), w = anCircle[ j ][ 2 ] * anCircle[ i ][ 2 ];

                anNet.push_back( nRadius * anCircle[ i ][ 0 ] * w );
                anNet.push_back( 0.5 * anCircle[ j ][ 1 ] * w );
                anNet.push_back( nRadius * anCircle[ i ][ 1 ] * w );
                anNet.push_back( w );
            }
        }
        anKnots.assign( anK, anK + 12 );
        Check( oTorus.SetSurface( 2, 2, 9, 9, anNet, anKnots, anKnots ) && oTorus.GetPatches().size() == 16,
               "a NURBS torus is cut into sixteen rational patches" );

        unsigned int nState = 99u;
        double       nWorst = 0.0;

        for ( size_t nPatch = 0; nPatch < oTorus.GetPatches().size(); nPatch++ )
        {
            for ( int n = 0; n < 20; n++ )
            {
                double anPoint[ 3 ], anDu[ 3 ], anDv[ 3 ];

                CBezier::Evaluate( oTorus.GetPatches()[ nPatch ], PatchRandom( nState ), PatchRandom( nState ), anPoint, anDu, anDv );

                double nAcross = sqrt( ( anPoint[ 0 ] * anPoint[ 0 ] ) + ( anPoint[ 2 ] * anPoint[ 2 ] ) ) - 2.0;
                double nOff = ABS( sqrt( ( nAcross * nAcross ) + ( anPoint[ 1 ] * anPoint[ 1 ] ) ) - 0.5 );

                nWorst = ( nOff > nWorst ) ? nOff : nWorst;
            }
        }
        Check( nWorst < 1e-12, "every point of the NURBS torus is exactly the tube's radius from its center circle" );

        CRay    oDown = RayFrom( 2.0, 5.0, 0.0, 0.0, -1.0, 0.0 );
        CVector oHit;

        Check( oTorus.Intersect( oDown, oHit ) && ABS( oDown.GetT() - 4.5 ) < 1e-9, "a ray down onto the NURBS torus's tube hits its top" );
    }

        // Saving keeps the patches as they came: shared points, indices,
        // weights, a degree and a uv rectangle.
    {
        const char *strShared = "{ \"type\": \"bezier\", \"points\": [[0, 0, 0], [1, 0, 0], [2, 0, 0], [0, 1, 0], [1, 1, 0, 2], [2, 1, 0],"
                                " [0, 0, 1], [1, 0, 1], [2, 0, 1]], \"patches\": [ { \"degree\": [2, 2], \"indices\": [0, 1, 2, 3, 4, 5, 6, 7, 8],"
                                " \"uv\": [0, 0, 0.5, 1] }, { \"degree\": [2, 1], \"points\": [[0, 2, 0], [1, 2, 0], [2, 2, 0], [0, 3, 0], [1, 3, 0], [2, 3, 0]] } ],"
                                " \"scale\": 2, \"closed\": false }";
        CScene      oFirst, oSecond, oThird;

        pObject = FirstObject( oFirst, strShared );
        if ( pObject )
        {
            std::string strSaved = oFirst.ToJson();

            Check( oSecond.FromJson( strSaved ) == 0 && oSecond.ToJson() == strSaved, "patches given as shared points and indices round-trip" );
            Check( strSaved.find( "\"indices\"" ) != std::string::npos && strSaved.find( "\"uv\"" ) != std::string::npos &&
                   strSaved.find( "\"scale\": 2" ) != std::string::npos, "saving keeps the indices, the uv rectangle and the scale" );
        }

        const char *strSpline = "{ \"type\": \"bspline\", \"degree\": [2, 1], \"points\": [ [[0, 0, 0], [1, 1, 0, 0.5], [2, 0, 0]],"
                                " [[0, 0, 1], [1, 1, 1], [2, 0, 1]] ], \"knotsU\": [0, 0, 0, 2, 2, 2] }";

        pObject = FirstObject( oThird, strSpline );
        if ( pObject )
        {
            std::string strSaved = oThird.ToJson();

            Check( oSecond.FromJson( strSaved ) == 0 && oSecond.ToJson() == strSaved, "a B-spline surface round-trips" );
        }
    }

        // What's wrong is reported.
    const char *strSphere = "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1";

    ExpectError( Minimal( strSphere, "\"type\": \"bezier\"" ), BAD_FILE, "needs patches" );
    ExpectError( Minimal( strSphere, "\"type\": \"bezier\", \"patches\": [ { \"points\": [[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0], [2, 2, 2]] } ]" ),
                 BAD_FILE, "not a square number" );
    ExpectError( Minimal( strSphere, "\"type\": \"bezier\", \"patches\": [ { \"degree\": [2, 2], \"points\": [[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]] } ]" ),
                 BAD_FILE, "needs 9 points" );
    ExpectError( Minimal( strSphere, "\"type\": \"bezier\", \"points\": [[0, 0, 0]], \"patches\": [ { \"indices\": [0, 0, 0, 1] } ]" ),
                 BAD_FILE, "indices must be whole numbers from 0 to 0" );
    ExpectError( Minimal( strSphere, "\"type\": \"bezier\", \"patches\": [ { \"points\": [[0, 0, 0], [1, 0, 0, 0], [0, 1, 0], [1, 1, 0]] } ]" ),
                 BAD_FILE, "a weight must be more than 0" );
    ExpectError( Minimal( strSphere, "\"type\": \"bezier\", \"file\": \"x.bpt\", \"patches\": []" ), BAD_FILE, "either a file or patches" );
    ExpectError( Minimal( strSphere, "\"type\": \"bezier\", \"scale\": 0, \"patches\": [ { \"points\": [[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]] } ]" ),
                 BAD_FILE, "scale: must be more than 0" );
    ExpectError( Minimal( strSphere, "\"type\": \"bspline\", \"points\": [ [[0, 0, 0], [1, 0, 0]], [[0, 1, 0], [1, 1, 0], [2, 1, 0]] ]" ), BAD_FILE,
                 "every row needs the same number of points" );
    ExpectError( Minimal( strSphere, "\"type\": \"bspline\", \"degree\": 1, \"points\": [ [[0, 0, 0], [1, 0, 0]], [[0, 1, 0], [1, 1, 0]] ],"
                                     " \"knotsU\": [0, 0, 1]" ), BAD_FILE, "knotsU needs 4 knots" );
    ExpectError( Minimal( strSphere, "\"type\": \"bspline\", \"degree\": 1, \"points\": [ [[0, 0, 0], [1, 0, 0]], [[0, 1, 0], [1, 1, 0]] ],"
                                     " \"knotsV\": [0, 1, 0, 1]" ), BAD_FILE, "knotsV must never decrease" );
    ExpectError( Minimal( strSphere, "\"type\": \"bspline\", \"points\": [ [[0, 0, 0], [1, 0, 0]], [[0, 1, 0], [1, 1, 0]] ]" ), BAD_FILE,
                 "needs at least degree + 1 points each way" );

        // A .bpt file's text, and its mistakes.
    std::vector<CBezier::SPatch> aoRead;
    std::string                  strError;

    Check( CBezier::ReadBpt( "1\n1 1\n0 0 0\n1 0 0\n0 1 0\n1 1 0\n", aoRead, strError ) && aoRead.size() == 1 && aoRead[ 0 ].nDegreeU == 1,
           "a .bpt file of one bilinear patch reads" );
    Check( ! CBezier::ReadBpt( "2\n1 1\n0 0 0\n1 0 0\n0 1 0\n1 1 0\n", aoRead, strError ) && strError.find( "ends before patch 2" ) != std::string::npos,
           "a .bpt file that ends early is refused" );
    Check( ! CBezier::ReadBpt( "1\n1 1\n0 0 0\n1 0 0\n0 1 0\n1 1 0\nextra\n", aoRead, strError ), "a .bpt file with more after its last patch is refused" );
}

    // The Utah teapot's own patches, against a fine mesh of them.
    // How many edges of a mesh (matched by position) don't join exactly two
    // faces: 0 for a watertight mesh.
static size_t OpenEdges( const CMesh::SGeometry &oGeometry )
{
    std::map< std::vector<long long>, int > oEdges;
    auto                                    Key = [ &oGeometry ]( int a, int b )
    {
        std::vector<long long> anKey;
        CVector                oA( oGeometry.aoVertices[ (size_t)a ] ), oB( oGeometry.aoVertices[ (size_t)b ] );
        long long              anA[ 3 ] = { llround( oA.GetX() * 1e7 ), llround( oA.GetY() * 1e7 ), llround( oA.GetZ() * 1e7 ) };
        long long              anB[ 3 ] = { llround( oB.GetX() * 1e7 ), llround( oB.GetY() * 1e7 ), llround( oB.GetZ() * 1e7 ) };

        if ( std::lexicographical_compare( anB, anB + 3, anA, anA + 3 ) )
            std::swap( anA, anB );
        anKey.assign( anA, anA + 3 );
        anKey.insert( anKey.end(), anB, anB + 3 );
        return anKey;
    };

    for ( size_t f = 0; f + 2 < oGeometry.anKept.size(); f += 3 )
    {
        for ( int k = 0; k < 3; k++ )
            oEdges[ Key( oGeometry.anKept[ f + (size_t)k ], oGeometry.anKept[ f + (size_t)( ( k + 1 ) % 3 ) ] ) ]++;
    }

    size_t nOpen = 0;

    for ( const auto &oEdge : oEdges )
        nOpen += ( oEdge.second != 2 ) ? 1 : 0;
    return nOpen;
}

    // A point of a cubic piece (four control points in r and y) at s.
static void PiecePoint( const double ( &anPiece )[ 4 ][ 2 ], double s, double &dR, double &dY )
{
    double u = 1.0 - s;

    dR = ( u * u * u * anPiece[ 0 ][ 0 ] ) + ( 3 * u * u * s * anPiece[ 1 ][ 0 ] ) + ( 3 * u * s * s * anPiece[ 2 ][ 0 ] ) +
         ( s * s * s * anPiece[ 3 ][ 0 ] );
    dY = ( u * u * u * anPiece[ 0 ][ 1 ] ) + ( 3 * u * u * s * anPiece[ 1 ][ 1 ] ) + ( 3 * u * s * s * anPiece[ 2 ][ 1 ] ) +
         ( s * s * s * anPiece[ 3 ][ 1 ] );
}

    // How far (r, y) is from a profile of cubic pieces: a hit on a lathe
    // lies on its surface when this is 0.
static double OffProfile( double dR, double dY, const double ( *anPieces )[ 4 ][ 2 ], int nPieces )
{
    double dBest = 1e300;

    for ( int n = 0; n < nPieces; n++ )
    {
            // Coarse to fine search of the piece's parameter.
        double dLow = 0.0, dHigh = 1.0;

        for ( int nPass = 0; nPass < 8; nPass++ )
        {
            double dBestS = dLow, dBestHere = 1e300;

            for ( int k = 0; k <= 64; k++ )
            {
                double s = dLow + ( ( dHigh - dLow ) * k / 64.0 ), r, y;

                PiecePoint( anPieces[ n ], s, r, y );

                double d = hypot( r - dR, y - dY );

                if ( d < dBestHere )
                {
                    dBestHere = d;
                    dBestS = s;
                }
            }
            if ( dBestHere < dBest )
                dBest = dBestHere;

            double dStep = ( dHigh - dLow ) / 64.0;

            dLow = ( dBestS - dStep > 0.0 ) ? dBestS - dStep : 0.0;
            dHigh = ( dBestS + dStep < 1.0 ) ? dBestS + dStep : 1.0;
        }
    }
    return dBest;
}

static const char *s_strSphereObject = "\"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1";

static void LatheChecks( void )
{
    CScene      oScene;
    CObjects   *pObject;
    CVector     oHit;

        // A straight profile makes a capped cylinder of radius 1, y 0 to 2.
    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 0], [1, 0], [1, 2], [0, 2] ] }" );
    if ( pObject )
    {
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 1, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a ray hits a lathe cylinder's side" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0.5, 5, 0, 0, -1, 0 ), 3.0, 0, 1, 0 ), "a ray hits a lathe cylinder's top" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0.5, -5, 0, 0, 1, 0 ), 5.0, 0, -1, 0 ), "a ray hits a lathe cylinder's base" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 1, 0, 1, 0, 0 ), 1.0, 1, 0, 0 ), "a ray from inside a lathe cylinder leaves by its side" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ), 3.0, 0, 1, 0 ), "a ray down the axis hits a lathe's top at the axis" );

        CRay oRim = RayFrom( 1, 5, 0, 0, -1, 0 );

        Check( pObject->Intersect( oRim, oHit ) && ABS( oRim.GetT() - 3.0 ) < 1e-9,
               "a ray down onto a lathe's rim, where two pieces join, hits it" );
        Check( Misses( *pObject, RayFrom( 0, 2.5, -5, 0, 0, 1 ) ), "a ray over a lathe cylinder misses it" );
        Check( Misses( *pObject, RayFrom( 1.01, 1, -5, 0, 0, 1 ) ), "a ray just beside a lathe cylinder misses it" );
        Check( pObject->IsClosed(), "a profile from the axis back to the axis makes a solid" );
    }

        // The same profile run the other way round: the normals still point out.
    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 2], [1, 2], [1, 0], [0, 0] ] }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 1, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a lathe's normals point out whichever way its profile runs" );

        // A cone: from radius 1 at y = 0 to the axis at y = 1.
    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 0], [1, 0], [0, 1] ] }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0.5, -5, 0, 0, 1 ), 4.5, 0, sqrt( 0.5 ), -sqrt( 0.5 ) ),
               "a ray hits a lathe cone's side where its radius is right, with the slanted normal" );

        // An open wall is a shell, not a solid; capped closes it.
    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [1, 0], [1, 1] ] }" );
    if ( pObject )
    {
        Check( ! pObject->IsClosed(), "an open lathe profile is a shell" );
        Check( Misses( *pObject, RayFrom( 0.5, 5, 0, 0, -1, 0 ) ), "an open lathe shell is open at its ends" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [1, 0], [1, 1] ], \"capped\": true }" );
    if ( pObject )
    {
        Check( pObject->IsClosed(), "a capped lathe is a solid" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0.5, 5, 0, 0, -1, 0 ), 4.0, 0, 1, 0 ), "a capped lathe's top is closed by a flat disc" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0.5, -5, 0, 0, 1, 0 ), 5.0, 0, -1, 0 ), "a capped lathe's base is closed by a flat disc" );
    }

        // A curved profile (a ball from two cubics): rays aimed at points of
        // it along the normal there hit those points, at the joint and the
        // poles too, and every hit lies on the surface.
    static const double anBall[ 2 ][ 4 ][ 2 ] = { { { 0, 0 }, { 0.36, 0 }, { 0.62, 0.22 }, { 0.62, 0.5 } },
                                                  { { 0.62, 0.5 }, { 0.62, 0.78 }, { 0.36, 1.0 }, { 0, 1.0 } } };

    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 0], "
                                   "{ \"cubic\": [ [0.36, 0], [0.62, 0.22], [0.62, 0.5] ] }, "
                                   "{ \"cubic\": [ [0.62, 0.78], [0.36, 1.0], [0, 1.0] ] } ] }" );
    if ( pObject )
    {
        double dWorst = 0.0;
        int    nMissed = 0, nWrongT = 0;

        for ( int n = 0; n < 400; n++ )
        {
            int    nPiece = n % 2;
            double s = ( n % 20 ) / 19.0, a = n * 2.399963, r, y, r1, y1, r2, y2;

                // The normal from the slope, taken just inside at the poles.
            double sa = ( s < 0.5 ) ? s : s - 1e-6, sb = ( s < 0.5 ) ? s + 1e-6 : s;

            PiecePoint( anBall[ nPiece ], s, r, y );
            PiecePoint( anBall[ nPiece ], sa, r1, y1 );
            PiecePoint( anBall[ nPiece ], sb, r2, y2 );

            double l = hypot( r2 - r1, y2 - y1 ), nr = ( y2 - y1 ) / l, ny = -( r2 - r1 ) / l;
            double px = r * cos( a ), pz = r * sin( a ), nx = nr * cos( a ), nz = nr * sin( a );
            CRay   oRay = RayFrom( px + ( 3 * nx ), y + ( 3 * ny ), pz + ( 3 * nz ), -nx, -ny, -nz );

            if ( ! pObject->Intersect( oRay, oHit ) )
            {
                nMissed++;
                continue;
            }
            if ( ABS( oRay.GetT() - 3.0 ) > 1e-6 )
                nWrongT++;

            double dOff = OffProfile( hypot( oHit.GetX(), oHit.GetZ() ), oHit.GetY(), anBall, 2 );

            dWorst = ( dOff > dWorst ) ? dOff : dWorst;
        }
        Check( nMissed == 0, "rays aimed at a curved lathe, its joints and its poles, all hit it" );
        Check( nWrongT == 0, "each hits a curved lathe where it was aimed" );
        Check( dWorst < 1e-8, "every hit on a curved lathe lies on its surface" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ), 4.0, 0, 1, 0 ), "a ray down the axis hits a curved lathe's pole" );
    }

        // A quarter turn of the cylinder: the end faces close it.
    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"sweep\": 90, "
                                   "\"profile\": [ [0, 0], [1, 0], [1, 2], [0, 2] ] }" );
    if ( pObject )
    {
        Check( ShapeHitsAt( *pObject, RayFrom( 0.5, 1, -5, 0, 0, 1 ), 5.0, 0, 0, -1 ), "a ray hits a partial lathe's first end face" );
        Check( ShapeHitsAt( *pObject, RayFrom( -5, 1, 0.5, 1, 0, 0 ), 5.0, -1, 0, 0 ), "a ray hits a partial lathe's last end face" );
        Check( ShapeHitsAt( *pObject, RayFrom( 5, 1, 0.5, -1, 0, 0 ), 5.0 - sqrt( 0.75 ), sqrt( 0.75 ), 0, 0.5 ),
               "a ray hits a partial lathe's side within its sweep" );
        Check( Misses( *pObject, RayFrom( -0.5, 1, -5, 0, 0, 1 ) ), "a ray through the part a partial lathe leaves out misses it" );
        Check( pObject->IsClosed(), "a partial lathe of a closed profile is a solid" );
    }

        // A lathe in a csg: a slab with a lathe's cylinder taken out.
    pObject = FirstObject( oScene, CsgOf( "difference", "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": [3, 0.4, 3] },"
                                                        "{ \"type\": \"lathe\", \"center\": [0, -1, 0], "
                                                        "\"profile\": [ [0, 0], [0.5, 0], [0.5, 2], [0, 2] ] }" ).c_str() );
    if ( pObject )
    {
        Check( Misses( *pObject, RayFrom( 0, 5, 0, 0, -1, 0 ) ), "a slab with a lathe taken out is open there" );
        Check( CsgHitsAt( *pObject, RayFrom( 1, 5, 0, 0, -1, 0 ), 4.8, 0, 1, 0 ), "the slab is whole beside the lathe's hole" );
    }

        // Saving keeps the profile as it was given.
    pObject = FirstObject( oScene, "{ \"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 0], [1, 0], "
                                   "{ \"quadratic\": [ [1.5, 0.5], [0.5, 1] ] }, [0, 1] ], \"sweep\": 180, \"capped\": true }" );
    if ( pObject )
    {
        std::string strSaved = oScene.ToJson();
        CScene      oAgain;

        Check( strSaved.find( "\"quadratic\"" ) != std::string::npos && strSaved.find( "\"sweep\"" ) != std::string::npos,
               "a lathe saves its curves and sweep as given" );
        Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "a lathe saved and loaded again is the same" );
    }

    ExpectError( Minimal( s_strSphereObject, "\"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [-1, 0], [1, 1] ]" ),
                 BAD_FILE, "can't be below 0" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 0], [1, 1] ], \"sweep\": 400" ),
                 BAD_FILE, "sweep must be more than 0" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"lathe\", \"center\": [0, 0, 0]" ), BAD_FILE, "profile" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ { \"quadratic\": [ [1, 0], [1, 1] ] } ]" ),
                 BAD_FILE, "must start with a point" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"lathe\", \"center\": [0, 0, 0], \"smooth\": true, "
                                             "\"profile\": [ [0, 0], { \"quadratic\": [ [1, 0], [1, 1] ] } ]" ),
                 BAD_FILE, "takes only points" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 0], [1] ]" ),
                 BAD_FILE, "profile[1]: a point must be [x, y]" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"lathe\", \"center\": [0, 0, 0], \"profile\": [ [0, 0], [0, 1] ]" ),
                 BAD_FILE, "lies on the axis" );
}

    // A mesh is hit at t = dT (its normals are blended, so only the place is
    // checked).
static bool MeshHitsAt( CObjects &oObject, CRay oRay, double dT )
{
    CVector oHit;

    return oObject.Intersect( oRay, oHit ) && ABS( oRay.GetT() - dT ) < 1e-9;
}

    // The value of a formula at a point, or NAN if it doesn't compile.
static double FormulaAt( const char *strFormula, double x, double y, double z )
{
    CExpression                   oExpression;
    std::map<std::string, double> aoParameters;
    std::string                   strError;

    aoParameters[ "k" ] = 2.0;
    if ( ! oExpression.Compile( strFormula, aoParameters, strError ) )
    {
        fprintf( stderr, "FAILED: \"%s\" doesn't compile: %s\n", strFormula, strError.c_str() );
        return NAN;
    }
    return oExpression.Value( x, y, z );
}

    // The error compiling a formula, or "" if it compiles.
static std::string FormulaError( const char *strFormula )
{
    CExpression                   oExpression;
    std::map<std::string, double> aoParameters;
    std::string                   strError;

    return oExpression.Compile( strFormula, aoParameters, strError ) ? std::string() : strError;
}

    // The Apollo cameras: their lenses, Sellmeier glasses, film, the Reseau
    // plate, camera bodies and the regolith material.
    // A little-endian glTF binary container: the JSON (padded with spaces)
    // and the binary chunk (padded with zeros).
static std::string MakeGlb( std::string strJson, std::string strBinary )
{
    while ( strJson.size() % 4 )
        strJson += ' ';
    while ( strBinary.size() % 4 )
        strBinary += '\0';

    std::string strOut;
    auto        Put32 = [ &strOut ]( uint32_t nValue ) { for ( int nByte = 0; nByte < 4; nByte++ ) strOut += (char)( ( nValue >> ( 8 * nByte ) ) & 0xFF ); };

    strOut = "glTF";
    Put32( 2 );
    Put32( (uint32_t)( 12 + 8 + strJson.size() + ( strBinary.empty() ? 0 : 8 + strBinary.size() ) ) );
    Put32( (uint32_t)strJson.size() );
    Put32( 0x4E4F534A );
    strOut += strJson;
    if ( ! strBinary.empty() )
    {
        Put32( (uint32_t)strBinary.size() );
        Put32( 0x004E4942 );
        strOut += strBinary;
    }
    return strOut;
}

static void PutFloat( std::string &strOut, float fValue )
{
    uint32_t nBits;

    memcpy( &nBits, &fValue, 4 );
    for ( int nByte = 0; nByte < 4; nByte++ )
        strOut += (char)( ( nBits >> ( 8 * nByte ) ) & 0xFF );
}

static std::string ToBase64( const std::string &strData )
{
    static const char *strDigits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string        strOut;

    for ( size_t nAt = 0; nAt < strData.size(); nAt += 3 )
    {
        uint32_t nBits = (uint32_t)(unsigned char)strData[ nAt ] << 16;
        size_t   nLeft = strData.size() - nAt;

        if ( nLeft > 1 )
            nBits |= (uint32_t)(unsigned char)strData[ nAt + 1 ] << 8;
        if ( nLeft > 2 )
            nBits |= (uint32_t)(unsigned char)strData[ nAt + 2 ];
        strOut += strDigits[ ( nBits >> 18 ) & 63 ];
        strOut += strDigits[ ( nBits >> 12 ) & 63 ];
        strOut += ( nLeft > 1 ) ? strDigits[ ( nBits >> 6 ) & 63 ] : '=';
        strOut += ( nLeft > 2 ) ? strDigits[ nBits & 63 ] : '=';
    }
    return strOut;
}

    // glTF 2.0 models, CMetalMaterial, the adaptive sampler's confidence and
    // channels, and CRayCountMap (2026-alpha-0.20.1).
    // The lens built as glass, with its ray fans (2026-alpha-0.21.2).
static void LensElementsChecks( void )
{
    const char *strObject = "{ \"type\": \"lensElements\", \"prescription\": \"double-gauss-50mm\", \"scale\": 1, \"rays\": 5 }";
    CScene      oScene;
    std::string strScene = Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }", strObject );

    Check( oScene.FromJson( strScene ) == 0, "a lensElements object loads" );

    CLensElements *pElements = dynamic_cast<CLensElements *>( oScene.GetObjectAt( 0 ) );
    CLens          oLens;
    std::string    strError;

    Check( pElements != NULL, "it is a CLensElements" );
    if ( pElements == NULL )
        return;
    oLens.SetPrescription( "double-gauss-50mm" );
    oLens.SetFocusDistance( 1e9 );
    Check( oLens.Prepare( 1e9, strError ) == TRUE, "the double Gauss prepares" );

        // Six runs of glass: two singlets, a doublet, a doublet and a singlet.
    Check( pElements->GetElementCount() == 6, "the double Gauss is built as its six elements" );
    Check( (int)pElements->GetParts().size() == 6 + 1 + pElements->GetRayCount(), "the parts are the glass, the stop and the rays" );
        // Two fields: on axis in three colors, then 0.7 of the field.
    Check( pElements->GetRayCount() == 4 * 5, "five rays in each of the four fans" );

        // Each ray meets every surface and ends on the sensor plane; the
        // on-axis green rays (the d line, the second fan) come to the axis there.
    const std::vector< std::vector<CVector> > &aaoPaths = pElements->GetRayPaths();
    bool                                       bEnds = true;
    bool                                       bFocus = true;
    bool                                       bCount = true;

    for ( size_t nRay = 0; nRay < aaoPaths.size(); nRay++ )
    {
        std::vector<CVector> aoPath = aaoPaths[ nRay ];

        if ( aoPath.size() != (size_t)oLens.GetSurfaceCount() + 2 )
            bCount = false;
        if ( aoPath.empty() )
            continue;

        CVector oEnd = aoPath.back();

            // In the lens frame (scale 1, no turn) z is along the axis and the
            // cutaway sets the rays just off the cut, at x = -2 x 0.25 mm.
        if ( ABS( oEnd.GetZ() - oLens.GetSensorZ() ) > 1e-6 || ABS( oEnd.GetX() + 0.5 ) > 1e-9 )
            bEnds = false;
        if ( nRay >= 5 && nRay < 10 && ABS( oEnd.GetY() ) > 0.15 )
            bFocus = false;
    }
    Check( bCount, "each ray has a point at every surface, its start and its end" );
    Check( bEnds, "each ray ends on the sensor plane, just off the cut face" );
    Check( bFocus, "the on-axis d-line rays meet the axis at the sensor, within 0.15 mm" );

        // A ray straight down the axis goes straight through.
    {
        CLensElements oStraight;
        std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse(
            "{ \"prescription\": \"planar-80mm\", \"scale\": 1, \"rays\": 3, \"fields\": [0.5] }" ) );

        Check( pNode && oStraight.FromJson( *pNode ) && oStraight.GetRayCount() == 3 && oStraight.GetElementCount() == 5,
               "the Planar 80 has five elements, and one fan of three rays off axis" );
    }

        // It hits as glass: a ray through the middle meets the front element.
    {
        CRay    oRay;
        CVector oHit;

            // 1 mm into the half the cutaway keeps (x >= 0); the front
            // surface (radius 29.475 mm) is there sag(1) behind its vertex.
        double dSag = 29.475 - sqrt( 29.475 * 29.475 - 1.0 );

        oRay.Set( 1.0, 0.0, -50.0, 0.0, 0.0, 1.0, 1e100, 1.0, -1, -1 );
        Check( pElements->Intersect( oRay, oHit ) && ABS( oHit.GetZ() - dSag ) < 1e-6, "a ray along the axis meets the front surface" );
        oRay.Set( -1.0, 0.0, -50.0, 0.0, 0.0, 1.0, 1e100, 1.0, -1, -1 );
        Check( ! pElements->Intersect( oRay, oHit ) || oHit.GetZ() > 40.0, "the cut-away half lets a ray past the glass" );
    }

        // It saves its settings, not its parts, and reads them back the same.
    std::string strSaved = oScene.ToJson();
    CScene      oAgain;

    Check( strSaved.find( "\"lensElements\"" ) != std::string::npos && strSaved.find( "\"csg\"" ) == std::string::npos,
           "a lensElements object saves its settings, not its parts" );
    Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "a lensElements object round-trips" );

    ExpectError( Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                          "{ \"type\": \"lensElements\", \"prescription\": \"no-such-lens\" }" ), BAD_FILE, "has no built-in lens" );
    ExpectError( Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                          "{ \"type\": \"lensElements\", \"prescription\": \"biogon-38mm\", \"scale\": 0 }" ), BAD_FILE, "scale must be more than 0" );
    ExpectError( Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                          "{ \"type\": \"lensElements\", \"prescription\": \"biogon-38mm\", \"fanOpacity\": 0 }" ), BAD_FILE,
                 "fanOpacity must be more than 0 and at most 1" );
    ExpectError( Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                          "{ \"type\": \"lensElements\", \"prescription\": \"biogon-38mm\", \"fanOpacity\": 1.5 }" ), BAD_FILE,
                 "fanOpacity must be more than 0 and at most 1" );
    ExpectError( Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                          "{ \"type\": \"lensElements\", \"prescription\": \"biogon-38mm\", \"fields\": [1.5] }" ), BAD_FILE, "must be from 0 to 1" );
    ExpectError( Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                          "{ \"type\": \"lensElements\", \"prescription\": \"biogon-38mm\", \"rays\": 0 }" ), BAD_FILE, "rays" );
    ExpectError( Minimal( "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }",
                          "{ \"type\": \"lensElements\" }" ), BAD_FILE, "prescription" );
}

static void ShowcaseChecks( void )
{
    std::string strError;

    {
            // One triangle, its corners as floats 16 bytes apart (a stride
            // with padding), indices as unsigned shorts, under a parent node
            // that moves it by (10, 0, 0) and a child that turns it 90
            // degrees about y; its material's base color is (0.2, 0.4, 0.6).
        std::string strBinary;
        const float afCorners[ 3 ][ 3 ] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };

        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            for ( int nAxis = 0; nAxis < 3; nAxis++ )
                PutFloat( strBinary, afCorners[ nCorner ][ nAxis ] );
            PutFloat( strBinary, 99.0f );           // padding the stride skips
        }

        const unsigned short anIndices[] = { 0, 1, 2 };

        for ( unsigned short nIndex : anIndices )
        {
            strBinary += (char)( nIndex & 0xFF );
            strBinary += (char)( nIndex >> 8 );
        }

        const double dHalf = sqrt( 0.5 );
        char         achJson[ 2048 ];

        snprintf( achJson, sizeof( achJson ),
                  "{\"asset\":{\"version\":\"2.0\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],"
                  "\"nodes\":[{\"translation\":[10,0,0],\"children\":[1]},{\"rotation\":[0,%.17g,0,%.17g],\"mesh\":0}],"
                  "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1,\"material\":0}]}],"
                  "\"materials\":[{\"name\":\"blue\",\"pbrMetallicRoughness\":{\"baseColorFactor\":[0.2,0.4,0.6,1]}}],"
                  "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                  "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
                  "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":48,\"byteStride\":16},"
                  "{\"buffer\":0,\"byteOffset\":48,\"byteLength\":6}],"
                  "\"buffers\":[{\"byteLength\":54}]}", dHalf, dHalf );

        std::string       strGlb = MakeGlb( achJson, strBinary );
        ModelFile::SModel oModel;

        Check( ModelFile::IsGltf( strGlb, "anything.dat" ) == TRUE, "a .glb is recognized by its magic, whatever its name" );
        Check( ModelFile::Read( strGlb, "tri.glb", oModel, strError ) == TRUE, "a .glb reads" );
            // The child turns (1, 0, 0) to (0, 0, -1), the parent moves it to
            // (10, 0, -1), and z is negated for Ray's left-handed scenes.
        Check( oModel.aoVertices.size() == 3 && oModel.anFaces.size() == 3 &&
               ABS( oModel.aoVertices[ 1 ].GetX() - 10.0 ) < 1e-6 && ABS( oModel.aoVertices[ 1 ].GetY() ) < 1e-6 &&
               ABS( oModel.aoVertices[ 1 ].GetZ() - 1.0 ) < 1e-6 && ABS( oModel.aoVertices[ 2 ].GetY() - 1.0 ) < 1e-6,
               "a glTF node's translation and rotation are flattened in, the stride is followed, and z is negated" );
        Check( oModel.anFaces.size() == 3 && oModel.anFaces[ 0 ] == 0 && oModel.anFaces[ 1 ] == 2 && oModel.anFaces[ 2 ] == 1,
               "a glTF face is turned back round when z is negated, so it isn't mirrored" );
        Check( oModel.aoFaceColors.size() == 1 && oModel.abHasColor.size() == 1 && oModel.abHasColor[ 0 ] &&
               ABS( oModel.aoFaceColors[ 0 ].GetB() - 0.6 ) < 1e-6 && ABS( oModel.aoFaceColors[ 0 ].GetR() - 0.2 ) < 1e-6,
               "a glTF material's baseColorFactor becomes its faces' color" );

            // The same as a .gltf whose buffer is a separate file, and as a
            // data: URI; a node by name; a material filter.
        std::string strText( achJson );
        std::string strExternal = strText;

        strExternal.replace( strExternal.find( "{\"byteLength\":54}" ), 17, "{\"byteLength\":54,\"uri\":\"tri.bin\"}" );

        ModelFile::SGltfFiles oFiles;
        std::vector<std::string> astrUris;

        oFiles.aoFiles.push_back( std::make_pair( std::string( "tri.bin" ), std::make_shared<const std::string>( strBinary ) ) );
        Check( ModelFile::GltfUris( strExternal, astrUris, strError ) == TRUE && astrUris.size() == 1 && astrUris[ 0 ] == "tri.bin",
               "a .gltf's other files are listed" );
        Check( ModelFile::ReadGltf( strExternal, &oFiles, "", "", oModel, strError ) == TRUE && oModel.aoVertices.size() == 3,
               "a .gltf reads its buffer from a file beside it" );
        Check( ModelFile::ReadGltf( strExternal, NULL, "", "", oModel, strError ) == FALSE && strError.find( "tri.bin" ) != std::string::npos,
               "a .gltf whose buffer is missing is refused, naming it" );

        std::string strData = strText;

        strData.replace( strData.find( "{\"byteLength\":54}" ), 17,
                         "{\"byteLength\":54,\"uri\":\"data:application/octet-stream;base64," + ToBase64( strBinary ) + "\"}" );
        Check( ModelFile::ReadGltf( strData, NULL, "", "", oModel, strError ) == TRUE && oModel.aoVertices.size() == 3 &&
               ABS( oModel.aoVertices[ 1 ].GetX() - 10.0 ) < 1e-6, "a .gltf with its buffer in a data: URI reads" );

        std::string strNamed = strData;

        strNamed.replace( strNamed.find( "{\"rotation\"" ), 1, "{\"name\":\"tri\"," );
        Check( ModelFile::ReadGltf( strNamed, NULL, "tri", "", oModel, strError ) == TRUE && oModel.aoVertices.size() == 3 &&
               ABS( oModel.aoVertices[ 1 ].GetX() - 1.0 ) < 1e-6 && ABS( oModel.aoVertices[ 1 ].GetZ() ) < 1e-6,
               "a named glTF node is read in its own frame (its own and its parents' placement left out)" );
        Check( ModelFile::ReadGltf( strNamed, NULL, "nothing", "", oModel, strError ) == FALSE,
               "a glTF node that isn't there is an error" );
        Check( ModelFile::ReadGltf( strData, NULL, "", "blue", oModel, strError ) == TRUE &&
               ModelFile::ReadGltf( strData, NULL, "", "red", oModel, strError ) == FALSE,
               "a glTF material filter keeps only that material's triangles" );

            // A sparse accessor moves corner 2 to (0, 2, 0).
        std::string strSparseBinary = strBinary;

        while ( strSparseBinary.size() % 4 )
            strSparseBinary += '\0';

        size_t nSparseAt = strSparseBinary.size();

        strSparseBinary += (char)2;                 // index 2, an unsigned byte
        strSparseBinary += std::string( 3, '\0' );
        PutFloat( strSparseBinary, 0.0f );
        PutFloat( strSparseBinary, 2.0f );
        PutFloat( strSparseBinary, 0.0f );

        std::string strSparse = strText;
        char        achViews[ 256 ];

        strSparse.replace( strSparse.find( "\"type\":\"VEC3\"}" ), 14,
                           "\"type\":\"VEC3\",\"sparse\":{\"count\":1,\"indices\":{\"bufferView\":2,\"componentType\":5121},"
                           "\"values\":{\"bufferView\":3}}}" );
        snprintf( achViews, sizeof( achViews ), ",{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":1},{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":12}]",
                  (int)nSparseAt, (int)( nSparseAt + 4 ) );
        strSparse.replace( strSparse.find( "\"byteLength\":6}]" ) + 15, 1, achViews );
        strSparse.replace( strSparse.find( "{\"byteLength\":54}" ), 17, "{\"byteLength\":" + std::to_string( strSparseBinary.size() ) + "}" );
        Check( ModelFile::Read( MakeGlb( strSparse, strSparseBinary ), "sparse.glb", oModel, strError ) == TRUE &&
               oModel.aoVertices.size() == 3 && ABS( oModel.aoVertices[ 2 ].GetY() - 2.0 ) < 1e-6,
               "a glTF sparse accessor replaces the elements it lists" );

            // Refusals.
        std::string strMeshopt = strText;

        strMeshopt.replace( strMeshopt.find( "\"scene\":0" ), 9, "\"extensionsRequired\":[\"EXT_meshopt_compression\"],\"scene\":0" );
        Check( ModelFile::Read( MakeGlb( strMeshopt, strBinary ), "m.glb", oModel, strError ) == FALSE &&
               strError.find( "EXT_meshopt_compression" ) != std::string::npos,
               "a meshopt-compressed glTF is refused, naming the extension" );

            // Draco is decoded now: requiring it is no reason to refuse a
            // file whose primitives don't use it.
        std::string strDraco = strText;

        strDraco.replace( strDraco.find( "\"scene\":0" ), 9, "\"extensionsRequired\":[\"KHR_draco_mesh_compression\"],\"scene\":0" );
        Check( ModelFile::Read( MakeGlb( strDraco, strBinary ), "d.glb", oModel, strError ) == TRUE,
               "a glTF that requires KHR_draco_mesh_compression is read" );

        std::string strVersion1 = strGlb;

        strVersion1[ 4 ] = 1;
        Check( ModelFile::Read( strVersion1, "v1.glb", oModel, strError ) == FALSE, "a glTF 1.0 binary is refused" );
        Check( ModelFile::Read( strGlb.substr( 0, 40 ), "short.glb", oModel, strError ) == FALSE, "a cut-short .glb is refused" );

        std::string strBadIndex = strBinary;

        strBadIndex[ 48 ] = 9;                      // index 0 becomes 9, past the 3 corners
        Check( ModelFile::Read( MakeGlb( achJson, strBadIndex ), "bad.glb", oModel, strError ) == FALSE,
               "a glTF index past its corners is refused" );
        Check( ModelFile::GltfUris( "{\"asset\":{\"version\":\"2.0\"},\"buffers\":[{\"uri\":\"../secret.bin\",\"byteLength\":1}]}",
                                    astrUris, strError ) == FALSE, "a .gltf naming a file outside its folder is refused" );
    }

    {
            // Metals: the Fresnel reflectance of a conductor.
        FLT anN[ 3 ], anK[ 3 ];

        Check( CMetalMaterial::Preset( "gold", anN, anK ) == TRUE && CMetalMaterial::Preset( "unobtainium", anN, anK ) == FALSE,
               "metal presets are found by name" );
        CMetalMaterial::Preset( "gold", anN, anK );

        double dNormal = ( ( anN[ 0 ] - 1.0 ) * ( anN[ 0 ] - 1.0 ) + anK[ 0 ] * anK[ 0 ] ) /
                         ( ( anN[ 0 ] + 1.0 ) * ( anN[ 0 ] + 1.0 ) + anK[ 0 ] * anK[ 0 ] );

        Check( ABS( CMetalMaterial::Reflectance( 1.0, anN[ 0 ], anK[ 0 ] ) - dNormal ) < 1e-9,
               "a metal's reflectance face on is ((n-1)^2 + k^2) / ((n+1)^2 + k^2)" );
        Check( CMetalMaterial::Reflectance( 1.0, anN[ 0 ], anK[ 0 ] ) > CMetalMaterial::Reflectance( 1.0, anN[ 2 ], anK[ 2 ] ) + 0.3,
               "gold reflects red far more than blue" );
        Check( CMetalMaterial::Reflectance( 0.01, anN[ 2 ], anK[ 2 ] ) > 0.9 && CMetalMaterial::Reflectance( 0.0, anN[ 2 ], anK[ 2 ] ) == 1.0,
               "a metal whitens toward grazing" );
            // A dielectric (k 0) agrees with the Schlick-free Fresnel value face on.
        Check( ABS( CMetalMaterial::Reflectance( 1.0, 1.5, 0.0 ) - 0.04 ) < 1e-9, "with k 0 it is the dielectric's 4%" );

        CScene oScene;

        Check( oScene.FromJson( "{\"format\":\"Ray scene\",\"version\":3,\"camera\":{\"position\":[0,0,-5],\"lookAt\":[0,0,0]},\"objects\":[{\"type\":\"sphere\",\"center\":[0,0,0],\"radius\":1,"
                                "\"materials\":[{\"type\":\"metal\",\"metal\":\"copper\",\"roughness\":0.2}]}]}" ) == 0,
               "a metal material loads" );
        Check( oScene.FromJson( "{\"format\":\"Ray scene\",\"version\":3,\"camera\":{\"position\":[0,0,-5],\"lookAt\":[0,0,0]},\"objects\":[{\"type\":\"sphere\",\"center\":[0,0,0],\"radius\":1,"
                                "\"materials\":[{\"type\":\"metal\",\"metal\":\"brass\"}]}]}" ) != 0 &&
               oScene.FromJson( "{\"format\":\"Ray scene\",\"version\":3,\"camera\":{\"position\":[0,0,-5],\"lookAt\":[0,0,0]},\"objects\":[{\"type\":\"sphere\",\"center\":[0,0,0],\"radius\":1,"
                                "\"materials\":[{\"type\":\"metal\",\"n\":[1,1,1],\"k\":[1,-1,1]}]}]}" ) != 0,
               "an unknown metal and a negative k are refused" );
    }

    {
            // Student's t, against its tables (two-sided 95% and 99%).
        Check( ABS( CAdaptiveSampler::StudentT( 0.95, 1 ) - 12.706 ) < 0.01 && ABS( CAdaptiveSampler::StudentT( 0.95, 2 ) - 4.303 ) < 0.005,
               "Student's t at 1 and 2 degrees of freedom is exact" );
        Check( ABS( CAdaptiveSampler::StudentT( 0.95, 3 ) - 3.182 ) < 0.02 * 3.182 && ABS( CAdaptiveSampler::StudentT( 0.95, 8 ) - 2.306 ) < 0.01 * 2.306 &&
               ABS( CAdaptiveSampler::StudentT( 0.95, 30 ) - 2.042 ) < 0.002 * 2.042 && ABS( CAdaptiveSampler::StudentT( 0.99, 10 ) - 3.169 ) < 0.01 * 3.169,
               "Student's t from 3 degrees of freedom up is within 2% of its table" );

        CScene oScene;
        auto   Sampler = []( const char *strSettings ) -> std::string
        {
            return std::string( "{\"format\":\"Ray scene\",\"version\":3,\"camera\":{\"position\":[0,0,-5],\"lookAt\":[0,0,0],"
                                "\"sampler\":{\"type\":\"adaptive\"" ) + strSettings + "}}}";
        };

        Check( oScene.FromJson( Sampler( ",\"min\":4,\"confidence\":0.95,\"channels\":\"rgb\"" ) ) == 0,
               "the adaptive sampler takes a confidence and per-channel errors" );

        IASLib::CObjectNode oSaved;

        oScene.GetCamera().GetSampler().ToJson( oSaved );
        Check( oSaved.get( "confidence" ) && ABS( oSaved.get( "confidence" )->doubleValue() - 0.95 ) < 1e-12 &&
               oSaved.get( "channels" ) && oSaved.get( "channels" )->textString() == "rgb",
               "the adaptive sampler saves its confidence and channels" );
        Check( oScene.FromJson( Sampler( "" ) ) == 0, "a plain adaptive sampler still loads" );

        IASLib::CObjectNode oPlain;

        oScene.GetCamera().GetSampler().ToJson( oPlain );
        Check( oPlain.get( "confidence" ) == NULL && oPlain.get( "channels" ) == NULL,
               "a plain adaptive sampler saves as it always did" );
        Check( oScene.FromJson( Sampler( ",\"confidence\":0.3" ) ) != 0 && oScene.FromJson( Sampler( ",\"channels\":\"hsv\"" ) ) != 0,
               "a confidence under 0.5 and unknown channels are refused" );
    }

    {
            // The ray count map.
        CRayCountMap::Enable( 4, 3 );
        CRayCountMap::Record( 2, 1, 17 );
        CRayCountMap::Record( 9, 9, 5 );        // outside: ignored
        Check( CRayCountMap::IsEnabled() && CRayCountMap::GetCounts().size() == 12 && CRayCountMap::GetCounts()[ 6 ] == 17,
               "the ray count map records a pixel's rays" );
        CRayCountMap::Disable();
        CRayCountMap::Record( 2, 1, 3 );
        Check( ! CRayCountMap::IsEnabled() && CRayCountMap::GetCounts().empty(), "the ray count map records nothing when off" );
    }
}

static void ApolloChecks( void )
{
    CScene  oScene;

        // Each lens: its focal length, widest f-number and a sensor behind
        // it, focused at infinity.
    struct SExpected { const char *strName; double dFocal; double dFNumber; };
    static const SExpected s_aoLenses[] =
    {
        { "biogon-38mm", 38.0, 4.5 }, { "biogon-60mm", 61.1, 5.6 }, { "biogon-60mm-reseau", 61.1, 5.6 },
        { "planar-80mm", 80.0, 2.8 }, { "planar-100mm", 100.0, 3.5 }, { "uv-sonnar-105mm", 105.0, 4.3 },
        { "sonnar-250mm", 250.0, 5.6 }, { "tele-tessar-500mm", 500.0, 8.0 },
    };
    for ( size_t nIndex = 0; nIndex < sizeof( s_aoLenses ) / sizeof( s_aoLenses[ 0 ] ); nIndex++ )
    {
        std::string strLens = std::string( "{ \"prescription\": \"" ) + s_aoLenses[ nIndex ].strName +
                              "\", \"focusDistance\": 1e9, \"sensorWidth\": 55 }";
        BOOL        bLoaded = ( oScene.FromJson( WithLens( strLens.c_str() ) ) == 0 ) ? TRUE : FALSE;
        CLens      *pLens = oScene.GetCamera().GetLens();
        std::string strWhat = std::string( s_aoLenses[ nIndex ].strName ) + " loads and has its focal length, f-number and back focus";

        Check( bLoaded && pLens && ABS( pLens->GetFocalLength() - s_aoLenses[ nIndex ].dFocal ) < 0.001 * s_aoLenses[ nIndex ].dFocal &&
               ABS( pLens->GetWideOpen() - s_aoLenses[ nIndex ].dFNumber ) < 0.03 * s_aoLenses[ nIndex ].dFNumber &&
               pLens->GetFilmDistance() > -1e-6, strWhat.c_str() );
    }
        // The Reseau plate's back face lies on the film, focused at infinity.
    Check( oScene.FromJson( WithLens( "{ \"prescription\": \"biogon-60mm-reseau\", \"focusDistance\": 1e9, \"sensorWidth\": 53 }" ) ) == 0 &&
           oScene.GetCamera().GetLens() && ABS( oScene.GetCamera().GetLens()->GetFilmDistance() ) < 0.05,
           "the Reseau plate lies against the film" );

        // Sellmeier glasses, against their catalog indices and Abbe numbers.
    double dSilicaD = CLens::SellmeierIndex( CLens::GLASS_FUSED_SILICA, 0.58756 );
    double dSilicaV = ( dSilicaD - 1.0 ) / ( CLens::SellmeierIndex( CLens::GLASS_FUSED_SILICA, 0.48613 ) -
                                             CLens::SellmeierIndex( CLens::GLASS_FUSED_SILICA, 0.65627 ) );
    double dFluorD = CLens::SellmeierIndex( CLens::GLASS_CALCIUM_FLUORIDE, 0.58756 );
    double dFluorV = ( dFluorD - 1.0 ) / ( CLens::SellmeierIndex( CLens::GLASS_CALCIUM_FLUORIDE, 0.48613 ) -
                                           CLens::SellmeierIndex( CLens::GLASS_CALCIUM_FLUORIDE, 0.65627 ) );

    Check( ABS( dSilicaD - 1.4585 ) < 2e-4 && ABS( dSilicaV - 67.8 ) < 0.5, "fused silica: n_d 1.4585, V 67.8" );
    Check( ABS( dFluorD - 1.4338 ) < 2e-4 && ABS( dFluorV - 95.0 ) < 0.6, "calcium fluoride: n_d 1.4338, V 95" );
    Check( CLens::SellmeierIndex( CLens::GLASS_FUSED_SILICA, 0.25 ) > 1.50 &&
           CLens::SellmeierIndex( CLens::GLASS_CALCIUM_FLUORIDE, 0.25 ) > 1.46, "both bend ultraviolet more" );
    Check( oScene.FromJson( WithLens( "{ \"surfaces\": [ { \"radius\": 60, \"thickness\": 5, \"glass\": \"calcium-fluoride\", \"aperture\": 10 },"
                                      " { \"radius\": -60, \"thickness\": 0, \"aperture\": 10 } ], \"sensorWidth\": 36 }" ) ) == 0 &&
           oScene.ToJson().find( "\"glass\": \"calcium-fluoride\"" ) != std::string::npos &&
           oScene.GetCamera().GetChannels() == 3, "a surface can name a Sellmeier glass, saves it, and disperses" );
    Check( oScene.FromJson( WithLens( "{ \"surfaces\": [ { \"radius\": 60, \"thickness\": 5, \"glass\": \"calcium-fluoride\", \"ior\": 1.5, \"aperture\": 10 },"
                                      " { \"radius\": -60, \"thickness\": 0, \"aperture\": 10 } ] }" ) ) != 0 &&
           oScene.FromJson( WithLens( "{ \"surfaces\": [ { \"radius\": 60, \"thickness\": 5, \"glass\": \"quartz\", \"aperture\": 10 },"
                                      " { \"radius\": -60, \"thickness\": 0, \"aperture\": 10 } ] }" ) ) != 0,
           "a glass and an ior, or an unknown glass, are refused" );

        // Film: the curve falls through the mid density at mid gray, as
        // steep there as the gamma, and the clear base shows white.
    CFilmColorModel oFilm;
    const CFilmColorModel::SStock &oStock = oFilm.GetStock();
    double dMid = oFilm.Density( 1, 0.18 );
    double dSlope = ( oFilm.Density( 1, 0.18 * 1.001 ) - oFilm.Density( 1, 0.18 / 1.001 ) ) / ( 2.0 * log10( 1.001 ) );

    Check( ABS( dMid - oStock.nMidDensity ) < 1e-9, "mid gray develops to the stock's mid density" );
    Check( oFilm.Density( 1, 1e-6 ) > oStock.nMaxDensity - 0.05 && oFilm.Density( 1, 1e6 ) < oStock.nMinDensity + 0.05 &&
           oFilm.Density( 1, 0.1 ) > oFilm.Density( 1, 0.2 ), "the curve falls from the maximum density to the base" );
    Check( dSlope < -0.6 * oStock.anGamma[ 1 ] && dSlope > -1.01 * oStock.anGamma[ 1 ], "the curve is about as steep as the gamma at mid gray" );
    Check( ABS( oFilm.Transmission( oStock.nMinDensity ) - 1.0 ) < 1e-12, "the clear base shows white" );
    {
            // Sunny 16: ISO 100, 1/100 s, f/16 puts 0.18 on mid gray.
        std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( "{ \"type\": \"film\", \"stock\": \"panatomic-x\", \"iso\": 100, \"shutter\": 0.01, \"fStop\": 16 }" ) );
        CFilmColorModel    oPan;
        BYTE               nRed, nGreen, nBlue;

        Check( pNode && oPan.FromJson( *pNode ), "a film's settings read" );
        oPan.ToRGB( CColor( 0.18, 0.18, 0.18 ), nRed, nGreen, nBlue );
        BYTE nExpected = (BYTE)( CSrgbColorModel::Encode( oPan.Transmission( oPan.GetStock().nMidDensity ) ) * 255.0 + 0.5 );
        Check( nRed == nGreen && nGreen == nBlue && ABS( (int)nRed - (int)nExpected ) <= 1, "sunny 16 puts mid gray on the curve's mid density" );

    }
    {
        std::vector<FLT> anKernel = CFilmColorModel::Kernel( 2.5 );
        double           dSum = anKernel[ 0 ];

        for ( size_t nIndex = 1; nIndex < anKernel.size(); nIndex++ )
            dSum += 2.0 * anKernel[ nIndex ];
        Check( ABS( dSum - 1.0 ) < 1e-12, "the spread's kernel sums to one" );

            // One bright pixel in the middle: the spread keeps its light.
        std::vector<FLT> anLayer( 61 * 61, 0.0 );
        anLayer[ 30 * 61 + 30 ] = 100.0;
        CFilmColorModel::Spread( anLayer, 61, 61, 0.1, 1.0, 0.05, 4.0 );
        double dTotal = 0.0;
        for ( size_t nIndex = 0; nIndex < anLayer.size(); nIndex++ )
            dTotal += anLayer[ nIndex ];
        Check( ABS( dTotal - 100.0 ) < 1e-6 && anLayer[ 30 * 61 + 30 ] > 85.0 && anLayer[ 30 * 61 + 30 ] < 90.0 && anLayer[ 30 * 61 + 34 ] > 0.0,
               "the spread keeps the light, leaves most where it fell and carries some away" );
    }
    {
            // Finishing an image twice gives the same bytes (grain is the
            // same every time), and a dark pixel next to a bright one gets
            // brighter.
        CImageFile  oFirst( 32, 32 );
        CImageFile  oSecond( 32, 32 );

        oFirst.KeepLinear();
        oSecond.KeepLinear();
        for ( int nY = 0; nY < 32; nY++ )
        {
            for ( int nX = 0; nX < 32; nX++ )
            {
                CColor oColor = ( nX == 16 && nY == 16 ) ? CColor( 500.0, 500.0, 500.0 ) : CColor( 0.01, 0.01, 0.01 );

                oFirst.PutLinear( nX, nY, oColor );
                oSecond.PutLinear( nX, nY, oColor );
            }
        }
        oFilm.SetWidth( 3.0 );      // a 3 mm piece of film, so the spread covers pixels
        oFilm.Finish( oFirst );
        oFilm.Finish( oSecond );
        Check( oFirst.GetMemory() == oSecond.GetMemory(), "film grain is the same every time" );
        CColor oNear = oFirst.GetPoint( 18, 16 );
        CColor oFar = oFirst.GetPoint( 2, 2 );
        Check( oNear.GetR() > oFar.GetR() + 0.05 && oNear.GetR() > oNear.GetB(), "halation lights the film around a bright point, reddest" );
    }
    Check( oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"colorModel\": { \"type\": \"film\", \"stock\": \"kodachrome\" }" ) ) != 0 &&
           oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"colorModel\": { \"type\": \"film\", \"fStop\": 0 }" ) ) != 0 &&
           oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"colorModel\": { \"type\": \"film\", \"spread\": 10 }" ) ) != 0,
           "an unknown stock, a zero stop and too much spread are refused" );

        // The Data Camera: its body brings the Reseau lens, the format,
        // film and the crosses; a lens without a prescription keeps it.
    Check( oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]",
                                     "\"lookAt\": [0, 0, 0], \"body\": \"hasselblad-hdc\", \"lens\": { \"millimetersPerUnit\": 1000, \"focusDistance\": 5 }" ) ) == 0,
           "a camera body loads" );
    {
        CCamera    &oCamera = oScene.GetCamera();
        std::string strSaved = oScene.ToJson();
        CScene      oReread;

        Check( oCamera.GetLens() && oCamera.GetLens()->GetSensorWidth() == 53.0 &&
               strcmp( oCamera.GetColorModel().GetName(), "film" ) == 0 && oCamera.GetReseau().bOn,
               "the Data Camera has its 53 mm format, film and Reseau plate" );
        Check( strSaved.find( "biogon-60mm-reseau" ) != std::string::npos && strSaved.find( "\"body\"" ) == std::string::npos &&
               oReread.FromJson( strSaved ) == 0 && oReread.ToJson() == strSaved, "a body saves as the camera it makes, and reads back" );
            // The crosses: 5 by 5, 10 mm apart, 1.2 mm across, lines 0.05 mm.
        Check( oCamera.OnReseau( 0.0, 0.0 ) && oCamera.OnReseau( 10.0, 0.0 ) && oCamera.OnReseau( -20.0, 20.0 ) &&
               oCamera.OnReseau( 0.55, 0.0 ) && oCamera.OnReseau( 20.0, -19.45 ), "the Reseau crosses are where they should be" );
        Check( ! oCamera.OnReseau( 5.0, 5.0 ) && ! oCamera.OnReseau( 0.65, 0.0 ) && ! oCamera.OnReseau( 0.3, 0.03 ) &&
               ! oCamera.OnReseau( 30.0, 0.0 ), "and nowhere else" );
    }
    Check( oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"body\": \"leica\"" ) ) != 0 &&
           oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]", "\"lookAt\": [0, 0, 0], \"reseau\": { }" ) ) != 0 &&
           oScene.FromJson( Minimal( "\"lookAt\": [0, 0, 0]",
                                     "\"lookAt\": [0, 0, 0], \"body\": \"hasselblad-hdc\", \"reseau\": { \"lineWidth\": 2 }" ) ) != 0,
           "an unknown body, a Reseau without a lens and crosses thicker than they are wide are refused" );

        // Regolith: exactly its color seen along the light at normal
        // incidence, the surge falling away from opposition, no limb
        // darkening.
    {
        std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( "{ \"type\": \"regolith\", \"surge\": 1.0, \"width\": 0.06 }" ) );
        CRegolithMaterial  oSoil;

        Check( pNode && oSoil.FromJson( *pNode ), "a regolith material reads" );
        Check( ABS( oSoil.Factor( 1.0, 1.0, 0.0 ) - 1.0 ) < 1e-12, "regolith seen along the light is its color" );
        double dSide = oSoil.Factor( (FLT)cos( PI / 4.0 ), (FLT)cos( PI / 4.0 ), PI / 2.0 );
        Check( ABS( dSide - 0.5 * ( 1.0 + 1.0 / ( 1.0 + 1.0 / 0.06 ) ) ) < 1e-9, "the opposition surge fades away from opposition" );
        Check( ABS( oSoil.Factor( 0.5, 0.5, 0.0 ) - 1.0 ) < 1e-12 && ABS( oSoil.Factor( 1.0, 0.0, 0.0 ) - 2.0 ) < 1e-12,
               "Lommel-Seeliger: no limb darkening, brightest seen edge-on" );

        pNode.reset( RayJson::Parse( "{ \"type\": \"regolith\", \"width\": 0 }" ) );
        CRegolithMaterial oBad;
        Check( pNode && ! oBad.FromJson( *pNode ), "a zero surge width is refused" );

    }
}

static void MathShapeChecks( void )
{
    const double dPi = 3.14159265358979323846;
    CScene       oScene;
    CObjects    *pObject;
    CVector      oHit;

        // The formula language: precedence, power to the right, unary minus
        // under power, functions and parameters.
    Check( FormulaAt( "1 + 2 * 3", 0, 0, 0 ) == 7.0, "* binds tighter than +" );
    Check( FormulaAt( "(1 + 2) * 3", 0, 0, 0 ) == 9.0, "parentheses group" );
    Check( FormulaAt( "2^3^2", 0, 0, 0 ) == 512.0, "^ groups from the right" );
    Check( FormulaAt( "-2^2", 0, 0, 0 ) == -4.0, "-x^2 is -(x^2)" );
    Check( FormulaAt( "2^-1", 0, 0, 0 ) == 0.5, "an exponent can be negative" );
    Check( FormulaAt( "10 - 4 - 3", 0, 0, 0 ) == 3.0 && FormulaAt( "24 / 4 / 2", 0, 0, 0 ) == 3.0, "- and / group from the left" );
    Check( FormulaAt( "x*100 + y*10 + z", 1, 2, 3 ) == 123.0, "x, y and z are the point" );
    Check( FormulaAt( "k * x", 3, 0, 0 ) == 6.0, "a parameter stands for its number" );
    Check( ABS( FormulaAt( "sin(pi/2) + cos(0) + tan(0)", 0, 0, 0 ) - 2.0 ) < 1e-15, "sin, cos, tan and pi" );
    Check( ABS( FormulaAt( "atan2(1, 1)", 0, 0, 0 ) - ( dPi / 4 ) ) < 1e-15 &&
           ABS( FormulaAt( "asin(1) + acos(1) + atan(0)", 0, 0, 0 ) - ( dPi / 2 ) ) < 1e-15, "the inverse trigonometric functions" );
    Check( FormulaAt( "sqrt(16) + abs(-3) + floor(-1.5)", 0, 0, 0 ) == 5.0, "sqrt, abs and floor" );
    Check( ABS( FormulaAt( "exp(log(3)) + pow(2, 10) + e - e", 0, 0, 0 ) - 1027.0 ) < 1e-12, "exp, log, pow and e" );
    Check( FormulaAt( "min(2, 3) + max(2, 3)", 0, 0, 0 ) == 5.0 && FormulaAt( "clamp(5, 0, 1) + clamp(-5, 0, 1)", 0, 0, 0 ) == 1.0,
           "min, max and clamp" );
    Check( FormulaAt( "mix(2, 4, 0.25)", 0, 0, 0 ) == 2.5 && FormulaAt( "smoothstep(0, 1, 0.5)", 0, 0, 0 ) == 0.5 &&
           FormulaAt( "smoothstep(0, 1, 2)", 0, 0, 0 ) == 1.0, "mix and smoothstep" );

        // Exact gradients.
    {
        CExpression                   oExpression;
        std::map<std::string, double> aoParameters;
        std::string                   strError;
        double                        anGradient[ 3 ];

        Check( oExpression.Compile( "x^2 + 3*y*z", aoParameters, strError ) &&
               oExpression.Gradient( 1, 2, 3, anGradient ) == 19.0 &&
               anGradient[ 0 ] == 2.0 && anGradient[ 1 ] == 9.0 && anGradient[ 2 ] == 6.0, "a polynomial's gradient is exact" );
        Check( oExpression.Compile( "sin(x)*cos(y) + exp(z)/2 + sqrt(x*x + 1)", aoParameters, strError ), "a formula of functions compiles" );
        oExpression.Gradient( 0.3, 0.4, 0.5, anGradient );
        Check( ABS( anGradient[ 0 ] - ( ( cos( 0.3 ) * cos( 0.4 ) ) + ( 0.3 / sqrt( 1.09 ) ) ) ) < 1e-14 &&
               ABS( anGradient[ 1 ] + ( sin( 0.3 ) * sin( 0.4 ) ) ) < 1e-14 && ABS( anGradient[ 2 ] - ( exp( 0.5 ) / 2 ) ) < 1e-14,
               "the gradient carries through the functions exactly" );
        Check( oExpression.Compile( "pow(x, y) + atan2(y, x)", aoParameters, strError ), "pow and atan2 compile" );
        oExpression.Gradient( 2.0, 3.0, 0.0, anGradient );
        Check( ABS( anGradient[ 0 ] - ( 12.0 - ( 3.0 / 13.0 ) ) ) < 1e-12 && ABS( anGradient[ 1 ] - ( ( 8.0 * log( 2.0 ) ) + ( 2.0 / 13.0 ) ) ) < 1e-12,
               "a power with a variable exponent, and atan2, have exact gradients" );
    }

        // Errors, with the column.
    Check( FormulaError( "x +" ) == "column 4: the formula ends too soon", "a formula that ends too soon is reported at its end" );
    Check( FormulaError( "foo(x) + 1" ) == "column 1: unknown name \"foo\"", "an unknown name is reported" );
    Check( FormulaError( "1 + sin x" ) == "column 5: sin needs its arguments in parentheses", "a function needs parentheses" );
    Check( FormulaError( "(x + 1" ) == "column 1: this \"(\" isn't closed", "an unclosed parenthesis is reported where it opens" );
    Check( FormulaError( "1 $ 2" ) == "column 3: unexpected \"$\"", "a stray character is reported" );
    Check( FormulaError( "min(x)" ) == "column 1: min takes 2 arguments", "a function's argument count is checked" );
    Check( FormulaError( "x y" ) == "column 3: unexpected \"y\"", "two terms without an operator are reported" );

        // An implicit unit sphere matches the sphere, scaled and from inside.
    pObject = FirstObject( oScene, "{ \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"x^2 + y^2 + z^2 - 1\", \"bounds\": 1.5 }" );
    if ( pObject )
    {
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "a ray hits an implicit sphere where a sphere is hit" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, 0, 1, 0, 0 ), 1.0, 1, 0, 0 ), "a ray from inside an implicit sphere leaves it" );
        Check( Misses( *pObject, RayFrom( 1.01, 0, -5, 0, 0, 1 ) ), "a ray beside an implicit sphere misses it" );
        Check( pObject->IsClosed() ? true : false, "an implicit surface is a solid" );

        CScene    oSphereScene;
        CObjects *pSphere = FirstObject( oSphereScene, "{ \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 }" );
        int       nBad = 0;

        for ( int n = 0; pSphere && n < 300; n++ )
        {
            double  a = n * 2.399963, b = ( ( n % 37 ) / 36.0 * 2.8 ) - 1.4;
            double  ox = 4 * cos( a ), oz = 4 * sin( a ), oy = 0.7 * sin( n * 0.7 );
            double  tx = 0.8 * sin( n * 1.3 ), ty = 0.8 * b / 1.4, tz = 0.8 * cos( n * 0.9 );
            double  dx = tx - ox, dy = ty - oy, dz = tz - oz, l = sqrt( ( dx * dx ) + ( dy * dy ) + ( dz * dz ) );
            CRay    oA = RayFrom( ox, oy, oz, dx / l, dy / l, dz / l ), oB = oA;
            CVector oHitA, oHitB, oNA, oNB;
            bool    bA = pObject->Intersect( oA, oHitA ) ? true : false, bB = pSphere->Intersect( oB, oHitB ) ? true : false;

            if ( bA != bB )
                nBad++;
            else if ( bA )
            {
                pObject->SurfaceNormal( oHitA, oNA );
                pSphere->SurfaceNormal( oHitB, oNB );
                if ( ABS( oA.GetT() - oB.GetT() ) > 1e-9 || ABS( oNA.GetX() - oNB.GetX() ) > 1e-6 || ABS( oNA.GetY() - oNB.GetY() ) > 1e-6 ||
                     ABS( oNA.GetZ() - oNB.GetZ() ) > 1e-6 )
                    nBad++;
            }
        }
        Check( nBad == 0, "an implicit sphere matches the sphere on 300 rays, hits and normals" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"x^2 + y^2 + z^2 - 1\", \"bounds\": 1, "
                                   "\"scale\": 2 }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 3.0, 0, 0, -1 ), "scale grows an implicit surface" );
    pObject = FirstObject( oScene, "{ \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"sqrt(x^2 + y^2 + z^2) - 1\", "
                                   "\"bounds\": 1.5, \"steps\": 8, \"lipschitz\": 1 }" );
    if ( pObject )
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 4.0, 0, 0, -1 ), "with a Lipschitz bound, long steps still find the surface" );

        // A gyroid: the hit is the first sign change along the ray, found by
        // a much finer march, and lies on the surface.
    pObject = FirstObject( oScene, "{ \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": "
                                   "\"sin(k*x)*cos(k*y) + sin(k*y)*cos(k*z) + sin(k*z)*cos(k*x)\", \"parameters\": { \"k\": 5 }, \"bounds\": 1 }" );
    if ( pObject )
    {
        CExpression                   oGyroid;
        std::map<std::string, double> aoParameters;
        std::string                   strError;
        int                           nBad = 0, nHits = 0;

        aoParameters[ "k" ] = 5.0;
        oGyroid.Compile( "sin(k*x)*cos(k*y) + sin(k*y)*cos(k*z) + sin(k*z)*cos(k*x)", aoParameters, strError );
        for ( int n = 0; n < 60; n++ )
        {
            double a = n * 2.399963, dy = 0.3 * sin( n * 1.7 );
            double dx = cos( a ), dz = sin( a ), l = sqrt( ( dx * dx ) + ( dy * dy ) + ( dz * dz ) );
            double ox = ( -3 * dx / l ) + ( 0.2 * sin( (double)n ) ), oy = -3 * dy / l, oz = -3 * dz / l;
            CRay   oRay = RayFrom( ox, oy, oz, dx / l, dy / l, dz / l );

            if ( ! pObject->Intersect( oRay, oHit ) )
                continue;
            nHits++;

            double tHit = oRay.GetT();
            double tFirst = -1.0, fPrevious = 0.0;

                // The finer march, inside the box.
            for ( int s = 0; s <= 200000; s++ )
            {
                double t = s * 6.0 / 200000;
                double x = ox + ( t * dx / l ), y = oy + ( t * dy / l ), z = oz + ( t * dz / l );

                if ( ABS( x ) > 1 || ABS( y ) > 1 || ABS( z ) > 1 )
                {
                    if ( fPrevious != 0.0 )
                        break;
                    continue;
                }

                double f = oGyroid.Value( x, y, z );

                if ( fPrevious != 0.0 && ( f < 0 ) != ( fPrevious < 0 ) )
                {
                    tFirst = t;
                    break;
                }
                fPrevious = ( f == 0.0 ) ? 1e-300 : f;
            }
            if ( ABS( tHit - tFirst ) > ( 6.0 / 200000 ) + 1e-9 || ABS( oGyroid.Value( oHit.GetX(), oHit.GetY(), oHit.GetZ() ) ) > 1e-9 )
                nBad++;
        }
        Check( nHits > 30 && nBad == 0, "a gyroid is hit at the first sign change along each ray, on the surface" );
    }

        // An implicit surface in a csg: a cube with a ball cut out of it.
    pObject = FirstObject( oScene, "{ \"type\": \"csg\", \"operation\": \"difference\", \"objects\": [ "
                                   "{ \"type\": \"box\", \"center\": [0, 0, 0], \"size\": 2 }, "
                                   "{ \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"x^2 + y^2 + z^2 - 0.25\", \"bounds\": 0.6 } ] }" );
    if ( pObject )
    {
        CRay oOutside = RayFrom( -5, 0, 0, 1, 0, 0 ), oInside = RayFrom( 0, 0, 0, 1, 0, 0 ), oCorner = RayFrom( -5, 0.9, 0.9, 1, 0, 0 );

        Check( pObject->Intersect( oOutside, oHit ) && ABS( oOutside.GetT() - 4.0 ) < 1e-9, "a csg with an implicit part is hit on its box" );
        Check( pObject->Intersect( oInside, oHit ) && ABS( oInside.GetT() - 0.5 ) < 1e-9,
               "a ray from inside the hole an implicit ball cut hits the hole's wall" );
        Check( pObject->Intersect( oCorner, oHit ) && ABS( oCorner.GetT() - 4.0 ) < 1e-9, "away from the hole the box is solid" );
    }

        // A lone metaball is a sphere of radius R sqrt( 1 - (threshold/s)^(1/3) ).
    pObject = FirstObject( oScene, "{ \"type\": \"blob\", \"center\": [0, 0, 0], \"balls\": [ { \"center\": [0, 0, 0], \"radius\": 2 } ] }" );
    if ( pObject )
    {
        double  r = 2.0 * sqrt( 1.0 - cbrt( 0.5 ) );
        CBlob  *pBlob = dynamic_cast<CBlob *>( pObject );
        double  anCenter[ 3 ] = { 0, 0, 0 }, anOne[ 3 ] = { 1, 0, 0 };

        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 5.0 - r, 0, 0, -1 ), "a lone metaball is the sphere of the right radius" );
        Check( ShapeHitsAt( *pObject, RayFrom( 0, 0, 0, 0, 1, 0 ), r, 0, 1, 0 ), "a ray from inside a metaball leaves it" );
        Check( pBlob && pBlob->Sum( anCenter ) == 1.0 && ABS( pBlob->Sum( anOne ) - 0.421875 ) < 1e-15,
               "a ball's field is its strength at its center and (1 - r^2/R^2)^3 of it at r" );
        Check( pObject->IsClosed() ? true : false, "a blob is a solid" );
    }

        // Two balls melt together: a ray down between them hits the bridge,
        // whose top faces straight up.
    pObject = FirstObject( oScene, "{ \"type\": \"blob\", \"center\": [0, 0, 0], \"balls\": [ { \"center\": [-0.6, 0, 0], \"radius\": 1.5 }, "
                                   "{ \"center\": [0.6, 0, 0], \"radius\": 1.5 } ], \"threshold\": 0.4 }" );
    if ( pObject )
    {
        CRay    oDown = RayFrom( 0, 5, 0, 0, -1, 0 );
        CVector oNormal;
        CBlob  *pBlob = dynamic_cast<CBlob *>( pObject );

        Check( pObject->Intersect( oDown, oHit ) && pObject->SurfaceNormal( oHit, oNormal ) && ABS( oNormal.GetY() - 1.0 ) < 1e-9,
               "two metaballs merge, with a smooth bridge between them" );

        double anAt[ 3 ] = { oHit.GetX(), oHit.GetY(), oHit.GetZ() };

        Check( pBlob && ABS( pBlob->Sum( anAt ) - 0.4 ) < 1e-9, "the bridge's surface is where the sum is the threshold" );
    }

        // A negative ball dents a positive one.
    pObject = FirstObject( oScene, "{ \"type\": \"blob\", \"center\": [0, 0, 0], \"balls\": [ { \"center\": [0, 0, 0], \"radius\": 2 }, "
                                   "{ \"center\": [0, 0, -1], \"radius\": 0.8, \"strength\": -1 } ] }" );
    if ( pObject )
    {
        CRay oDent = RayFrom( 0, 0, -5, 0, 0, 1 ), oSide = RayFrom( 0, 0, 5, 0, 0, -1 );

        Check( pObject->Intersect( oDent, oHit ) && pObject->Intersect( oSide, oHit ) && oDent.GetT() > oSide.GetT() + 0.05,
               "a ball of negative strength dents the blob" );
    }

        // Fractals: a menger sponge of no iterations is the cube, one of one
        // iteration is drilled through the middle, and the trace is the same
        // every time.
    pObject = FirstObject( oScene, "{ \"type\": \"fractal\", \"kind\": \"menger\", \"center\": [0, 0, 0], \"iterations\": 0 }" );
    if ( pObject )
    {
        CRay    oRay = RayFrom( 0.3, 0.2, -5, 0, 0, 1 );
        CVector oNormal;

        Check( pObject->Intersect( oRay, oHit ) && oRay.GetT() <= 4.0 && oRay.GetT() > 4.0 - 2e-4 &&
               pObject->SurfaceNormal( oHit, oNormal ) && ABS( oNormal.GetZ() + 1.0 ) < 1e-3, "a menger sponge of 0 iterations is the cube" );
        Check( ! pObject->IsClosed(), "a fractal is not a solid for a csg" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"fractal\", \"kind\": \"menger\", \"center\": [0, 0, 0], \"iterations\": 1 }" );
    if ( pObject )
    {
        Check( Misses( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ) ), "a ray through a menger sponge's middle hole misses it" );
        Check( ! Misses( *pObject, RayFrom( 0.8, 0.8, -5, 0, 0, 1 ) ), "a ray at a menger sponge's corner hits it" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"fractal\", \"kind\": \"sierpinski\", \"center\": [0, 0, 0], \"iterations\": 0 }" );
    if ( pObject )
    {
        CFractal *pFractal = dynamic_cast<CFractal *>( pObject );
        double    anCenter[ 3 ] = { 0, 0, 0 }, anCorner[ 3 ] = { 0.99, 0.99, 0.99 }, anOpposite[ 3 ] = { -0.9, -0.9, -0.9 };

        Check( pFractal && pFractal->Estimate( anCenter ) < 0 && pFractal->Estimate( anCorner ) < 0 && pFractal->Estimate( anOpposite ) > 0,
               "a sierpinski tetrahedron of 0 iterations is the tetrahedron" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"fractal\", \"kind\": \"mandelbulb\", \"center\": [0, 0, 0], \"power\": 2 }" );
    if ( pObject )
    {
        CFractal *pFractal = dynamic_cast<CFractal *>( pObject );
        double    anOrigin[ 3 ] = { 0, 0, 0 }, anFar[ 3 ] = { 3, 0, 0 };

        Check( pFractal && pFractal->Estimate( anOrigin ) < 0 && pFractal->Estimate( anFar ) > 0 && pFractal->Estimate( anFar ) < 3,
               "a power-2 mandelbulb holds the origin, and its estimate outside is positive and no more than the distance" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"fractal\", \"kind\": \"mandelbulb\", \"center\": [0, 0, 0] }" );
    if ( pObject )
    {
        CRay    oA = RayFrom( 0.1, 0.05, -5, 0, 0, 1 ), oB = oA;
        CVector oHitB;

        Check( pObject->Intersect( oA, oHit ) && pObject->Intersect( oB, oHitB ) && oA.GetT() == oB.GetT() && oA.GetT() > 3.5 && oA.GetT() < 4.5,
               "a mandelbulb is hit near its heart, the same every time" );
    }

        // Round trips.
    {
        CScene      oRound, oAgain;
        std::string strText = Minimal( "\"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 } ]",
            "\"objects\": [ { \"type\": \"implicit\", \"center\": [1, 2, 3], \"formula\": \"x^2 + a*y^2 + z^2 - b\", "
            "\"parameters\": { \"b\": 1, \"a\": 2 }, \"bounds\": [1, 2, 3], \"steps\": 100, \"lipschitz\": 4, \"scale\": 0.5, \"rotation\": [10, 20, 30] }, "
            "{ \"type\": \"blob\", \"center\": [0, 0, 0], \"balls\": [ { \"center\": [0, 1, 0], \"radius\": 1, \"strength\": -0.5 }, "
            "{ \"center\": [1, 1, 0], \"radius\": 2 } ], \"threshold\": 0.3 }, "
            "{ \"type\": \"fractal\", \"kind\": \"mandelbulb\", \"center\": [0, 0, 0], \"power\": 6, \"iterations\": 7, \"bailout\": 3, "
            "\"epsilon\": 0.001, \"maxSteps\": 100, \"stepFactor\": 0.9 }, "
            "{ \"type\": \"fractal\", \"kind\": \"sierpinski\", \"center\": [0, 0, 0] } ]" );

        if ( oRound.FromJson( strText ) == 0 )
        {
            std::string strSaved = oRound.ToJson();

            Check( oAgain.FromJson( strSaved ) == 0 && oAgain.ToJson() == strSaved, "implicit surfaces, blobs and fractals read back as saved" );
            Check( strSaved.find( "\"b\": 1" ) != std::string::npos && strSaved.find( "\"a\": 2" ) > strSaved.find( "\"b\": 1" ),
                   "an implicit surface's parameters keep their order" );
        }
        else
            Check( false, oRound.GetLoadError() );
    }

        // Bad settings are reported.
    std::string strObjects = "\"objects\": [ { \"type\": \"sphere\", \"center\": [0, 0, 0], \"radius\": 1 } ]";

    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"x + \", \"bounds\": 1 } ]" ),
                 BAD_FILE, "column 5: the formula ends too soon" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"x\", \"bounds\": 0 } ]" ),
                 BAD_FILE, "bounds must be more than 0" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"x\", \"bounds\": 1, "
                                              "\"parameters\": { \"x\": 1 } } ]" ),
                 BAD_FILE, "\"x\" is taken by the formula language" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"implicit\", \"center\": [0, 0, 0], \"formula\": \"x\" } ]" ),
                 BAD_FILE, "bounds" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"blob\", \"center\": [0, 0, 0], \"balls\": [ "
                                              "{ \"center\": [0, 0, 0], \"radius\": 1, \"strength\": -1 } ] } ]" ),
                 BAD_FILE, "at least one ball of positive strength" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"blob\", \"center\": [0, 0, 0], \"balls\": [ "
                                              "{ \"center\": [0, 0, 0], \"radius\": 0 } ] } ]" ),
                 BAD_FILE, "radius must be more than 0" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"fractal\", \"center\": [0, 0, 0], \"kind\": \"julia\" } ]" ),
                 BAD_FILE, "kind must be mandelbulb, menger or sierpinski" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"fractal\", \"center\": [0, 0, 0], \"kind\": \"menger\", \"power\": 3 } ]" ),
                 BAD_FILE, "only a mandelbulb has a power and a bailout" );
    ExpectError( Minimal( strObjects.c_str(), "\"objects\": [ { \"type\": \"fractal\", \"center\": [0, 0, 0], \"kind\": \"menger\", \"epsilon\": 0 } ]" ),
                 BAD_FILE, "epsilon must be more than 0" );
}

static void ExtrusionChecks( void )
{
    CScene      oScene;
    CObjects   *pObject;
    CVector     oHit;
    std::string strSquare = "[ [-1, -1], [1, -1], [1, 1], [-1, 1] ]";
    std::string strHole = "[ [-0.5, -0.5], [0.5, -0.5], [0.5, 0.5], [-0.5, 0.5] ]";

        // A square pushed back 2 along +z: a box from z = 0 to 2.
    pObject = FirstObject( oScene, ( "{ \"type\": \"extrusion\", \"outline\": " + strSquare + ", \"depth\": 2 }" ).c_str() );
    if ( pObject )
    {
        Check( MeshHitsAt( *pObject, RayFrom( 0.2, 0.3, -5, 0, 0, 1 ), 5.0 ), "a ray hits an extrusion's face" );
        Check( MeshHitsAt( *pObject, RayFrom( 5, 0.2, 1, -1, 0, 0 ), 4.0 ), "a ray hits an extrusion's side, square at a corner" );
        Check( MeshHitsAt( *pObject, RayFrom( 0.2, 0.3, 5, 0, 0, -1 ), 3.0 ), "a ray hits an extrusion's back" );
        Check( pObject->IsClosed(), "an extrusion is a solid" );
    }

        // A hole, worked out from the nesting; the edges beveled front and back.
    pObject = FirstObject( oScene, ( "{ \"type\": \"extrusion\", \"outlines\": [ " + strSquare + ", " + strHole +
                                     " ], \"depth\": 1, \"bevel\": 0.1, \"backBevel\": 0.1 }" ).c_str() );
    if ( pObject )
    {
        CExtrusion *pExtrusion = dynamic_cast<CExtrusion *>( pObject );

        Check( Misses( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ) ), "a ray through an extrusion's hole passes through" );
        Check( MeshHitsAt( *pObject, RayFrom( 0.75, 0, -5, 0, 0, 1 ), 5.0 ), "a ray onto an extrusion's frame hits its face" );
        Check( pExtrusion && pExtrusion->GetGeometry() && OpenEdges( *pExtrusion->GetGeometry() ) == 0,
               "a beveled extrusion with a hole is watertight" );
    }

        // Tapered to half: the sides lean in, as planes.
    pObject = FirstObject( oScene, ( "{ \"type\": \"extrusion\", \"outline\": " + strSquare + ", \"depth\": 1, \"taper\": 0.5 }" ).c_str() );
    if ( pObject )
    {
        CExtrusion *pExtrusion = dynamic_cast<CExtrusion *>( pObject );
        double      dL = sqrt( 1.25 );

        Check( MeshHitsAt( *pObject, RayFrom( 5, 0, 0.5, -1, 0, 0 ), 4.25 ),
               "a tapered extrusion's side is where its taper puts it, leaning back" );
        Check( pExtrusion && pExtrusion->GetGeometry() && OpenEdges( *pExtrusion->GetGeometry() ) == 0, "a tapered extrusion is watertight" );
    }

        // Twisted a quarter turn: halfway back the square has turned 45
        // degrees, so its corner reaches out along x.
    pObject = FirstObject( oScene, ( "{ \"type\": \"extrusion\", \"outline\": " + strSquare + ", \"depth\": 1, \"twist\": 90 }" ).c_str() );
    if ( pObject )
    {
        CExtrusion *pExtrusion = dynamic_cast<CExtrusion *>( pObject );
        CRay        oRay = RayFrom( 5, 0.01, 0.5, -1, 0, 0 );
        bool        bHit = pObject->Intersect( oRay, oHit );

        Check( bHit && ABS( oHit.GetX() - sqrt( 2.0 ) ) < 0.02, "a twisted extrusion has turned halfway back" );
        Check( MeshHitsAt( *pObject, RayFrom( 0.5, 0.5, 5, 0, 0, -1 ), 4.0 ), "a twisted extrusion's back face is turned square" );
        Check( pExtrusion && pExtrusion->GetGeometry() && OpenEdges( *pExtrusion->GetGeometry() ) == 0, "a twisted extrusion is watertight" );
    }

        // A straight round tube along z, capped.
    pObject = FirstObject( oScene, "{ \"type\": \"sweep\", \"path\": [ [0, 0, 0], [0, 0, 2] ], \"smooth\": false, \"radius\": 0.5 }" );
    if ( pObject )
    {
        CSweep *pSweep = dynamic_cast<CSweep *>( pObject );
        CRay    oRay = RayFrom( 0, 5, 1, 0, -1, 0 );

        Check( pObject->Intersect( oRay, oHit ) && oRay.GetT() >= 4.5 - 1e-9 && oRay.GetT() <= 5.0 - ( 0.5 * cos( PI / 24 ) ) + 1e-9,
               "a ray hits a sweep's tube between its inner and outer radius" );
        Check( MeshHitsAt( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ), 5.0 ), "a sweep's open end is capped" );
        Check( pSweep && pSweep->GetGeometry() && OpenEdges( *pSweep->GetGeometry() ) == 0, "a capped sweep is watertight" );
    }

        // A spring, capped, and a tube round a closed path, whose frame
        // carried round meets itself, so it is watertight with no caps.
    std::string strHelix = "[";

    for ( int n = 0; n <= 36; n++ )
        strHelix += ( n ? ", [" : " [" ) + std::to_string( cos( n * PI / 6 ) ) + ", " + std::to_string( n * 0.05 ) + ", " +
                    std::to_string( sin( n * PI / 6 ) ) + "]";
    strHelix += " ]";
    pObject = FirstObject( oScene, ( "{ \"type\": \"sweep\", \"path\": " + strHelix + ", \"radius\": 0.08 }" ).c_str() );
    if ( pObject )
    {
        CSweep *pSweep = dynamic_cast<CSweep *>( pObject );

        Check( pSweep && pSweep->GetGeometry() && OpenEdges( *pSweep->GetGeometry() ) == 0, "a capped spring is watertight" );
    }
    pObject = FirstObject( oScene, "{ \"type\": \"sweep\", \"closed\": true, \"radius\": 0.2, "
                                   "\"path\": [ [1, 0, 0], [0, 0.3, 1], [-1, 0, 0], [0, -0.3, -1] ] }" );
    if ( pObject )
    {
        CSweep *pSweep = dynamic_cast<CSweep *>( pObject );

        Check( pSweep && pSweep->GetGeometry() && OpenEdges( *pSweep->GetGeometry() ) == 0, "a sweep round a closed path meets itself" );
    }

        // An extrusion in a csg: a slab with a square hole through it.
    pObject = FirstObject( oScene, CsgOf( "difference", "{ \"type\": \"box\", \"center\": [0, 0, 1], \"size\": [3, 3, 0.5] },"
                                                        "{ \"type\": \"extrusion\", \"outline\": [ [-0.5, -0.5], [0.5, -0.5], [0.5, 0.5], [-0.5, 0.5] ], "
                                                        "\"depth\": 2 }" ).c_str() );
    if ( pObject )
    {
        Check( Misses( *pObject, RayFrom( 0, 0, -5, 0, 0, 1 ) ), "a slab with an extrusion taken out is open there" );
        Check( CsgHitsAt( *pObject, RayFrom( 1, 0, -5, 0, 0, 1 ), 5.75, 0, 0, -1 ), "the slab is whole beside the extrusion's hole" );
    }

    ExpectError( Minimal( s_strSphereObject, ( "\"type\": \"extrusion\", \"outline\": " + strSquare + ", \"bevel\": 0.1, \"taper\": 0.5" ).c_str() ),
                 BAD_FILE, "can't be used with a taper or a twist" );
    ExpectError( Minimal( s_strSphereObject, ( "\"type\": \"extrusion\", \"outline\": " + strSquare + ", \"depth\": 0" ).c_str() ),
                 BAD_FILE, "depth must be more than 0" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"extrusion\", \"outline\": [ [0, 0], [1, 0] ]" ), BAD_FILE, "at least three points" );
    ExpectError( Minimal( s_strSphereObject, ( "\"type\": \"extrusion\", \"outline\": " + strSquare + ", \"outlines\": [ " + strSquare + " ]" ).c_str() ),
                 BAD_FILE, "either outline" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"sweep\", \"path\": [ [0, 0, 0], [0, 0, 1] ], \"radius\": 0.1, "
                                             "\"outline\": [ [0, 0], [1, 0], [0, 1] ]" ),
                 BAD_FILE, "either radius" );
    ExpectError( Minimal( s_strSphereObject, "\"type\": \"sweep\", \"path\": [ [0, 0, 0] ]" ), BAD_FILE, "at least two points" );
}

static void PatchFileChecks( const char *strScenes )
{
    std::string                  strText = ReadFile( std::string( strScenes ) + "/models/teapot.bpt" );
    std::vector<CBezier::SPatch> aoPatches;
    std::string                  strError;
    CBezier                      oTeapot;

    Check( CBezier::ReadBpt( strText, aoPatches, strError ) && aoPatches.size() == 32, "the Utah teapot's 32 patches read" );
    if ( aoPatches.size() != 32 || ! oTeapot.SetPatches( aoPatches ) )
    {
        Check( false, "the teapot's patches set up" );
        return;
    }

        // Its knob and base: every patch has a finite, unit normal at every
        // corner, edges that shrink to a point included.
    bool bNormals = true;

    for ( size_t nPatch = 0; nPatch < aoPatches.size(); nPatch++ )
    {
        for ( int nCorner = 0; nCorner < 4; nCorner++ )
        {
            double anNormal[ 3 ];

            CBezier::Normal( aoPatches[ nPatch ], ( nCorner & 1 ) ? 1.0 : 0.0, ( nCorner & 2 ) ? 1.0 : 0.0, anNormal );

            double nLength = sqrt( ( anNormal[ 0 ] * anNormal[ 0 ] ) + ( anNormal[ 1 ] * anNormal[ 1 ] ) + ( anNormal[ 2 ] * anNormal[ 2 ] ) );

            bNormals = bNormals && ( nLength == nLength ) && ABS( nLength - 1.0 ) < 1e-9;
        }
    }
    Check( bNormals, "every corner of the teapot's patches, its knob and base included, has a unit normal" );

        // Rays at the teapot from all around agree with a fine mesh of its
        // patches: the same hits, at nearly the same distance, except for a
        // few rays grazing an edge. (The data has z up.)
    std::vector<FLT> anTriangles, anNormals;

    oTeapot.Triangles( 48, anTriangles, anNormals );

    unsigned int nState = 4242u;
    int          nAgree = 0, nTotal = 600, nHits = 0;

    for ( int n = 0; n < nTotal; n++ )
    {
        double a = PatchRandom( nState ) * 2.0 * PI, b = ( PatchRandom( nState ) - 0.5 ) * PI;
        double anFrom[ 3 ] = { 10.0 * cos( a ) * cos( b ), 10.0 * sin( a ) * cos( b ), 1.5 + ( 10.0 * sin( b ) ) };
        double anAt[ 3 ] = { ( PatchRandom( nState ) - 0.5 ) * 6.0, ( PatchRandom( nState ) - 0.5 ) * 4.0, PatchRandom( nState ) * 3.3 };
        double anDirection[ 3 ] = { anAt[ 0 ] - anFrom[ 0 ], anAt[ 1 ] - anFrom[ 1 ], anAt[ 2 ] - anFrom[ 2 ] };
        CRay   oRay = RayFrom( anFrom[ 0 ], anFrom[ 1 ], anFrom[ 2 ], anDirection[ 0 ], anDirection[ 1 ], anDirection[ 2 ] );
        CVector oHit;
        bool   bHit = oTeapot.Intersect( oRay, oHit ) ? true : false;
        double nT = bHit ? oRay.GetT() : 0.0;
        double nMeshT = 1e300;

        for ( size_t k = 0; k + 8 < anTriangles.size(); k += 9 )
        {
            double e1[ 3 ], e2[ 3 ], s[ 3 ], p[ 3 ], q[ 3 ];

            for ( int c = 0; c < 3; c++ )
            {
                e1[ c ] = anTriangles[ k + 3 + c ] - anTriangles[ k + c ];
                e2[ c ] = anTriangles[ k + 6 + c ] - anTriangles[ k + c ];
                s[ c ] = anFrom[ c ] - anTriangles[ k + c ];
            }
            p[ 0 ] = ( anDirection[ 1 ] * e2[ 2 ] ) - ( anDirection[ 2 ] * e2[ 1 ] );
            p[ 1 ] = ( anDirection[ 2 ] * e2[ 0 ] ) - ( anDirection[ 0 ] * e2[ 2 ] );
            p[ 2 ] = ( anDirection[ 0 ] * e2[ 1 ] ) - ( anDirection[ 1 ] * e2[ 0 ] );

            double nDet = ( e1[ 0 ] * p[ 0 ] ) + ( e1[ 1 ] * p[ 1 ] ) + ( e1[ 2 ] * p[ 2 ] );

            if ( ABS( nDet ) < 1e-14 )
                continue;

            double u = ( ( s[ 0 ] * p[ 0 ] ) + ( s[ 1 ] * p[ 1 ] ) + ( s[ 2 ] * p[ 2 ] ) ) / nDet;

            q[ 0 ] = ( s[ 1 ] * e1[ 2 ] ) - ( s[ 2 ] * e1[ 1 ] );
            q[ 1 ] = ( s[ 2 ] * e1[ 0 ] ) - ( s[ 0 ] * e1[ 2 ] );
            q[ 2 ] = ( s[ 0 ] * e1[ 1 ] ) - ( s[ 1 ] * e1[ 0 ] );

            double v = ( ( anDirection[ 0 ] * q[ 0 ] ) + ( anDirection[ 1 ] * q[ 1 ] ) + ( anDirection[ 2 ] * q[ 2 ] ) ) / nDet;
            double t = ( ( e2[ 0 ] * q[ 0 ] ) + ( e2[ 1 ] * q[ 1 ] ) + ( e2[ 2 ] * q[ 2 ] ) ) / nDet;

            if ( u >= 0 && v >= 0 && u + v <= 1 && t > 1e-9 && t < nMeshT )
                nMeshT = t;
        }

        bool bMeshHit = nMeshT < 1e300;

        nHits += bHit ? 1 : 0;
        if ( bHit == bMeshHit && ( ! bHit || ABS( nT - nMeshT ) < 2e-3 ) )
            nAgree++;
    }
    if ( nAgree < nTotal - 6 )
        fprintf( stderr, "  patch teapot against its mesh: %d of %d agree, %d hits\n", nAgree, nTotal, nHits );
    Check( nHits > nTotal / 3 && nAgree >= nTotal - 6, "600 rays at the patch teapot agree with a fine mesh of it (all but a few grazing rays)" );

        // Rays aimed at points on the seams between its patches all hit:
        // there or nearer.
    bool bSeams = true;

    for ( size_t nPatch = 0; nPatch < aoPatches.size() && bSeams; nPatch++ )
    {
        for ( int n = 0; n <= 8 && bSeams; n++ )
        {
            double anPoint[ 3 ], anDu[ 3 ], anDv[ 3 ], anNormal[ 3 ];
            double u = ( n % 2 ) ? n / 8.0 : 0.0, v = ( n % 2 ) ? 0.0 : n / 8.0;

            CBezier::Evaluate( aoPatches[ nPatch ], u, v, anPoint, anDu, anDv );
            CBezier::Normal( aoPatches[ nPatch ], u, v, anNormal );

            double anFrom[ 3 ];

            for ( int c = 0; c < 3; c++ )
                anFrom[ c ] = anPoint[ c ] + ( 2.0 * anNormal[ c ] );

            CRay    oRay = RayFrom( anFrom[ 0 ], anFrom[ 1 ], anFrom[ 2 ], -anNormal[ 0 ], -anNormal[ 1 ], -anNormal[ 2 ] );
            CVector oHit;

            bSeams = oTeapot.Intersect( oRay, oHit ) && oRay.GetT() <= 2.0 + 1e-7;
        }
    }
    Check( bSeams, "rays aimed at points on the seams between the teapot's patches all hit" );
    Check( oTeapot.PieceCount() > 32, "the teapot's patches are split into flat pieces" );
}

// --------------------------------------------------------------------------
// JPEG and WebP: the fixtures tools/make_image_fixtures.py writes, each with
// Pillow's decode beside it (libjpeg-turbo and libwebp).
// --------------------------------------------------------------------------

    // Decodes a JPEG or WebP with Ray's own decoder.
static bool DecodeImageFile( const std::string &strData, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels, std::string &strError,
                             std::vector<BYTE> *pAlpha = NULL )
{
    if ( CJpegImage::IsJpeg( strData ) )
        return CJpegImage::Decode( strData, nWidth, nHeight, anPixels, strError );
    return CWebpImage::Decode( strData, nWidth, nHeight, anPixels, strError, pAlpha );
}

    // The largest difference of a fixture's decode from Pillow's, with the
    // mean in dMean, or -1 if it doesn't decode or the sizes differ.
static int CompareFixture( const std::string &strFolder, const char *strName, double &dMean, bool bAlpha = false )
{
    std::string       strData = ReadFile( strFolder + strName );
    std::string       strBase = std::string( strName ).substr( 0, strlen( strName ) - ( strstr( strName, ".webp" ) ? 5 : 4 ) );
    std::string       strRef = ReadFile( strFolder + strBase + ( bAlpha ? "_alpha.png" : "_ref.png" ) );
    int               nWidth = 0, nHeight = 0, nRefWidth = 0, nRefHeight = 0;
    std::vector<BYTE> anPixels, anRef, anAlpha;
    std::string       strError;

    dMean = 0.0;
    if ( ! DecodeImageFile( strData, nWidth, nHeight, anPixels, strError, &anAlpha ) )
    {
        fprintf( stderr, "%s: %s\n", strName, strError.c_str() );
        return -1;
    }
    if ( ! CPngImage::Decode( strRef, nRefWidth, nRefHeight, anRef, strError ) || nWidth != nRefWidth || nHeight != nRefHeight )
    {
        fprintf( stderr, "%s: %d x %d against the reference's %d x %d\n", strName, nWidth, nHeight, nRefWidth, nRefHeight );
        return -1;
    }

    int    nWorst = 0;
    double dSum = 0.0;
    size_t nCount = 0;

    for ( size_t n = 0; n < (size_t)nWidth * nHeight; n++ )
        for ( int c = 0; c < ( bAlpha ? 1 : 3 ); c++ )
        {
                // The alpha reference is gray, so its PNG decodes to R = G = B.
            int nMine = bAlpha ? anAlpha[ n ] : anPixels[ n * 3 + c ];
            int nTheirs = anRef[ n * 3 + c ];
            int nDiff = abs( nMine - nTheirs );

            nWorst = (std::max)( nWorst, nDiff );
            dSum += nDiff;
            nCount++;
        }
    dMean = nCount ? dSum / (double)nCount : 0.0;
    return nWorst;
}

static void ImageFormatChecks( const char *strScenes )
{
    std::string strFolder = std::string( strScenes ) + "/textures/formats/";

        // JPEG: as libjpeg-turbo decodes it (the same integer DCT,
        // upsampling and color conversion), within 1.
    const char *astrJpegs[] = { "base444.jpg", "base422.jpg", "base420.jpg", "prog420.jpg", "prog444.jpg", "gray.jpg",
                                "restart.jpg", "exif3.jpg", "exif6.jpg", "exif8.jpg" };

    for ( const char *strName : astrJpegs )
    {
        double dMean = 0.0;
        int    nWorst = CompareFixture( strFolder, strName, dMean );

        if ( nWorst != 0 )
            fprintf( stderr, "  %s: largest difference from libjpeg-turbo %d, mean %.4f\n", strName, nWorst, dMean );
        Check( nWorst >= 0 && nWorst <= 1 && dMean < 0.05, ( std::string( "the JPEG " ) + strName + " decodes as libjpeg-turbo does, within 1" ).c_str() );
    }

        // WebP: lossless exactly; lossy as libwebp decodes it, within 1.
    const char *astrLossless[] = { "lossless.webp", "lossless_fast.webp", "lossless_alpha.webp", "palette2.webp", "palette4.webp",
                                   "palette12.webp", "palette100.webp" };

    for ( const char *strName : astrLossless )
    {
        double dMean = 0.0;

        Check( CompareFixture( strFolder, strName, dMean ) == 0, ( std::string( "the lossless WebP " ) + strName + " decodes exactly" ).c_str() );
    }
    {
        double dMean = 0.0;

        Check( CompareFixture( strFolder, "lossless_alpha.webp", dMean, true ) == 0, "lossless_alpha.webp's alpha decodes exactly" );
    }

    const char *astrLossy[] = { "lossy.webp", "lossy_small.webp", "lossy_alpha.webp", "lossy_alpha_lossy.webp", "lossy_simple.webp",
                                "lossy_sharp.webp", "lossy_parts.webp", "lossy_low.webp" };

    for ( const char *strName : astrLossy )
    {
        double dMean = 0.0;
        int    nWorst = CompareFixture( strFolder, strName, dMean );

        if ( nWorst != 0 )
            fprintf( stderr, "  %s: largest difference from libwebp %d, mean %.4f\n", strName, nWorst, dMean );
        Check( nWorst >= 0 && nWorst <= 1 && dMean < 0.05, ( std::string( "the lossy WebP " ) + strName + " decodes as libwebp does, within 1" ).c_str() );
    }
    for ( const char *strName : { "lossy_alpha.webp", "lossy_alpha_lossy.webp" } )
    {
        double dMean = 0.0;

        Check( CompareFixture( strFolder, strName, dMean, true ) == 0, ( std::string( strName ) + "'s alpha decodes exactly" ).c_str() );
    }

        // What isn't read is reported, with a reason.
    {
        int               nWidth = 0, nHeight = 0;
        std::vector<BYTE> anPixels;
        std::string       strError;
        std::string       strBase = ReadFile( strFolder + "base444.jpg" );
        std::string       strData;
        size_t            nFrame = strBase.find( "\xFF\xC0" );

        Check( ! CJpegImage::Decode( ReadFile( strFolder + "cmyk.jpg" ), nWidth, nHeight, anPixels, strError ) &&
               strError.find( "CMYK" ) != std::string::npos, "a CMYK JPEG is refused, saying so" );
        Check( nFrame != std::string::npos, "base444.jpg has a baseline frame header" );
        if ( nFrame != std::string::npos )
        {
            strData = strBase;
            strData[ nFrame + 1 ] = (char)0xC9;
            strError.clear();
            Check( ! CJpegImage::Decode( strData, nWidth, nHeight, anPixels, strError ) && strError.find( "arithmetic" ) != std::string::npos,
                   "an arithmetic-coded JPEG is refused, saying so" );
            strData = strBase;
            strData[ nFrame + 4 ] = 12;
            strError.clear();
            Check( ! CJpegImage::Decode( strData, nWidth, nHeight, anPixels, strError ) && strError.find( "12-bit" ) != std::string::npos,
                   "a 12-bit JPEG is refused, saying so" );
            strData = strBase;
            strData[ nFrame + 1 ] = (char)0xC3;
            strError.clear();
            Check( ! CJpegImage::Decode( strData, nWidth, nHeight, anPixels, strError ) && strError.find( "lossless" ) != std::string::npos,
                   "a lossless JPEG is refused, saying so" );
        }

            // An animated WebP: a VP8X chunk with the animation flag.
        std::string strAnimated( "RIFF\x16\0\0\0WEBPVP8X\x0A\0\0\0\x02\0\0\0\0\0\0\0\0\0", 30 );

        strError.clear();
        Check( ! CWebpImage::Decode( strAnimated, nWidth, nHeight, anPixels, strError ) && strError.find( "animated" ) != std::string::npos,
               "an animated WebP is refused, saying so" );
    }

        // Cut-off and damaged files fail cleanly (and, under the sanitizers,
        // never read past their data).
    {
        const char *astrAll[] = { "base444.jpg", "base420.jpg", "prog420.jpg", "gray.jpg", "restart.jpg", "exif6.jpg", "lossless.webp",
                                  "lossless_alpha.webp", "palette4.webp", "lossy.webp", "lossy_alpha.webp", "lossy_parts.webp" };
        bool        bCutOff = true, bWhole = true;
        uint32_t    nSeed = 12345;

        for ( const char *strName : astrAll )
        {
            std::string strData = ReadFile( strFolder + strName );

            for ( int nEighth = 1; nEighth < 8; nEighth++ )
            {
                int               nWidth = 0, nHeight = 0;
                std::vector<BYTE> anPixels;
                std::string       strError;

                if ( DecodeImageFile( strData.substr( 0, strData.size() * nEighth / 8 ), nWidth, nHeight, anPixels, strError ) )
                {
                    fprintf( stderr, "  %s cut to %d/8 decoded\n", strName, nEighth );
                    bCutOff = false;
                }
            }
            for ( int nTrial = 0; nTrial < 40 && strData.size() > 32; nTrial++ )
            {
                std::string       strDamaged = strData;
                int               nWidth = 0, nHeight = 0;
                std::vector<BYTE> anPixels;
                std::string       strError;

                for ( int nFlip = 0; nFlip < 4; nFlip++ )
                {
                    nSeed = nSeed * 1103515245u + 12345u;

                    size_t nAt = 16 + ( nSeed >> 8 ) % ( strDamaged.size() - 16 );

                    nSeed = nSeed * 1103515245u + 12345u;
                    strDamaged[ nAt ] = (char)( strDamaged[ nAt ] ^ ( 1 << ( ( nSeed >> 16 ) & 7 ) ) );
                }
                if ( DecodeImageFile( strDamaged, nWidth, nHeight, anPixels, strError ) && anPixels.size() != (size_t)nWidth * nHeight * 3 )
                    bWhole = false;
            }
        }
        Check( bCutOff, "a JPEG or WebP cut short is reported, not decoded" );
        Check( bWhole, "a damaged JPEG or WebP decodes to whole pixels or is reported" );
    }

        // Image textures read JPEG and WebP by their contents, whatever their names.
    {
        CImageTexture::SImage oImage;
        std::string           strError;
        int                   nWidth = 0, nHeight = 0;
        std::vector<BYTE>     anPixels;

        Check( CImageTexture::ParseImage( ReadFile( strFolder + "lossless.webp" ), "image.png", oImage, strError ) &&
               oImage.nWidth == 130 && oImage.nHeight == 97, "an image texture reads a WebP by its contents" );
        Check( CJpegImage::Decode( ReadFile( strFolder + "exif6.jpg" ), nWidth, nHeight, anPixels, strError ) && nWidth == 45 && nHeight == 61,
               "a JPEG with Exif orientation 6 comes out turned upright (45 x 61)" );
        Check( CImageTexture::ParseImage( ReadFile( strFolder + "prog420.jpg" ), "image.webp", oImage, strError ) &&
               oImage.nWidth == 130 && oImage.nHeight == 97, "an image texture reads a progressive JPEG" );
    }
}

    // 2026-alpha-0.21.3: model parts and materialMap, creaseAngle, nodeFrame,
    // faceColors' "sample": "uv", object names, --save-object and include,
    // and the LM-5 Eagle model's parts.
static void ModelPartChecks( const char *strScenes, const char *strDir )
{
    std::string             strModels = std::string( strScenes ) + "/models/";
    RayJson::CBaseDirectory oBase( strScenes );
    std::string             strError;

    auto Mesh = [ & ]( const std::string &strJson, BOOL &bRead ) -> std::unique_ptr<CMesh>
    {
        std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( strJson ) );
        std::unique_ptr<CMesh>             pMesh( new CMesh );

        bRead = ( pNode && pMesh->FromJson( *pNode ) ) ? TRUE : FALSE;
        if ( ! bRead )
        {
            strError = RayJson::GetError();
            RayJson::ClearError();
        }
        return pMesh;
    };
    BOOL bRead = FALSE;

        // Parts: each face's glTF material, by name.
    {
        ModelFile::SModel oModel;
        std::string       strData = ReadFile( strModels + "apollo/apollo_lunar_module.glb" );

        Check( ModelFile::ReadGltf( strData, NULL, "", "", oModel, strError ) == TRUE, "NASA's LM reads" );
        Check( oModel.anFacePart.size() == oModel.anFaces.size() / 3 && oModel.astrParts.size() == 12,
               "each face of the LM has its part, one of its 12 materials" );
        Check( std::find( oModel.astrParts.begin(), oModel.astrParts.end(), "blinn5SG.002" ) != oModel.astrParts.end(),
               "the parts are named by the glTF materials" );
        Check( ! oModel.apImages.empty() && oModel.anFaceImage.size() == oModel.anFaces.size() / 3 &&
               oModel.anFaceImageUVs.size() == oModel.anFaces.size() * 2, "the base color images and each face's image coordinates are kept" );
    }

        // A materialMap gives a part its own materials and bump; a name the
        // model doesn't have is refused, listing the parts.
    {
        std::unique_ptr<CMesh> pMesh = Mesh( "{\"file\": \"models/apollo/apollo_lunar_module.glb\", \"materialMap\": {"
                                             "\"blinn5SG.002\": {\"materials\": [{\"type\": \"diffuse\", \"influence\": 1, \"color\": [1, 0, 0]}],"
                                             " \"bump\": {\"texture\": {\"type\": \"noise\"}}}}}", bRead );
        const CMesh::SGeometry *pGeometry = pMesh->GetGeometry();
        int                     nBell = -1, nOther = -1, nPart = -1;
        std::vector<std::string> astrParts = pMesh->GetPartNames();

        Check( bRead == TRUE, "a mesh with a materialMap reads" );
        for ( size_t i = 0; i < astrParts.size(); i++ )
        {
            if ( astrParts[ i ] == "blinn5SG.002" )
                nPart = (int)i;
        }
        for ( size_t i = 0; pGeometry && i < pGeometry->anFacePart.size(); i++ )
        {
            if ( pGeometry->anFacePart[ i ] == nPart && nBell < 0 )
                nBell = (int)i;
            if ( pGeometry->anFacePart[ i ] != nPart && nOther < 0 )
                nOther = (int)i;
        }
        Check( nBell >= 0 && nOther >= 0 && pMesh->FaceHasBump( nBell ) && ! pMesh->FaceHasBump( nOther ),
               "a part's faces take its entry's bump, the others the mesh's (none)" );

        std::unique_ptr<IASLib::CObjectNode> pOut( new IASLib::CObjectNode );

        pMesh->ToJson( *pOut );
        Check( pOut->get( "materialMap" ) && pOut->get( "materialMap" )->get( "blinn5SG.002" ), "the materialMap is saved" );

        Mesh( "{\"file\": \"models/apollo/apollo_lunar_module.glb\", \"materialMap\": {\"hull\": {\"materials\": []}}}", bRead );
        Check( bRead == FALSE && strError.find( "has no part named \"hull\"" ) != std::string::npos &&
               strError.find( "blinn5SG.002" ) != std::string::npos, "a part the model doesn't have is refused, naming its parts" );
        Mesh( "{\"vertices\": [[0,0,0],[1,0,0],[0,1,0]], \"faces\": [[0,1,2]], \"materialMap\": {}}", bRead );
        Check( bRead == FALSE, "a materialMap is for a model file" );
        Mesh( "{\"file\": \"models/apollo/apollo_lunar_module.glb\", \"materialMap\": {\"blinn5SG.002\": {\"bump\": {}}}}", bRead );
        Check( bRead == FALSE && strError.find( "needs \"materials\"" ) != std::string::npos, "an entry needs materials" );
    }

        // creaseAngle: normals worked out from the faces. A cube's corners
        // stay sharp under 90 degrees and round past it.
    {
        std::unique_ptr<CMesh> pSharp = Mesh( "{\"file\": \"models/color_cube.stl\", \"smooth\": true, \"creaseAngle\": 30}", bRead );
        BOOL                   bSharp = bRead;
        std::unique_ptr<CMesh> pRound = Mesh( "{\"file\": \"models/color_cube.stl\", \"smooth\": true, \"creaseAngle\": 100}", bRead );
        BOOL                   bAllFace = TRUE, bAnyBent = FALSE;

        Check( bSharp && bRead, "creaseAngle reads" );
        const CMesh::SGeometry &oSharp = *pSharp->GetGeometry();
        const CMesh::SGeometry &oRound = *pRound->GetGeometry();

        for ( int f = 0; f < oSharp.FaceCount(); f++ )
        {
            CVector oFace = oSharp.aoFaceNormals[ (size_t)f ];

            for ( int c = 0; c < 3; c++ )
            {
                CVector oN = oSharp.aoVertexNormals[ (size_t)oSharp.anKept[ (size_t)f * 3 + c ] ];

                if ( fabs( oN.Dot( oFace ) - 1.0 ) > 1e-9 )
                    bAllFace = FALSE;
                oN = oRound.aoVertexNormals[ (size_t)oRound.anKept[ (size_t)f * 3 + c ] ];
                oFace = oRound.aoFaceNormals[ (size_t)f ];
                if ( oN.Dot( oFace ) < 0.9 )
                    bAnyBent = TRUE;
                oFace = oSharp.aoFaceNormals[ (size_t)f ];
            }
        }
        Check( bAllFace, "under the crease angle a cube's corners keep their faces' normals" );
        Check( bAnyBent, "past it they are blended with the faces around them" );
        Mesh( "{\"file\": \"models/color_cube.stl\", \"creaseAngle\": 200}", bRead );
        Check( bRead == FALSE, "a creaseAngle past 180 is refused" );
    }

        // nodeFrame: a picked node in its own frame, or where its parents put it.
    {
        std::unique_ptr<CMesh> pLocal = Mesh( "{\"file\": \"models/chess/chess_set_1k.gltf\", \"node\": \"piece_rook_white_01\"}", bRead );
        BOOL                   bLocal = bRead;
        std::unique_ptr<CMesh> pWorld = Mesh( "{\"file\": \"models/chess/chess_set_1k.gltf\", \"node\": \"piece_rook_white_01\", "
                                              "\"nodeFrame\": \"world\"}", bRead );

        Check( bLocal && bRead, "nodeFrame reads" );
        CVector oLocal = pLocal->GetGeometry()->oMinimum, oWorld = pWorld->GetGeometry()->oMinimum;

            // The rook's node moves it by (0.2026, 0.0174, -0.2026); glTF's z
            // is turned round in Ray.
        Check( fabs( ( oWorld.GetX() - oLocal.GetX() ) - 0.20260828733444214 ) < 1e-6 &&
               fabs( ( oWorld.GetY() - oLocal.GetY() ) - 0.017392655834555626 ) < 1e-6 &&
               fabs( ( oWorld.GetZ() - oLocal.GetZ() ) - 0.20260828733444214 ) < 1e-6,
               "\"nodeFrame\": \"world\" places a node by its own transform" );
        Mesh( "{\"file\": \"models/chess/chess_set_1k.gltf\", \"nodeFrame\": \"world\"}", bRead );
        Check( bRead == FALSE, "nodeFrame needs a node" );
    }

        // faceColors' "sample": "uv": a face of NASA's LM that has a base
        // color image samples it through the face's coordinates.
    {
        std::unique_ptr<CMesh>  pMesh = Mesh( "{\"file\": \"models/apollo/apollo_lunar_module.glb\"}", bRead );
        const CMesh::SGeometry &oGeometry = *pMesh->GetGeometry();
        CRay                    oRay;
        CVector                 oHit;
        BOOL                    bFlip = FALSE;
        int                     nFace = -1;

        for ( size_t i = 0; i < oGeometry.anFaceImage.size() && nFace < 0; i++ )
        {
            if ( oGeometry.anFaceImage[ i ] >= 0 )
                nFace = (int)i;
        }
        Check( nFace >= 0, "some of the LM's faces have an image" );
        if ( nFace < 0 )
            return;

        CVector oA = oGeometry.aoVertices[ (size_t)oGeometry.anKept[ (size_t)nFace * 3 ] ];
        CVector oB = oGeometry.aoVertices[ (size_t)oGeometry.anKept[ (size_t)nFace * 3 + 1 ] ];
        CVector oC = oGeometry.aoVertices[ (size_t)oGeometry.anKept[ (size_t)nFace * 3 + 2 ] ];
        CVector oN = oGeometry.aoFaceNormals[ (size_t)nFace ];
        FLT     nX = ( oA.GetX() + oB.GetX() + oC.GetX() ) / 3.0, nY = ( oA.GetY() + oB.GetY() + oC.GetY() ) / 3.0;
        FLT     nZ = ( oA.GetZ() + oB.GetZ() + oC.GetZ() ) / 3.0;

            // From just off the face's middle, straight at it.
        oRay.Set( nX + oN.GetX() * 1e-4, nY + oN.GetY() * 1e-4, nZ + oN.GetZ() * 1e-4, -oN.GetX(), -oN.GetY(), -oN.GetZ(), 1e100, 1.0, 0, 0 );
        Check( pMesh->Intersect( oRay, oHit ) == TRUE, "a ray at that face hits it" );

        CObjects *pFace = pMesh->HitPart( bFlip );
        CColor    oColor;

        Check( pFace && pFace->SurfaceImageColor( oHit, oColor ) && oColor.GetR() >= 0.0 && oColor.GetR() <= 1.0,
               "a face with an image samples it at the hit" );

        std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( "{\"type\": \"faceColors\", \"sample\": \"uv\"}" ) );
        std::unique_ptr<CTexture>          pTexture( CTexture::Read( *pNode ) );
        STexturePoint                      oAt;

        pFace->TextureAt( oHit, oAt );
        Check( pTexture && pTexture->ColorAt( oAt ).GetR() == oColor.GetR(), "faceColors with \"sample\": \"uv\" uses it" );
        pNode.reset( RayJson::Parse( "{\"type\": \"faceColors\", \"sample\": \"both\"}" ) );
        pTexture.reset( CTexture::Read( *pNode ) );
        RayJson::ClearError();
        Check( ! pTexture, "an unknown sample is refused" );
    }

        // Object names, --save-object and include.
    {
        CScene      oScene;
        std::string strPrefab = std::string( strDir ) + "/prefab_cube.ray";
        std::string strUse = std::string( strDir ) + "/use_prefab.ray";

        Check( oScene.Load( ( std::string( strScenes ) + "/models_test.ray" ).c_str() ) == 0, "models_test loads" );
        Check( oScene.FindObject( "2" ) == 1 && oScene.FindObject( "9" ) == -1, "an object is found by its number" );
        Check( oScene.SaveObject( 1, strPrefab.c_str(), TRUE ) == 0, "one object saves as a scene of its own" );

        CScene oPrefab;

        Check( oPrefab.Load( strPrefab.c_str() ) == 0 && oPrefab.GetObjectCount() == 1, "the prefab holds just that object" );

        FILE *pFile = fopen( strUse.c_str(), "wb" );

        fputs( "{\"format\": \"Ray scene\", \"version\": 3, \"camera\": {\"position\": [0, 2, -10], \"lookAt\": [0, 0, 0]},"
               " \"objects\": [{\"type\": \"include\", \"name\": \"cubes\", \"file\": \"prefab_cube.ray\", \"position\": [3, 0, 0]}],"
               " \"lights\": []}", pFile );
        fclose( pFile );

        CScene oUse;

        Check( oUse.Load( strUse.c_str() ) == 0 && oUse.GetObjectCount() == 2, "an include brings its object in after it" );
        if ( oUse.GetObjectCount() == 2 )
        {
            CMesh  *pOriginal = dynamic_cast<CMesh *>( oPrefab.GetObjectAt( 0 ) );
            CMesh  *pPlaced = dynamic_cast<CMesh *>( oUse.GetObjectAt( 1 ) );

            Check( pOriginal && pPlaced && fabs( CVector( pPlaced->GetGeometry()->oMinimum ).GetX() - CVector( pOriginal->GetGeometry()->oMinimum ).GetX() - 3.0 ) < 1e-9,
                   "the include's position moves what it brings in" );
            Check( oUse.GetObjectAt( 0 )->GetName() == "cubes" && oUse.FindObject( "cubes" ) == 0, "an object keeps its name" );
            Check( oUse.SaveObject( 1, strPrefab.c_str(), TRUE ) != 0, "an included object can't be saved alone" );
        }

        std::string strSaved = oUse.ToJson();

        Check( strSaved.find( "\"include\"" ) != std::string::npos && strSaved.find( "color_cube" ) == std::string::npos &&
               strSaved.find( "\"name\": \"cubes\"" ) != std::string::npos, "the scene saves the include, not what it brought in" );

        std::string strTask;
        {
            RayJson::CEmbedData oEmbed;

            strTask = oUse.ToJson( TRUE );
        }
        CScene oTask;

        Check( oTask.FromJson( strTask ) == 0 && oTask.GetObjectCount() == 2, "a task's copy carries the included objects inline" );
        Check( strTask.find( "\"objects\":[{\"type\":\"mesh\"" ) != std::string::npos, "a task's copy holds the mesh itself" );

            // An included scene in a folder of its own: its files go in an
            // archive under that folder, so extracting the archive puts them
            // beside it, where it looks for them.
        std::filesystem::create_directories( std::string( strDir ) + "/parts" );
        Check( oScene.SaveObject( 1, ( std::string( strDir ) + "/parts/cube.ray" ).c_str(), TRUE ) == 0, "a prefab saves in a folder" );

        std::string strNested = std::string( strDir ) + "/use_nested.ray";

        pFile = fopen( strNested.c_str(), "wb" );
        fputs( "{\"format\": \"Ray scene\", \"version\": 3, \"camera\": {\"position\": [0, 2, -10], \"lookAt\": [0, 0, 0]},"
               " \"objects\": [{\"type\": \"include\", \"file\": \"parts/cube.ray\"}], \"lights\": []}", pFile );
        fclose( pFile );

        CScene      oNested, oArchived, oExtracted;
        std::string strArchive = std::string( strDir ) + "/use_nested_archive.ray";
        std::string strOut = std::string( strDir ) + "/extracted/use.ray";

        std::filesystem::create_directories( std::string( strDir ) + "/extracted" );
        Check( oNested.Load( strNested.c_str() ) == 0 && oNested.Save( strArchive.c_str() ) == 0, "a scene with an include in a folder saves as an archive" );
        Check( oArchived.Load( strArchive.c_str() ) == 0 && oArchived.GetObjectCount() == 2, "the archive reads back" );
        Check( oArchived.SaveJson( strOut.c_str() ) == 0, "and extracts" );
        Check( std::filesystem::exists( std::string( strDir ) + "/extracted/parts/cube.ray" ) &&
               std::filesystem::exists( std::string( strDir ) + "/extracted/parts/models/color_cube.stl" ),
               "an included scene's files are extracted beside it" );
        Check( oExtracted.Load( strOut.c_str() ) == 0 && oExtracted.GetObjectCount() == 2, "the extracted scene reads" );
    }

        // The LM-5 Eagle: NASA's model, its parts named.
    {
        ModelFile::SModel oModel;
        std::string       strData = ReadFile( strModels + "apollo/lm5_eagle.glb" );
        const char       *astrNames[] = { "kapton_blanket", "kapton_gold", "silver_foil", "kapton_mesa", "black_blanket", "dps_bell",
                                          "porch_aluminum", "inconel_dark", "white_paint", "al2024_anodized", "al5056_anodized",
                                          "al_olive", "pyromark_black", "window_quartz" };
        BOOL              bAll = TRUE;

        Check( ModelFile::ReadGltf( strData, NULL, "", "", oModel, strError ) == TRUE, "lm5_eagle.glb reads" );
        for ( const char *strName : astrNames )
        {
            if ( std::find( oModel.astrParts.begin(), oModel.astrParts.end(), strName ) == oModel.astrParts.end() )
                bAll = FALSE;
        }
        Check( bAll && oModel.astrParts.size() == sizeof( astrNames ) / sizeof( astrNames[ 0 ] ), "every Eagle part is there by name" );
        Check( oModel.aoNormals.size() == oModel.aoVertices.size(), "the Eagle carries its worked-out normals" );
    }
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
        ModelPartChecks( argv[ 2 ], argv[ 3 ] );
        DracoChecks( argv[ 2 ] );
        BlendChecks( argv[ 2 ] );
        TextChecks( argv[ 2 ] );
        PatchFileChecks( argv[ 2 ] );
        ImageFormatChecks( argv[ 2 ] );
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
        IndirectChecks();
        HazeChecks();
        PresetChecks();
        CellChecks();
        LightOptionChecks();
        ModelChecks();
        SceneEditChecks();
        MapChecks();
        ShapeChecks();
        PatchChecks();
        LatheChecks();
        ExtrusionChecks();
        MathShapeChecks();
        ApolloChecks();
        ShowcaseChecks();
        LensElementsChecks();
    }

    if ( g_nFailures == 0 )
        printf( "All checks passed\n" );
    return ( g_nFailures == 0 ) ? 0 : 1;
}
