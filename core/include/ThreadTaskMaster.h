/*************************************************************************
*   CThreadTaskMaster                                                    *
*                                                                        *
*   Traces on this machine with one worker thread per task. Each worker  *
*   gets its own copy of the scene. Batches of rays go to each worker    *
*   through a bounded queue, and the colors come back through one queue  *
*   shared by all of them.                                               *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __THREADTASKMASTER_H__
#define __THREADTASKMASTER_H__

#include "TaskMaster.h"
#include "WorkQueue.h"

#include <memory>
#include <string>
#include <vector>

class CColorArray;

class CThreadTaskMaster : public CTaskMaster
{
    public:
        class CWorker;

    protected:
        std::vector<CWorker *>      m_apWorkers;
        TWorkQueue<CColorArray *>   m_oResults;     // NULL: a worker stopped by itself.

    public:
                        CThreadTaskMaster( void );
        virtual        ~CThreadTaskMaster( void );

        virtual BOOL    SpawnTasks( int nTasksToSpawn );
        virtual BOOL    EndTasks( void );

        virtual BOOL    SceneToTasks( CScene &oScene );
        virtual BOOL    RaysToTask( CRayArray &oRays, int nTask );
        virtual int     ColorsFromTasks( CImageFile &oImageFile, BOOL bWait );

    private:
                        CThreadTaskMaster( const CThreadTaskMaster & );
        CThreadTaskMaster &operator =( const CThreadTaskMaster & );
};

#endif
