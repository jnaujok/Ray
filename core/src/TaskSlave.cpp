#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "RayArray.h"
#include "ColorArray.h"
#include "TaskSlave.h"

#ifdef RAY_GUI
#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

BOOL CTaskSlave::RunEngine( void )
{
    CScene          oScene;
    CRayArray       oRays( 128 );
    CColorArray     oColors;
    CColor          oColor;
    int             nCount;
    BOOL            bRetVal = FALSE;

    if ( SceneFromMaster( oScene ) )
    {
        while ( RaysFromMaster( oRays ) )
        {
            bRetVal = TRUE;
            if ( oRays.GetSize() )
            {
                nCount = 0;
                oColors.RemoveAll();

                while ( nCount < oRays.GetSize() )
                {
                    oScene.TraceRay( oRays[ nCount ], oColor, NULL );

                    oColors.Add( oColor, oRays[nCount].GetPixelX(), oRays[nCount].GetPixelY() );

                    nCount++;
                }

                ColorsToMaster( oColors );
            }
            oRays.RemoveAll();
        }
    }
    return bRetVal;
}

