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
#include "Scene.h"
#include "ReinhardColorModel.h"


#include <stdio.h>
#include <string.h>

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

    CScene  oAgain;
    Check( oAgain.Load( strFirst.c_str() ) == 0, "the saved scene loads" );
    Check( oAgain.Save( strSecond.c_str() ) == 0, "the reloaded scene saves" );

    std::string strData = ReadFile( strFirst );
    Check( ! strData.empty() && strData == ReadFile( strSecond ), "saving is stable across a reload" );

    remove( strFirst.c_str() );
    remove( strSecond.c_str() );
}

int main( int argc, char **argv )
{
    if ( argc >= 3 )
        RoundTrip( argv[ 1 ], argv[ 2 ] );
    else
    {
        WireChecks();
        JsonChecks();
        TriangleChecks();
    }

    if ( g_nFailures == 0 )
        printf( "All checks passed\n" );
    return ( g_nFailures == 0 ) ? 0 : 1;
}
