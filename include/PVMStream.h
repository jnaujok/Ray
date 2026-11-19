#ifndef __PVMSTREAM_H__
#define __PVMSTREAM_H__

#include "Stream.h"
#include <stdio.h>

class CPVMStream : public CStream
{
    protected:
        int                 m_nTaskID;
        int                 m_nBufID;
        int                 m_nMessageTag;
        BOOL                m_bSending;
        int                *m_anTasks;
    public:
                            CPVMStream( int TaskID, int nMessageTag, BOOL bSending );
                            CPVMStream( int *anTaskList, int nTasks, int nMessageTag );
        virtual            ~CPVMStream( void );

        virtual void        Write( char nChar );
        virtual void        Write( unsigned char nUChar );
        virtual void        Write( short nShort );
        virtual void        Write( unsigned short nUShort );
        virtual void        Write( unsigned long nLong );
        virtual void        Write( long nULong );
        virtual void        Write( float nFloat );
        virtual void        Write( double nDouble );
        virtual void        Write( LPCSTR strString );

        virtual void        Read( char &nChar );
        virtual void        Read( unsigned char &nUChar );
        virtual void        Read( short &nShort );
        virtual void        Read( unsigned short &nUShort );
        virtual void        Read( unsigned long &nLong );
        virtual void        Read( long &nULong );
        virtual void        Read( float &nFloat );
        virtual void        Read( double &nDouble );
        virtual void        Read( CString &strString );

        virtual void        Flush( void );

        virtual BOOL        DataAvailable( void );
};

#endif

