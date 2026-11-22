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
        RayWire::CDecoder       m_oDecoder;     // the scene from the master
        IASLib::CString         m_strLastError;

    public:
                            CSocketTaskSlave( void );
        virtual            ~CSocketTaskSlave( void );

            /// Connects to the master and says hello. With nRetrySeconds,
            /// keeps trying that long for the master to start listening.
        BOOL                Connect( const char *strHost, int nPort, int nRetrySeconds = 0 );

        virtual EMessage    FromMaster( std::unique_ptr<CScene> &pScene, STile &oTile, uint32_t &nTrace );
        virtual BOOL        RequestWork( void );
        virtual BOOL        PixelsToMaster( uint32_t nTrace, const STile &oTile, const CRayCounts &oCounts, const std::vector<double> &anColors );

        const char         *GetLastError( void ) const { return (const char *)m_strLastError; }

    private:
        BOOL                Send( uint8_t nTag, const std::string &strPayload );

                            CSocketTaskSlave( const CSocketTaskSlave & );
        CSocketTaskSlave   &operator =( const CSocketTaskSlave & );
};

#endif
