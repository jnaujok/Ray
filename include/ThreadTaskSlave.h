#ifndef __THREADTASKSLAVE_H__
#define __THREADTASKSLAVE_H__

#include "PipeStream.h"
#include "TaskSlave.h"
#include "Semaphore.h"

class CThreadTaskSlave : public CTaskSlave
{
    protected:
        CPipeStream    &m_oStream;
        CPipeStream     m_oToMaster;
        CSem           &m_oSemaphore;
        CSem           *m_poMasterLock;

    public:
                        CThreadTaskSlave( CPipeStream &oStream, CSem &oSemaphore );
        virtual        ~CThreadTaskSlave( void );

        virtual BOOL    SceneFromMaster( CScene &oScene );
        virtual BOOL    RaysFromMaster( CRayArray &oRays );
        virtual BOOL    ColorsToMaster( CColorArray &oColors );
};

#endif

