/*************************************************************************
*   CThreadTaskMaster                                                    *
*                                                                        *
*   Traces on this machine with one worker thread per task. Each worker  *
*   gets its own copy of the scene, then asks for tiles and traces them  *
*   (making its own camera rays, so the lens's work is spread over the   *
*   threads too).                                                        *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __THREADTASKMASTER_H__
#define __THREADTASKMASTER_H__

#include "TaskMaster.h"

#include <memory>
#include <string>
#include <vector>

class CThreadTaskMaster : public CTaskMaster
{
    public:
        class CWorker;

    protected:
        std::vector<CWorker *>              m_apWorkers;
        std::shared_ptr<const std::string>  m_pSceneText;       // the scene as JSON, for trace m_nSceneTextTrace
        uint32_t                            m_nSceneTextTrace;

        virtual BOOL    SceneToTask( int nTask, CScene &oScene );
        virtual BOOL    TileToTask( int nTask, uint32_t nTrace, const STile &oTile );

    public:
                        CThreadTaskMaster( void );
        virtual        ~CThreadTaskMaster( void );

        virtual BOOL    SpawnTasks( int nTasksToSpawn );
        virtual BOOL    EndTasks( void );

    private:
                        CThreadTaskMaster( const CThreadTaskMaster & );
        CThreadTaskMaster &operator =( const CThreadTaskMaster & );
};

#endif
