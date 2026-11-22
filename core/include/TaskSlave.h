#ifndef __TASKSLAVE_H__
#define __TASKSLAVE_H__

#include "Globals.h"
#include "TileTracer.h"

#include <memory>
#include <stdint.h>
#include <vector>

class CScene;

    // A tracing task. It pulls its work from the master: once it has the
    // scene it asks for a tile, traces it, sends the pixels back and asks
    // for the next, until the master tells it to stop. A new scene can come
    // at any time; the task answers it with a request.
class CTaskSlave
{
    public:
            // What FromMaster got.
        enum EMessage
        {
            FROM_MASTER_ERROR,      // the connection failed, or the message was bad
            FROM_MASTER_SCENE,
            FROM_MASTER_TILE,
            FROM_MASTER_STOP
        };

    protected:
        long long           m_nTilesTraced;
        long long           m_nRaysTraced;

    public:
                            CTaskSlave( void ) : m_nTilesTraced( 0 ), m_nRaysTraced( 0 ) {}
        virtual            ~CTaskSlave( void ) {}

            /// Pulls and traces tiles until told to stop. Returns FALSE if it
            /// stopped because something failed.
        virtual BOOL        RunEngine( void );

            /// Waits for the master's next message. A scene is read into a
            /// new CScene in pScene; a tile into oTile and nTrace.
        virtual EMessage    FromMaster( std::unique_ptr<CScene> &pScene, STile &oTile, uint32_t &nTrace ) = 0;
            /// Asks the master for a tile.
        virtual BOOL        RequestWork( void ) = 0;
            /// Sends a traced tile's pixels.
        virtual BOOL        PixelsToMaster( uint32_t nTrace, const STile &oTile, const CRayCounts &oCounts, const std::vector<double> &anColors ) = 0;

        long long           GetTilesTraced( void ) const { return m_nTilesTraced; }
        long long           GetRaysTraced( void ) const { return m_nRaysTraced; }
};

#endif
