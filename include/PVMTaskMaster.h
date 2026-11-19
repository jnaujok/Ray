#ifndef __PVMTASKMASTER_H__
#define __PVMTASKMASTER_H__

#include "TaskMaster.h"

class CPVMTaskMaster : public CTaskMaster
{
    protected:
        int             m_nMyTaskID;
        int             m_nChildTasks;
        int            *m_anChildren;
        int             m_nPercentCompleted;
    public:
                        CPVMTaskMaster( void );
        virtual        ~CPVMTaskMaster( void );

        virtual BOOL    SpawnTasks( int nTasksToSpawn );
        virtual BOOL    EndTasks( void );

        virtual BOOL    SceneToTasks( CScene &oScene );
        virtual BOOL    RaysToTask( CRayArray &oRays, int nTask );
        virtual BOOL    ColorsFromTask( CImageFile &oImageFile, int &nTask, int &nPixels );

        virtual int     Status( void );

};

#endif