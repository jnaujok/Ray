#ifndef __FILESTREAM_H__
#define __FILESTREAM_H__

#include "Stream.h"
#include <stdio.h>

class CFileStream : public CStream
{
    protected:
        FILE               *m_fpStreamFile;
        DWORD               m_dwFileSize;
    public:
                            CFileStream( LPCSTR strFilename );
        virtual            ~CFileStream( void );

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

