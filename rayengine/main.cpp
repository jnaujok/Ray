/*************************************************************************
*   rayengine                                                            *
*                                                                        *
*   A tracing task for the distributed ray tracer. Connects to a         *
*   raythrower (or the scene editor) over TCP, receives the scene, and   *
*   traces the batches of rays it is sent until it is told to stop.      *
*                                                                        *
* Copyright (c) 1997, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"
#include "SocketTaskSlave.h"
#include "WireProtocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
    void ShowUsage( void )
    {
        printf( "Distributed Ray Tracer - Ray Engine\n\n" );
        printf( "Usage: rayengine <master-host> [port] [--retry <seconds>]\n\n" );
        printf( "Connects to a raythrower started with --listen (default port %d)\n", RAY_WIRE_DEFAULT_PORT );
        printf( "and traces until the master says the image is done. --retry keeps\n" );
        printf( "trying to connect for that long if the master is not listening yet.\n" );
    }

    bool ParsePort( const char *strValue, int &nPort )
    {
        char   *pEnd = NULL;
        long    nParsed = strtol( strValue, &pEnd, 10 );

        if ( *pEnd != '\0' || nParsed < 1 || nParsed > 65535 )
            return false;
        nPort = (int)nParsed;
        return true;
    }
}

int main( int argc, char **argv )
{
    const char *strHost = NULL;
    int         nPort = RAY_WIRE_DEFAULT_PORT;
    int         nRetry = 0;
    bool        bHavePort = false;

    for ( int nCount = 1; nCount < argc; nCount++ )
    {
        if ( strcmp( argv[ nCount ], "--retry" ) == 0 && nCount + 1 < argc )
            nRetry = atoi( argv[ ++nCount ] );
        else if ( strcmp( argv[ nCount ], "-h" ) == 0 || strcmp( argv[ nCount ], "--help" ) == 0 )
        {
            ShowUsage();
            return 0;
        }
        else if ( strHost == NULL )
            strHost = argv[ nCount ];
        else if ( ! bHavePort && ParsePort( argv[ nCount ], nPort ) )
            bHavePort = true;
        else
        {
            ShowUsage();
            return 1;
        }
    }

    if ( strHost == NULL )
    {
        ShowUsage();
        return 1;
    }

    CSocketTaskSlave oEngine;

    if ( ! oEngine.Connect( strHost, nPort, nRetry ) )
    {
        fprintf( stderr, "rayengine: %s\n", oEngine.GetLastError() );
        return 1;
    }

        // RunEngine is FALSE when no rays came at all, which is fine when
        // there were more engines than work; only a real error fails.
    if ( ! oEngine.RunEngine() && oEngine.GetLastError()[ 0 ] != '\0' )
    {
        fprintf( stderr, "rayengine: %s\n", oEngine.GetLastError() );
        return 1;
    }

#ifdef _DEBUG
    printf( "Total Rays Traced: %lu\n", oEngine.GetRaysTraced() );
#endif

    return 0;
}
