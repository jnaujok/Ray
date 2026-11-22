#include "Globals.h"

#include "Scene.h"
#include "TaskSlave.h"


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

                CRayCounts oCounts;
                long long  nRays = oTracer.Trace( *pScene, oTile, anColors, &oCounts );

                m_nTilesTraced++;
                m_nRaysTraced += nRays;
                if ( ! PixelsToMaster( nTrace, oTile, oCounts, anColors ) || ! RequestWork() )
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
