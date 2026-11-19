/*************************************************************************
*   raythrower                                                           *
*                                                                        *
*   Command-line master for the distributed ray tracer. Loads a .ray     *
*   scene and traces it, either with threads on this machine or with     *
*   rayengine processes that connect over TCP.                           *
*                                                                        *
* Copyright (c) 1997, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"
#include "Scene.h"
#include "ImageFile.h"
#include "Color.h"
#include "RayStream.h"
#include "ThreadTaskMaster.h"
#include "SocketTaskMaster.h"
#include "WireProtocol.h"

#include "Exceptions/Exception.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{
    struct SOptions
    {
        std::string strScene;
        std::string strOutput;
        std::string strPpm;
        std::string strEngine;
        int         nTasks = 0;
        int         nPort = RAY_WIRE_DEFAULT_PORT;
        int         nTimeout = 60;
        int         nBatch = 0;
        bool        bListen = false;
        bool        bSpawn = false;
        bool        bQuiet = false;
    };

    void ShowUsage( void )
    {
        printf( "Distributed Ray Tracer - Ray Thrower\n\n" );
        printf( "Usage: raythrower <FileToTrace> [options]\n\n" );
        printf( "  -N<count>, --tasks <count>  Number of tracing tasks (default: one per CPU)\n" );
        printf( "  -o, --output <file>         Image file to write (default: the one named in the scene)\n" );
        printf( "  --ppm <file>                Also write the image as a PPM file\n" );
        printf( "  --listen [port]             Trace with rayengine processes that connect on\n" );
        printf( "                              this port (default %d; 0 for any free port)\n", RAY_WIRE_DEFAULT_PORT );
        printf( "                              instead of threads\n" );
        printf( "  --spawn                     With --listen, start the rayengines on this machine\n" );
        printf( "  --engine <path>             The rayengine to start (default: next to raythrower)\n" );
        printf( "  --timeout <seconds>         How long to wait for engines to connect (default 60)\n" );
        printf( "  --batch <rays>              Rays per batch sent to a task (default 127)\n" );
        printf( "  -q, --quiet                 Print only errors\n" );
        printf( "  -h, --help                  Show this help\n" );
    }

    bool ParseInt( const char *strValue, int &nValue )
    {
        char   *pEnd = NULL;
        long    nParsed;

        if ( strValue == NULL || *strValue == '\0' )
            return false;
        nParsed = strtol( strValue, &pEnd, 10 );
        if ( *pEnd != '\0' || nParsed < 0 || nParsed > 1000000 )
            return false;
        nValue = (int)nParsed;
        return true;
    }

    bool ParseCommands( SOptions &oOptions, int nArgs, char **astrArgs )
    {
        for ( int nCount = 1; nCount < nArgs; nCount++ )
        {
            std::string strArg = astrArgs[ nCount ];
            const char *strNext = ( nCount + 1 < nArgs ) ? astrArgs[ nCount + 1 ] : NULL;

            if ( strArg == "-h" || strArg == "--help" )
                return false;
            else if ( strArg == "-q" || strArg == "--quiet" )
                oOptions.bQuiet = true;
            else if ( strArg == "--spawn" )
                oOptions.bSpawn = true;
            else if ( strArg == "--listen" )
            {
                oOptions.bListen = true;
                if ( strNext && strNext[ 0 ] != '-' && ParseInt( strNext, oOptions.nPort ) )
                    nCount++;
            }
            else if ( strArg == "--tasks" || strArg == "--timeout" || strArg == "--batch" )
            {
                int &nTarget = ( strArg == "--tasks" ) ? oOptions.nTasks :
                               ( strArg == "--timeout" ) ? oOptions.nTimeout : oOptions.nBatch;
                if ( ! ParseInt( strNext, nTarget ) )
                {
                    fprintf( stderr, "%s needs a number\n", strArg.c_str() );
                    return false;
                }
                nCount++;
            }
            else if ( strArg == "-o" || strArg == "--output" || strArg == "--ppm" || strArg == "--engine" )
            {
                if ( strNext == NULL )
                {
                    fprintf( stderr, "%s needs a value\n", strArg.c_str() );
                    return false;
                }
                std::string &strTarget = ( strArg == "--ppm" ) ? oOptions.strPpm :
                                         ( strArg == "--engine" ) ? oOptions.strEngine : oOptions.strOutput;
                strTarget = strNext;
                nCount++;
            }
            else if ( strArg.size() > 2 && strArg[ 0 ] == '-' && ( strArg[ 1 ] == 'n' || strArg[ 1 ] == 'N' ) )
            {
                    // The original -Nxxx form.
                if ( ! ParseInt( strArg.c_str() + 2, oOptions.nTasks ) )
                {
                    fprintf( stderr, "Bad task count: %s\n", strArg.c_str() );
                    return false;
                }
            }
            else if ( strArg[ 0 ] == '-' )
            {
                fprintf( stderr, "Unknown option: %s\n", strArg.c_str() );
                return false;
            }
            else
                oOptions.strScene = strArg;
        }

        if ( oOptions.bSpawn && ! oOptions.bListen )
            oOptions.bListen = true;

        return ! oOptions.strScene.empty();
    }

        // The rayengine next to this program, unless one was named.
    std::string EnginePath( const SOptions &oOptions, const char *strArgv0 )
    {
        if ( ! oOptions.strEngine.empty() )
            return oOptions.strEngine;

        std::string strSelf = strArgv0 ? strArgv0 : "";
        size_t      nSlash = strSelf.find_last_of( "/\\" );
        std::string strDir = ( nSlash == std::string::npos ) ? std::string() : strSelf.substr( 0, nSlash + 1 );

#if defined( _WIN32 )
        return strDir + "rayengine.exe";
#else
        return strDir.empty() ? std::string( "rayengine" ) : strDir + "rayengine";
#endif
    }

        // Starts nCount local rayengines. Each runs on a thread of its own
        // until it exits; the caller joins them.
    void SpawnEngines( std::vector<std::thread> &aEngines, const std::string &strEngine, int nPort, int nCount )
    {
        char        achPort[ 16 ];
        std::string strCommand;

        snprintf( achPort, sizeof( achPort ), "%d", nPort );
        strCommand = "\"" + strEngine + "\" 127.0.0.1 " + achPort + " --retry 30";
#if defined( _WIN32 )
            // cmd.exe drops the outer quotes when the command starts with one.
        strCommand = "\"" + strCommand + "\"";
#endif

        for ( int nEngine = 0; nEngine < nCount; nEngine++ )
            aEngines.emplace_back( [strCommand]() { int nResult = system( strCommand.c_str() ); (void)nResult; } );
    }

    bool WritePpm( const std::string &strRaw, const std::string &strPpm )
    {
        FILE           *pIn = fopen( strRaw.c_str(), "rb" );
        FILE           *pOut = NULL;
        unsigned char   achHeader[ 4 ];
        bool            bOk = false;

        if ( pIn == NULL )
            return false;

        if ( fread( achHeader, 1, 4, pIn ) == 4 )
        {
            int nWidth = achHeader[ 0 ] * 256 + achHeader[ 1 ];
            int nHeight = achHeader[ 2 ] * 256 + achHeader[ 3 ];
            std::vector<unsigned char> aPixels( (size_t)nWidth * (size_t)nHeight * 3 );

            if ( fread( aPixels.data(), 1, aPixels.size(), pIn ) == aPixels.size() )
            {
                pOut = fopen( strPpm.c_str(), "wb" );
                if ( pOut )
                {
                    fprintf( pOut, "P6\n%d %d\n255\n", nWidth, nHeight );
                    bOk = fwrite( aPixels.data(), 1, aPixels.size(), pOut ) == aPixels.size();
                    bOk = ( fclose( pOut ) == 0 ) && bOk;
                }
            }
        }
        fclose( pIn );
        return bOk;
    }
}

int main( int argc, char **argv )
{
    SOptions        oOptions;
    CScene          oScene;

    if ( ! ParseCommands( oOptions, argc, argv ) )
    {
        ShowUsage();
        return 1;
    }

    if ( oOptions.nTasks <= 0 )
    {
        oOptions.nTasks = (int)std::thread::hardware_concurrency();
        if ( oOptions.nTasks <= 0 )
            oOptions.nTasks = 1;
    }

        // Load the scene.
    DWORD nLoaded = oScene.Load( oOptions.strScene.c_str() );
    if ( nLoaded != 0 )
    {
        fprintf( stderr, "%s %s\n", oOptions.strScene.c_str(),
                 ( nLoaded == WRONG_VERSION ) ? "was saved by a newer version of the ray tracer"
                                              : "is not a scene file this program can read" );
        return 1;
    }

    if ( ! oOptions.bQuiet )
        printf( "Scene loaded from %s\n", oOptions.strScene.c_str() );

    CSettings      &oSettings = oScene.GetSettings();
    std::string     strImage = oOptions.strOutput.empty() ? std::string( oSettings.GetImageFileName() ) : oOptions.strOutput;

    if ( strImage.empty() )
        strImage = "ray.out";

        // Set up the tasks.
    std::unique_ptr<CTaskMaster>    pMaster;
    std::vector<std::thread>        aEngines;

    if ( oOptions.bListen )
    {
        CSocketTaskMaster *pSockets = new CSocketTaskMaster( oOptions.nPort, oOptions.nTimeout );

        pMaster.reset( pSockets );
        if ( ! pSockets->Listen() )
        {
            fprintf( stderr, "Couldn't listen on port %d: %s\n", pSockets->GetPort(), pSockets->GetLastError() );
            return 1;
        }

        if ( oOptions.bSpawn )
            SpawnEngines( aEngines, EnginePath( oOptions, argv[ 0 ] ), pSockets->GetPort(), oOptions.nTasks );
        else if ( ! oOptions.bQuiet )
            printf( "Waiting for %d rayengine(s) on port %d\n", oOptions.nTasks, pSockets->GetPort() );

        if ( ! pSockets->SpawnTasks( oOptions.nTasks ) )
        {
            fprintf( stderr, "Couldn't start the tasks: %s\n", pSockets->GetLastError() );
            pMaster.reset();
            for ( auto &oEngine : aEngines )
                oEngine.join();
            return 1;
        }
    }
    else
    {
        pMaster.reset( new CThreadTaskMaster );
        pMaster->SpawnTasks( oOptions.nTasks );
    }

    if ( oOptions.nBatch > 0 )
        pMaster->SetRaysPerPacket( oOptions.nBatch );

        // Trace.
    BOOL bTraced;
    auto tStart = std::chrono::steady_clock::now();
    {
        CImageFile oImage( strImage.c_str(), oSettings.GetImageWidth(), oSettings.GetImageHeight(), FALSE );

        if ( ! oImage.IsOpen() )
        {
            fprintf( stderr, "Couldn't create the image file %s\n", strImage.c_str() );
            pMaster.reset();
            for ( auto &oEngine : aEngines )
                oEngine.join();
            return 1;
        }

        if ( ! oOptions.bQuiet )
            printf( "Tracing %dx%d with %d task(s) into %s\n", oSettings.GetImageWidth(), oSettings.GetImageHeight(),
                    pMaster->GetTaskCount(), strImage.c_str() );

        bTraced = pMaster->TraceScene( oScene, oImage );
    }
    auto tEnd = std::chrono::steady_clock::now();

    IASLib::CString strError;
    if ( oOptions.bListen )
        strError = static_cast<CSocketTaskMaster *>( pMaster.get() )->GetLastError();

    pMaster->EndTasks();
    pMaster.reset();
    for ( auto &oEngine : aEngines )
        oEngine.join();

    if ( ! bTraced )
    {
        fprintf( stderr, "The trace did not finish%s%s\n", strError.GetLength() ? ": " : "", (const char *)strError );
        return 1;
    }

    if ( ! oOptions.strPpm.empty() && ! WritePpm( strImage, oOptions.strPpm ) )
    {
        fprintf( stderr, "Couldn't write %s\n", oOptions.strPpm.c_str() );
        return 1;
    }

    if ( ! oOptions.bQuiet )
    {
        double  dSeconds = std::chrono::duration<double>( tEnd - tStart ).count();
        long    nPixels = (long)oSettings.GetImageHeight() * (long)oSettings.GetImageWidth();

        printf( "Total Pixels Traced: %ld\n", nPixels );
        printf( "Time to trace: %.2f seconds\n", dSeconds );
        if ( dSeconds > 0.0 )
            printf( "Pixels per Second: %.0f\n", (double)nPixels / dSeconds );
    }

    return 0;
}
