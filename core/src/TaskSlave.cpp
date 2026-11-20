#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Scene.h"
#include "TaskSlave.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CTaskSlave::RunEngine( void )
{
    std::unique_ptr<CScene> pScene;
    CTileTracer             oTracer;
    std::vector<double>     anColors;
    STile                   oTile;
    uint32_t                nTrace;

    for ( ;; )
    {
        switch ( FromMaster( pScene, oTile, nTrace ) )
        {
            case FROM_MASTER_SCENE:
                if ( ! RequestWork() )
                    return FALSE;
                break;

            case FROM_MASTER_TILE:
            {
                if ( ! pScene )
                    return FALSE;       // a tile before any scene

                long long nRays = oTracer.Trace( *pScene, oTile, anColors );

                m_nTilesTraced++;
                m_nRaysTraced += nRays;
                if ( ! PixelsToMaster( nTrace, oTile, nRays, anColors ) || ! RequestWork() )
                    return FALSE;
                break;
            }

            case FROM_MASTER_STOP:
                return TRUE;

            default:
                return FALSE;
        }
    }
}
