// Checks that CRayStream writes exactly the bytes the 1997 CFileStream did,
// that JSON scenes are read and checked properly, and that scene files
// survive a load and save unchanged.
//
//   format_test                   the byte-level format and JSON checks
//   format_test <scene.ray> <dir> load, save, load, save; the two saves must match
#include "Globals.h"
#include "RayStream.h"
#include "Scene.h"
#include "ReinhardColorModel.h"

#include "Streams/StringStream.h"

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

static std::string Bytes( const char *pData, size_t nLength )
{
    return std::string( pData, nLength );
}

static void FormatChecks( void )
{
    IASLib::CStringStream   oBuffer;
    CRayStream              oOut( oBuffer );

    oOut << (int)TRUE;                      // BOOL: one byte
    oOut << (char)'A';
    oOut << (short)-2;                      // two bytes, big-endian
    oOut << (unsigned short)0x1234;
    oOut << (long)-1;                       // four bytes whatever sizeof(long) is
    oOut << (DWORD)0x01020304;
    oOut << 1.0;                            // double as "%.18e" text
    oOut << "ray.out";

    std::string strExpected;
    strExpected += Bytes( "\x01", 1 );
    strExpected += Bytes( "A", 1 );
    strExpected += Bytes( "\xFF\xFE", 2 );
    strExpected += Bytes( "\x12\x34", 2 );
    strExpected += Bytes( "\xFF\xFF\xFF\xFF", 4 );
    strExpected += Bytes( "\x01\x02\x03\x04", 4 );
    strExpected += Bytes( "\x00\x18", 2 ) + "1.000000000000000000e+00";
    strExpected += Bytes( "\x00\x07", 2 ) + "ray.out";

    Check( oBuffer.GetOutput() == strExpected, "values are written in the 1997 file format" );

    IASLib::CStringStream   oInBuffer( oBuffer.GetOutput().data(), oBuffer.GetOutput().size() );
    CRayStream              oIn( oInBuffer );
    int                     bValue;
    char                    chValue;
    short                   nShort;
    unsigned short          nUShort;
    long                    nLong;
    DWORD                   nDword;
    double                  dValue;
    IASLib::CString         strValue;

    oIn >> bValue >> chValue >> nShort >> nUShort >> nLong >> nDword >> dValue >> strValue;

    Check( bValue == TRUE, "BOOL reads back" );
    Check( chValue == 'A', "char reads back" );
    Check( nShort == -2, "short reads back" );
    Check( nUShort == 0x1234, "unsigned short reads back" );
    Check( nLong == -1, "long reads back, sign included" );
    Check( nDword == 0x01020304, "DWORD reads back" );
    Check( dValue == 1.0, "double reads back" );
    Check( strcmp( (const char *)strValue, "ray.out" ) == 0, "string reads back" );
    Check( ! oIn.DataAvailable(), "nothing is left over" );

        // Doubles must survive the text form exactly.
    const double adValues[] = { 0.1, -3.141592653589793, 1e-300, 6.02214076e23, 2.0 / 3.0 };
    for ( double dOriginal : adValues )
    {
        IASLib::CStringStream   oRound;
        CRayStream              oWrite( oRound );

        oWrite << dOriginal;

        IASLib::CStringStream   oRead( oRound.GetOutput().data(), oRound.GetOutput().size() );
        CRayStream              oReadBack( oRead );
        double                  dRead = 0.0;

        oReadBack >> dRead;
        Check( dRead == dOriginal, "a double round-trips through its text form" );
    }

        // The pre-April-1997 layout: a whole part and a fraction of 2^31.
    {
        std::string             strLegacy = Bytes( "\xFF\xFF\xFF\xFC\x00\x00\x00\x00\x00\x00\x00\x00\x01\x47\xAE\x14", 16 );
        IASLib::CStringStream   oLegacyBuffer( strLegacy.data(), strLegacy.size() );
        CRayStream              oLegacy( oLegacyBuffer );
        double                  dFirst, dSecond;

        oLegacy.SetLegacyDoubles( TRUE );
        oLegacy >> dFirst >> dSecond;
        Check( dFirst == -4.0, "a legacy double with a negative whole part" );
        Check( dSecond > 0.00999 && dSecond < 0.01001, "a legacy double with a fraction" );
    }
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
        FormatChecks();
        JsonChecks();
    }

    if ( g_nFailures == 0 )
        printf( "All checks passed\n" );
    return ( g_nFailures == 0 ) ? 0 : 1;
}
