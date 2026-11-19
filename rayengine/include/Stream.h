#ifndef __STREAM_H__
#define __STREAM_H__

#include "Globals.h"

class CStream
{
    public:
                            CStream( void ) {}
        virtual            ~CStream( void ) {}

        virtual BOOL        DataAvailable( void ) = 0;

        CStream            &operator <<( BOOL           bValue )
                            {
                                char    chValue;

                                chValue = ( bValue ) ? 1 : 0;
                                Write( chValue );
                                return *this;
                            }

        CStream            &operator >>( BOOL          &bValue )
                            {
                                char    chValue;

                                Read( chValue );
                                bValue = ( chValue == 1 ) ? TRUE : FALSE;
                                return *this;
                            }

        CStream            &operator <<( char           nChar )
                            {
                                Write( nChar );
                                return *this;
                            }

        CStream            &operator <<( unsigned char  nUChar )
                            {
                                Write( nUChar );
                                return *this;
                            }

        CStream            &operator <<( short          nShort )
                            {
                                Write( nShort );
                                return *this;
                            }

        CStream            &operator <<( unsigned short nUShort )
                            {
                                Write( nUShort );
                                return *this;
                            }

        CStream            &operator <<( long           nLong )
                            {
                                Write( nLong );
                                return *this;
                            }

        CStream            &operator <<( unsigned long  nULong )
                            {
                                Write( nULong );
                                return *this;
                            }

        CStream            &operator <<( float          nFloat )
                            {
                                Write( nFloat );
                                return *this;
                            }

        CStream            &operator <<( double         nDouble )
                            {
                                Write( nDouble );
                                return *this;
                            }

        CStream            &operator <<( LPCSTR         strString )
                            {
                                Write( strString );
                                return *this;
                            }


        CStream            &operator >>( char           &nChar )
                            {
                                Read( nChar );
                                return *this;
                            }

        CStream            &operator >>( unsigned char  &nUChar )
                            {
                                Read( nUChar );
                                return *this;
                            }

        CStream            &operator >>( short          &nShort )
                            {
                                Read( nShort );
                                return *this;
                            }

        CStream            &operator >>( unsigned short &nUShort )
                            {
                                Read( nUShort );
                                return *this;
                            }

        CStream            &operator >>( long           &nLong )
                            {
                                Read( nLong );
                                return *this;
                            }

        CStream            &operator >>( unsigned long  &nULong )
                            {
                                Read( nULong );
                                return *this;
                            }

        CStream            &operator >>( float          &nFloat )
                            {
                                Read( nFloat );
                                return *this;
                            }

        CStream            &operator >>( double         &nDouble )
                            {
                                Read( nDouble );
                                return *this;
                            }

        CStream            &operator >>( CString        &strString )
                            {
                                Read( strString );
                                return *this;
                            }


        virtual void        Write( char nChar ) = 0;
        virtual void        Write( unsigned char nUChar ) = 0;
        virtual void        Write( short nShort ) = 0;
        virtual void        Write( unsigned short nUShort ) = 0;
        virtual void        Write( unsigned long nLong ) = 0;
        virtual void        Write( long nULong ) = 0;
        virtual void        Write( float nFloat ) = 0;
        virtual void        Write( double nDouble ) = 0;
        virtual void        Write( LPCSTR strString ) = 0;

        virtual void        Read( char &nChar ) = 0;
        virtual void        Read( unsigned char &nUChar ) = 0;
        virtual void        Read( short &nShort ) = 0;
        virtual void        Read( unsigned short &nUShort ) = 0;
        virtual void        Read( unsigned long &nLong ) = 0;
        virtual void        Read( long &nULong ) = 0;
        virtual void        Read( float &nFloat ) = 0;
        virtual void        Read( double &nDouble ) = 0;
        virtual void        Read( CString &strString ) = 0;

        virtual void        Flush( void ) {}
};

#endif // __STREAM_H__
