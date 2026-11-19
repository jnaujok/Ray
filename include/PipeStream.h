#ifndef __PIPESTREAM_H__
#define __PIPESTREAM_H__

#include "Stream.h"
#include <stdio.h>

class CPipeStream : public CStream
{
    protected:
        HANDLE              m_hOutputPipe;
        HANDLE              m_hInputPipe;
        CString             m_strPipeName;
    public:
                            CPipeStream( LPCSTR strPipeName, BOOL bClient );
                            CPipeStream( void );
        virtual            ~CPipeStream( void );

        void                Attach( LPCSTR strPipeName, BOOL bClient );

        virtual BOOL        DataAvailable( void );

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

};

#endif

