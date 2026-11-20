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
#include "ThreadTaskMaster.h"
#include "SocketTaskMaster.h"
#include "WireProtocol.h"
#include "PixelSampler.h"
#include "ColorModel.h"
#include "SceneJson.h"

#include "Exceptions/Exception.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if ! defined( _WIN32 )
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace
{
    struct SOptions
    {
        std::string strScene;
        std::string strOutput;
        std::string strPpm;
        std::string strEngine;
        std::string strSampler;         // Empty: what the scene says.
        std::string strColorModel;      // Empty: what the scene says.
        std::string strSave;
        int         nTasks = 0;
        int         nPort = RAY_WIRE_DEFAULT_PORT;
        int         nTimeout = 60;
        int         nBatch = 0;
        int         nWidth = 0;         // 0: the scene's size
        int         nHeight = 0;
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
        printf( "  --size <width>x<height>     Image size, instead of the scene's\n" );
        printf( "  --aa <sampler>              How each pixel is sampled (anti-aliasing), instead\n" );
        printf( "                              of what the scene says. Settings follow a colon:\n" );
        printf( "                              --aa random:samples=9,seed=2\n" );

        int                             nTypes;
        const CPixelSampler::SType     *aoTypes = CPixelSampler::GetTypes( nTypes );
        for ( int nIndex = 0; nIndex < nTypes; nIndex++ )
            printf( "                                %-9s %s%s\n", aoTypes[ nIndex ].strName, aoTypes[ nIndex ].strDescription,
                    ( nIndex == 0 ) ? " (default)" : "" );

        printf( "  --color <model>             How pixel colors become bytes, instead of what the\n" );
        printf( "                              scene says, with settings as for --aa:\n" );

        const CColorModel::SType       *aoModels = CColorModel::GetTypes( nTypes );
        for ( int nIndex = 0; nIndex < nTypes; nIndex++ )
            printf( "                                %-9s %s%s\n", aoModels[ nIndex ].strName, aoModels[ nIndex ].strDescription,
                    ( nIndex == 0 ) ? " (default)" : "" );
        printf( "  --save <file>               Save the scene, with any --aa or --color applied, and\n" );
        printf( "                              exit without tracing\n" );
        printf( "  -q, --quiet                 Print only errors\n" );
        printf( "  -h, --help                  Show this help\n" );
    }

        // A sampler or color model from "name" or "name:key=value,...", the
        // settings as the scene file would give them. NULL, with a message in
        // strError, if the name or a setting is not right.
    template <class T> T *MakeModel( const std::string &strSpec, std::string &strError )
    {
        size_t              nColon = strSpec.find( ':' );
        std::string         strName = strSpec.substr( 0, nColon );
        std::unique_ptr<T>  pModel( T::Create( strName.c_str() ) );

        if ( ! pModel )
        {
            strError = "there is none called \"" + strName + "\"";
            return NULL;
        }
        if ( nColon == std::string::npos )
            return pModel.release();

        IASLib::CObjectNode oSettings;
        std::string         strRest = strSpec.substr( nColon + 1 );

        oSettings.put( "type", strName.c_str() );
        while ( ! strRest.empty() )
        {
            size_t      nComma = strRest.find( ',' );
            std::string strPair = strRest.substr( 0, nComma );
            size_t      nEquals = strPair.find( '=' );

            strRest = ( nComma == std::string::npos ) ? "" : strRest.substr( nComma + 1 );
            if ( nEquals == std::string::npos || nEquals == 0 )
            {
                strError = "settings are given as key=value, not \"" + strPair + "\"";
                return NULL;
            }

            std::string strKey = strPair.substr( 0, nEquals );
            std::string strValue = strPair.substr( nEquals + 1 );
            char       *pEnd = NULL;
            double      dValue = strtod( strValue.c_str(), &pEnd );

            if ( strValue == "true" || strValue == "false" )
                oSettings.put( strKey, strValue == "true" );
            else if ( ! strValue.empty() && *pEnd == '\0' )
            {
                if ( dValue == (double)(long long)dValue && strValue.find_first_of( ".eE" ) == std::string::npos )
                    oSettings.put( strKey, (long long)dValue );
                else
                    oSettings.put( strKey, dValue );
            }
            else
                oSettings.put( strKey, strValue.c_str() );
        }

        RayJson::ClearError();
        if ( ! pModel->FromJson( oSettings ) )
        {
            strError = RayJson::GetError();
            RayJson::ClearError();
            return NULL;
        }
        return pModel.release();
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
            else if ( strArg == "--aa" || strArg == "--antialias" )
            {
                std::string                     strError;
                std::unique_ptr<CPixelSampler>  pSampler( strNext ? MakeModel<CPixelSampler>( strNext, strError ) : NULL );

                if ( ! pSampler )
                {
                    fprintf( stderr, "%s needs a sampler%s%s; see --help\n", strArg.c_str(),
                             strError.empty() ? "" : ": ", strError.c_str() );
                    return false;
                }
                oOptions.strSampler = strNext;
                nCount++;
            }
            else if ( strArg == "--color" )
            {
                std::string                     strError;
                std::unique_ptr<CColorModel>    pModel( strNext ? MakeModel<CColorModel>( strNext, strError ) : NULL );

                if ( ! pModel )
                {
                    fprintf( stderr, "%s needs a color model%s%s; see --help\n", strArg.c_str(),
                             strError.empty() ? "" : ": ", strError.c_str() );
                    return false;
                }
                oOptions.strColorModel = strNext;
                nCount++;
            }
            else if ( strArg == "--listen" )
            {
                oOptions.bListen = true;
                if ( strNext && strNext[ 0 ] != '-' && ParseInt( strNext, oOptions.nPort ) )
                    nCount++;
            }
            else if ( strArg == "--size" )
            {
                int nWidth = 0, nHeight = 0;
                char chExtra;

                if ( strNext == NULL || sscanf( strNext, "%dx%d%c", &nWidth, &nHeight, &chExtra ) != 2 ||
                     nWidth < 1 || nHeight < 1 || nWidth > 65535 || nHeight > 65535 )
                {
                    fprintf( stderr, "--size needs <width>x<height>, such as 640x480\n" );
                    return false;
                }
                oOptions.nWidth = nWidth;
                oOptions.nHeight = nHeight;
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
            else if ( strArg == "-o" || strArg == "--output" || strArg == "--ppm" || strArg == "--engine" || strArg == "--save" )
            {
                if ( strNext == NULL )
                {
                    fprintf( stderr, "%s needs a value\n", strArg.c_str() );
                    return false;
                }
                std::string &strTarget = ( strArg == "--ppm" ) ? oOptions.strPpm :
                                         ( strArg == "--save" ) ? oOptions.strSave :
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

        // Local rayengine processes started by --spawn. They are started
        // directly (posix_spawn or CreateProcess), not through system():
        // macOS runs system() calls one at a time, so only one engine ran.
    class CEngineProcesses
    {
        protected:
#if defined( _WIN32 )
            std::vector<HANDLE> m_aProcesses;
#else
            std::vector<pid_t>  m_aProcesses;
#endif

        public:
            ~CEngineProcesses( void ) { WaitAll(); }

            bool Start( const std::string &strEngine, int nPort, int nCount )
            {
                char achPort[ 16 ];

                snprintf( achPort, sizeof( achPort ), "%d", nPort );

                for ( int nEngine = 0; nEngine < nCount; nEngine++ )
                {
#if defined( _WIN32 )
                    std::string         strCommand = "\"" + strEngine + "\" 127.0.0.1 " + achPort + " --retry 30";
                    STARTUPINFOA        stStartup;
                    PROCESS_INFORMATION stProcess;

                    memset( &stStartup, 0, sizeof( stStartup ) );
                    stStartup.cb = sizeof( stStartup );
                    memset( &stProcess, 0, sizeof( stProcess ) );

                    std::vector<char> achCommand( strCommand.begin(), strCommand.end() );
                    achCommand.push_back( '\0' );

                    if ( ! CreateProcessA( NULL, achCommand.data(), NULL, NULL, FALSE, 0, NULL, NULL,
                                           &stStartup, &stProcess ) )
                    {
                        fprintf( stderr, "Couldn't start %s (error %lu)\n", strEngine.c_str(), (unsigned long)GetLastError() );
                        return false;
                    }
                    CloseHandle( stProcess.hThread );
                    m_aProcesses.push_back( stProcess.hProcess );
#else
                    std::string strHost = "127.0.0.1";
                    std::string strRetry = "--retry";
                    std::string strSeconds = "30";
                    char       *astrArgs[] = { const_cast<char *>( strEngine.c_str() ), &strHost[ 0 ], achPort,
                                               &strRetry[ 0 ], &strSeconds[ 0 ], NULL };
                    pid_t       nPid;
                    int         nError;

                        // posix_spawnp searches PATH when the name has no '/'.
                    nError = posix_spawnp( &nPid, strEngine.c_str(), NULL, NULL, astrArgs, environ );
                    if ( nError != 0 )
                    {
                        fprintf( stderr, "Couldn't start %s: %s\n", strEngine.c_str(), strerror( nError ) );
                        return false;
                    }
                    m_aProcesses.push_back( nPid );
#endif
                }
                return true;
            }

                /// Waits for every engine to exit.
            void WaitAll( void )
            {
#if defined( _WIN32 )
                for ( HANDLE hProcess : m_aProcesses )
                {
                    WaitForSingleObject( hProcess, INFINITE );
                    CloseHandle( hProcess );
                }
#else
                for ( pid_t nPid : m_aProcesses )
                {
                    int nStatus;
                    while ( waitpid( nPid, &nStatus, 0 ) < 0 && errno == EINTR )
                        ;
                }
#endif
                m_aProcesses.clear();
            }
    };

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
        fprintf( stderr, "Can't read %s: %s\n", oOptions.strScene.c_str(), oScene.GetLoadError() );
        return 1;
    }

    if ( ! oOptions.bQuiet )
        printf( "Scene loaded from %s\n", oOptions.strScene.c_str() );

    if ( ! oOptions.strSampler.empty() )
    {
        std::string strError;
        oScene.GetCamera().SetSampler( MakeModel<CPixelSampler>( oOptions.strSampler, strError ) );
    }
    if ( ! oOptions.strColorModel.empty() )
    {
        std::string strError;
        oScene.GetCamera().SetColorModel( MakeModel<CColorModel>( oOptions.strColorModel, strError ) );
    }

    if ( oOptions.nWidth > 0 )
    {
        oScene.GetSettings().SetImageWidth( oOptions.nWidth );
        oScene.GetSettings().SetImageHeight( oOptions.nHeight );
    }

    if ( ! oOptions.strSave.empty() )
    {
        if ( oScene.Save( oOptions.strSave.c_str() ) != 0 )
        {
            fprintf( stderr, "Couldn't write %s\n", oOptions.strSave.c_str() );
            return 1;
        }
        if ( ! oOptions.bQuiet )
            printf( "Scene saved to %s\n", oOptions.strSave.c_str() );
        return 0;
    }

    CSettings      &oSettings = oScene.GetSettings();
    std::string     strImage = oOptions.strOutput.empty() ? std::string( oSettings.GetImageFileName() ) : oOptions.strOutput;

    if ( strImage.empty() )
        strImage = "ray.out";

        // Set up the tasks.
    std::unique_ptr<CTaskMaster>    pMaster;
    CEngineProcesses                oEngines;

    if ( oOptions.bListen )
    {
        CSocketTaskMaster *pSockets = new CSocketTaskMaster( oOptions.nPort, oOptions.nTimeout );

        pMaster.reset( pSockets );
        if ( ! pSockets->Listen() )
        {
            fprintf( stderr, "Couldn't listen on port %d: %s\n", pSockets->GetPort(), pSockets->GetLastError() );
            return 1;
        }

        if ( oOptions.bSpawn && ! oEngines.Start( EnginePath( oOptions, argv[ 0 ] ), pSockets->GetPort(), oOptions.nTasks ) )
        {
            pMaster.reset();
            oEngines.WaitAll();
            return 1;
        }
        else if ( ! oOptions.bQuiet )
            printf( "Waiting for %d rayengine(s) on port %d\n", oOptions.nTasks, pSockets->GetPort() );

        if ( ! pSockets->SpawnTasks( oOptions.nTasks ) )
        {
            fprintf( stderr, "Couldn't start the tasks: %s\n", pSockets->GetLastError() );
            pMaster.reset();
            oEngines.WaitAll();
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
            oEngines.WaitAll();
            return 1;
        }

        if ( ! oOptions.bQuiet && oScene.GetCamera().GetLens() )
            printf( "Lens: %s\n", oScene.GetCamera().GetLens()->Describe().c_str() );
        if ( ! oOptions.bQuiet )
            printf( "Tracing %dx%d with %d task(s), sampling %s, color model %s, into %s\n",
                    oSettings.GetImageWidth(), oSettings.GetImageHeight(), pMaster->GetTaskCount(),
                    oScene.GetCamera().GetSampler().GetName(), oScene.GetCamera().GetColorModel().GetName(),
                    strImage.c_str() );

        bTraced = pMaster->TraceScene( oScene, oImage );
    }
    auto tEnd = std::chrono::steady_clock::now();

    long long nRays = pMaster->GetRaysTraced();
    IASLib::CString strError;
    if ( oOptions.bListen )
        strError = static_cast<CSocketTaskMaster *>( pMaster.get() )->GetLastError();

    pMaster->EndTasks();
    pMaster.reset();
    oEngines.WaitAll();

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
        printf( "Rays Traced: %lld (%.2f per pixel)\n", nRays, nPixels ? (double)nRays / (double)nPixels : 0.0 );
        printf( "Time to trace: %.2f seconds\n", dSeconds );
        if ( dSeconds > 0.0 )
            printf( "Pixels per Second: %.0f\n", (double)nPixels / dSeconds );
    }

    return 0;
}
