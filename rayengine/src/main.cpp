#ifndef __ENGINE__
#pragma message( "ERROR! This file should only be included in the Ray Tracing Engine." )
#endif

#include "Globals.h"
#include "Scene.h"
#include "PVMStream.h"
#include "Rays.h"
#include "RayArray.h"
#include "MsgTag.h"
#include "pvm3.h"

#include <stdio.h>
#include <time.h>

int main( int argc, char **argv )
{
    CScene      oScene;
    CRayArray   oRays;
    CPVMStream *poStream;
    CPVMStream *poReturnStream;
    int         nTaskID = 0;
    int         nMsgID = 0;
    BOOL        bDone = FALSE;
    CColor      oColorRet;
    DWORD       nRaysTraced = 0;
    int         nCount;
    int         nMyTaskID = pvm_mytid();
    int         nMyParentID = pvm_parent();
    time_t      nStart;

    time( &nStart );

    if ( nMyTaskID < 0 )
    {
        printf( "PVM Returned error code: %d\n", nMyTaskID );
        while ( ( time( NULL ) - nStart ) < 5 );
        exit( 0 );
    }

    if ( nMyParentID == PvmNoParent )
    {
        printf( "The ray tracing engine is a PVM client. It should not be started\n" );
        printf( "outside of PVM.\n\n" );
        while ( ( time( NULL ) - nStart ) < 5 );
        exit( 0 );
    }

#ifdef _DEBUG
    printf( "Engine started\n" );
#endif

    poStream = new CPVMStream( nMyParentID, MESSAGE_SCENE, FALSE );

    oScene.Serialize( *poStream, TRUE );

    delete poStream;

#ifdef _DEBUG
    printf( "Scene Loaded\n" );
#endif

    while ( ! bDone )
    {
#ifdef _DEBUG
        printf( "Retrieving Rays\n" );
#endif
        poStream = new CPVMStream( nMyParentID, MESSAGE_RAYS, FALSE );

        *poStream >> oRays;

        poStream->Flush();

        delete poStream;

        if ( oRays.GetSize() == 0 )
        {
            bDone = TRUE;
            poReturnStream = NULL;
        }
        else
        {
            poReturnStream = new CPVMStream( nMyParentID, MESSAGE_COLOR, TRUE );
            *poReturnStream << (DWORD)oRays.GetSize();
        }
        
        nCount = 0;
        while ( nCount < oRays.GetSize() )
        {
            oScene.TraceRay( oRays[ nCount ], oColorRet, NULL );
            *poReturnStream << (DWORD)oRays[nCount].GetPixelX();
            *poReturnStream << (DWORD)oRays[nCount].GetPixelY();
            *poReturnStream << oColorRet;
            nCount++;
            nRaysTraced++;
        }
        if ( poReturnStream )
        {
            poReturnStream->Flush();
            delete poReturnStream;
        }
#ifdef _DEBUG
        printf( "Rays Traced\n" );
#endif
    }

#ifdef _DEBUG
    printf( "Total Rays Traced: %ld\n", nRaysTraced );
#endif

    pvm_exit();
    return 0;
}
