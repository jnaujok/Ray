#ifndef __THREADTASKMASTER_H__
#define __THREADTASKMASTER_H__

#include "TaskMaster.h"
#include "PipeStream.h"
#include "Semaphore.h"

typedef CPipeStream *LPPIPE;
typedef CSem       *LPSEMAPHORE;

class CThreadTaskMaster : public CTaskMaster
{
    protected:
        int             m_nMyTaskID;
        int             m_nChildTasks;
        LPPIPE         *m_anPipes;
        LPPIPE         *m_anReturnPipes;
        int             m_nPercentCompleted;
        LPSEMAPHORE    *m_anMasterLocks;
        LPSEMAPHORE    *m_anLocks;

    public:
                        CThreadTaskMaster( void );
        virtual        ~CThreadTaskMaster( void );

        virtual BOOL    SpawnTasks( int nTasksToSpawn );
        virtual BOOL    EndTasks( void );

        virtual BOOL    SceneToTasks( CScene &oScene );
        virtual BOOL    RaysToTask( CRayArray &oRays, int nTask );
        virtual BOOL    ColorsFromTask( CImageFile &oImageFile, int &nTask, int &nPixels );

        virtual int     Status( void );
};

#endif
