/*************************************************************************
*   CSocketTaskSlave                                                     *
*                                                                        *
*   The engine end of the TCP protocol: connects to a CSocketTaskMaster, *
*   then traces what it is sent. Used by rayengine.                      *
*                                                                        *
*************************************************************************/
#ifndef __SOCKETTASKSLAVE_H__
#define __SOCKETTASKSLAVE_H__

#include "TaskSlave.h"
#include "WireProtocol.h"

#include <stdint.h>
#include <string>

namespace IASLib
{
    class CSocket;
    class CSocketStream;
}

class CSocketTaskSlave : public CTaskSlave
{
    protected:
        IASLib::CSocketStream  *m_pStream;
        RayWire::CEncoder       m_oEncoder;     // colors to the master
        RayWire::CDecoder       m_oDecoder;     // the scene and rays from it
        IASLib::CString         m_strLastError;
        unsigned long           m_nRaysTraced;

    public:
                            CSocketTaskSlave( void );
        virtual            ~CSocketTaskSlave( void );

            /// Connects to the master and says hello. With nRetrySeconds,
            /// keeps trying that long for the master to start listening.
        BOOL                Connect( const char *strHost, int nPort, int nRetrySeconds = 0 );

        virtual BOOL        SceneFromMaster( CScene &oScene );
        virtual BOOL        RaysFromMaster( CRayArray &oRays );
        virtual BOOL        ColorsToMaster( CColorArray &oColors );

        const char         *GetLastError( void ) const { return (const char *)m_strLastError; }
        unsigned long       GetRaysTraced( void ) const { return m_nRaysTraced; }

    private:
        BOOL                Receive( uint8_t nExpected, std::string &strPayload );

                            CSocketTaskSlave( const CSocketTaskSlave & );
        CSocketTaskSlave   &operator =( const CSocketTaskSlave & );
};

#endif
