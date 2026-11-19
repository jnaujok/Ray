#ifndef __PVMTASKSLAVE_H__
#define __PVMTASKSLAVE_H__

#include "TaskSlave.h"

class CPVMTaskSlave : public CTaskSlave
{
    int         m_nTaskID;
    int         m_nParentID;

    public:
                        CPVMTaskSlave( void );
        virtual        ~CPVMTaskSlave( void );

        virtual BOOL    SceneFromMaster( CScene &oScene );
        virtual BOOL    RaysFromMaster( CRayArray &oRays );
        virtual BOOL    ColorsToMaster( CColorArray &oColors );
};

#endif

