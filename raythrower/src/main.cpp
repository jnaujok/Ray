#ifndef __THROWER__
#pragma message( "ERROR! This file should only be included in the Ray Thrower." )
#endif

#include "Globals.h"
#include "Scene.h"
#include "FileStream.h"
#include "PVMStream.h"
#include "Rays.h"
#include "RayArray.h"
#include "CString.h"
#include "ImageFile.h"
#include "MsgTag.h"
#include "pvm3.h"
#include <stdio.h>

#define RAYS_PER_PACKET     16

void ShowUsage( void )
{
    printf( "Correct Usage for the Distributed Ray Tracer - Ray Thrower:\n\n" );
    printf( "    RAYTHROW <FileToTrace> [-Nxxx]\n\n" );
    printf( "Where FileToTrace is the fully qualified file name of the scene\n" );
    printf( "to be traced, and the optional -N parameter is the number of\n" );
    printf( "tasks to be spawned under PVM.\n" );
}

BOOL ParseCommands( CString &oString, DWORD &nNumberOfTasks, int nArgs, char **astrArgs )
{
    BOOL    bRetVal = FALSE;
    int     nCount;
    CString strWork;

    nNumberOfTasks = 1;

    nCount = 1;

    while ( nArgs > nCount )
    {
        strWork = astrArgs[ nCount ];

        strWork.Trim();

        if ( strWork[0] == '-' )
        {
            if ( ( strWork[1] == 'n' ) || ( strWork[1] == 'N' ) )
            {
                CString     strTasks;

                strTasks = strWork.Right( strWork.GetLength() - 2 );
                strTasks.Trim();
                nNumberOfTasks = strTasks.GetValue();
                if ( nNumberOfTasks <= 0 )
                    nNumberOfTasks = 1;
            }
        }
        else
        {
            oString = strWork;
            bRetVal = TRUE;
        }

        nCount++;
    }    
    return bRetVal;
}

DWORD CheckReturn( CImageFile &oImage, int nChildTaskID )
{
    DWORD           nTotalPixels = 0;
    DWORD           nPixels;
    DWORD           nCount = 0;
    CPVMStream     *poStream;
    CColor          oColor;
    DWORD           nX, nY;

    while ( pvm_probe( nChildTaskID, MESSAGE_COLOR ) )
    {
        poStream = new CPVMStream( nChildTaskID, MESSAGE_COLOR, FALSE );

        *poStream >> nPixels;

        while ( nCount < nPixels )
        {
            *poStream >> (DWORD)nX;
            *poStream >> (DWORD)nY;
            *poStream << oColor;
            oImage.PutPoint( (int)nX, (int)nY, oColor );
            nCount++;
        }
        delete poStream;
        nTotalPixels += nPixels;
    }
    return nTotalPixels;
}

int main( int argc, char **argv )
{
    CScene          oScene;
    CRay            oRay;
    CRayArray       oRayArray;
    CStream        *poInputStream;
    CPVMStream     *poStream;
    int             nTaskID = 0;
    int             nMsgID = 0;
    BOOL            bDone = FALSE;
    CColor          oColorRet;
    DWORD           nRaysTraced = 0;
    CImageFile     *poImageFile;
    DWORD           nTasksToSpawn;
    CString         strFileToTrace;
    int             nMyTaskID = pvm_mytid();
    int             nMyParentID = pvm_parent();
    time_t          nStart;
    DWORD           nPixelCount;
    int            *anChildTasks;
    int             nCurrentTask = 0;
    int             nTasksBuilt;


#ifdef _DEBUG
    printf( "Thrower started\n" );
#endif

    time( &nStart );

    if ( nMyTaskID < 0 )
    {
        printf( "PVM Returned error code: %d\n", nMyTaskID );
        while ( ( time( NULL ) - nStart ) < 5 );
        exit( 0 );
    }

    if ( nMyParentID != PvmNoParent )
    {
        printf( "The ray thrower is a PVM master program. It should not be started\n" );
        printf( "as a PVM client.\n\n" );
        while ( ( time( NULL ) - nStart ) < 5 );
        exit( 0 );
    }

    if ( ! ParseCommands( strFileToTrace, nTasksToSpawn, argc, argv ) )
    {
        ShowUsage();
        return 1;
    }

    anChildTasks = new int[ nTasksToSpawn ];
    nTasksBuilt = pvm_spawn( "raythrower", NULL, PvmTaskDefault, "", nTasksToSpawn, anChildTasks );
    if ( nTasksBuilt <= 0 )
    {
        printf( "Couldn't spawn tasks.\nPVM Returned error code: %d\n", nTasksBuilt );
        while ( ( time( NULL ) - nStart ) < 5 );
        exit( 0 );
    }

    poInputStream = new CFileStream( strFileToTrace );

    oScene.Serialize( *poInputStream, TRUE );

    delete poInputStream;

#ifdef _DEBUG
    printf( "Scene Loaded from %s\n", strFileToTrace );
#endif

    poStream = new CPVMStream( anChildTasks, nTasksBuilt, MESSAGE_SCENE );

    oScene.Serialize( *poStream, FALSE );

    poStream->Flush();

    delete poStream;

#ifdef _DEBUG
    printf( "Scene multicast to %d children\n", nTasksBuilt );
#endif

    CSettings      &oSettings = oScene.GetSettings();
    CCamera        &oCamera = oScene.GetCamera();

    poImageFile = new CImageFile( oSettings.GetImageFileName(), oSettings.GetImageWidth(), oSettings.GetImageHeight(), FALSE );

    nPixelCount = (DWORD)oSettings.GetImageHeight() * (DWORD)oSettings.GetImageWidth();

    {
        FLT nWidth = (FLT)oSettings.GetImageWidth();
        FLT nHeight = (FLT)oSettings.GetImageHeight();
        FLT nXCenter = nWidth / 2;
        FLT nYCenter = nHeight / 2;

            // Current Trace Point;
        FLT nCurrentX = -nXCenter;
        FLT nCurrentY = nYCenter;
        FLT nXRestart;

            // Angular step per pixel/line
        FLT nXStep = oCamera.GetHorizontalAngle() / nWidth;
        FLT nYStep = oCamera.GetVerticalAngle() / nHeight;

            // Current Angular Position for 0, 0 position
        FLT nXAngle;
        FLT nYAngle;

            // Pixel Iterators
        int     nX;
        int     nY;

            // The Ray we're casting
        CRay    oRay;

            // Camera information
        CVector oCamPos = oCamera.GetPosition();
        CVector oCamPoint = oCamera.PointAt() - oCamPos;

        int         nYStop = oSettings.GetImageHeight();
        int         nXStart = 0;
        int         nXStop = oSettings.GetImageWidth();
        int         nYStart = 0;

        nXRestart = nCurrentX;

        nY = nYStart;
        while ( nY < nYStop )
        {
            nX = nXStart;
            while ( nX < nXStop )
            {
                nXAngle = nCurrentX * nXStep;
                nYAngle = nCurrentY * nYStep;

                oRay.CreateRay( oScene, nXAngle, nYAngle, 1.0, nX, nY );
                oRayArray.Add( oRay );

                if ( oRayArray.GetSize() > RAYS_PER_PACKET )
                {
                    poStream = new CPVMStream( anChildTasks[ nCurrentTask ],MESSAGE_RAYS, TRUE );
                    *poStream << oRayArray;
                    delete poStream;
                    nPixelCount -= CheckReturn( *poImageFile, anChildTasks[ nCurrentTask ] );
                    nCurrentTask++;
                    nCurrentTask = nCurrentTask % nTasksBuilt;
                    oRayArray.RemoveAll();
                }

                nCurrentX += 1.0;
                nX++;
            }
            nCurrentX = nXRestart;
            nCurrentY -= 1.0;
            nY++;
        }
    }

    while ( nPixelCount )
    {
        nPixelCount -= CheckReturn( *poImageFile, anChildTasks[ nCurrentTask ] );
        nCurrentTask++;
        nCurrentTask = nCurrentTask % nTasksBuilt;
    }

#ifdef _DEBUG
    printf( "Total Pixels Traced: %ld\n", (DWORD)oSettings.GetImageHeight() * (DWORD)oSettings.GetImageWidth() );
    printf( "Time to trace: %ld\n", (DWORD)(time(NULL)-nStart) );
    printf( "Pixels per Second: %ld\n", (DWORD)(((DWORD)oSettings.GetImageHeight() * (DWORD)oSettings.GetImageWidth() ) / (time(NULL)-nStart)) );
#endif

    delete poImageFile;

    return 0;
}
