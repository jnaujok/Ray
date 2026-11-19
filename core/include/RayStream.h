/*************************************************************************
*   CRayStream                                                           *
*                                                                        *
*   Reads and writes the ray tracer's binary serialization format on top *
*   of any IASLib byte stream (a file, a socket, or memory). The format  *
*   is the one the original 1997 CFileStream wrote, so existing .ray     *
*   scene files load unchanged:                                          *
*                                                                        *
*     BOOL / int      1 byte, 1 = TRUE                                   *
*     char            1 byte                                             *
*     short           2 bytes, big-endian                                *
*     long / DWORD    4 bytes, big-endian (whatever the size of long)    *
*     float, double   text ("%.10e" / "%.18e") written as a string       *
*     string          2-byte big-endian length, then the characters      *
*                                                                        *
*   Plain int is written as a one-byte BOOL, exactly as it was on Win32  *
*   where BOOL is int.                                                   *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __RAYSTREAM_H__
#define __RAYSTREAM_H__

#include "Globals.h"
#include "Streams/Stream.h"

class CRayStream
{
    protected:
        IASLib::CStream    &m_oStream;
        BOOL                m_bLegacyDoubles;

    public:
        explicit            CRayStream( IASLib::CStream &oStream ) : m_oStream( oStream ), m_bLegacyDoubles( FALSE ) {}

            /// Scene files from before April 1997 store each float or double
            /// as two 4-byte values, a whole part and a fraction of 2^31,
            /// instead of as text. Reading only.
        void                SetLegacyDoubles( BOOL bLegacy ) { m_bLegacyDoubles = bLegacy; }
        BOOL                IsLegacyDoubles( void ) const { return m_bLegacyDoubles; }
        virtual            ~CRayStream( void ) {}

            /// True while there is more to read. Uses the byte count when the
            /// stream knows it (files and memory): a file only reports its
            /// end after a read has failed.
        BOOL                DataAvailable( void );
            /// The byte stream underneath.
        IASLib::CStream    &GetStream( void ) { return m_oStream; }
        void                Flush( void ) { m_oStream.Flush(); }

        void                WriteBool( int bValue ) { WriteByte( (uint8_t)( bValue ? 1 : 0 ) ); }
        void                WriteByte( uint8_t nValue ) { m_oStream.PutUInt8( nValue ); }
        void                WriteUInt16( uint16_t nValue ) { m_oStream.PutUInt16( nValue ); }
        void                WriteUInt32( uint32_t nValue ) { m_oStream.PutUInt32( nValue ); }
        void                WriteFloat( float nValue );
        void                WriteDouble( double nValue );
        void                WriteString( const char *strValue );

        BOOL                ReadBool( void ) { return ( ReadByte() == 1 ) ? TRUE : FALSE; }
        uint8_t             ReadByte( void ) { return m_oStream.GetUInt8(); }
        uint16_t            ReadUInt16( void ) { return m_oStream.GetUInt16(); }
        uint32_t            ReadUInt32( void ) { return m_oStream.GetUInt32(); }
        float               ReadFloat( void );
        double              ReadDouble( void );
        IASLib::CString     ReadString( void );

        CRayStream         &operator <<( int nValue )            { WriteBool( nValue ); return *this; }
        CRayStream         &operator <<( char nValue )           { WriteByte( (uint8_t)nValue ); return *this; }
        CRayStream         &operator <<( unsigned char nValue )  { WriteByte( nValue ); return *this; }
        CRayStream         &operator <<( short nValue )          { WriteUInt16( (uint16_t)nValue ); return *this; }
        CRayStream         &operator <<( unsigned short nValue ) { WriteUInt16( nValue ); return *this; }
        CRayStream         &operator <<( unsigned int nValue )   { WriteUInt32( (uint32_t)nValue ); return *this; }
        CRayStream         &operator <<( long nValue )           { WriteUInt32( (uint32_t)nValue ); return *this; }
        CRayStream         &operator <<( unsigned long nValue )  { WriteUInt32( (uint32_t)nValue ); return *this; }
        CRayStream         &operator <<( float nValue )          { WriteFloat( nValue ); return *this; }
        CRayStream         &operator <<( double nValue )         { WriteDouble( nValue ); return *this; }
        CRayStream         &operator <<( const char *strValue )  { WriteString( strValue ); return *this; }

        CRayStream         &operator >>( int &nValue )            { nValue = ReadBool(); return *this; }
        CRayStream         &operator >>( char &nValue )           { nValue = (char)ReadByte(); return *this; }
        CRayStream         &operator >>( unsigned char &nValue )  { nValue = ReadByte(); return *this; }
        CRayStream         &operator >>( short &nValue )          { nValue = (short)ReadUInt16(); return *this; }
        CRayStream         &operator >>( unsigned short &nValue ) { nValue = ReadUInt16(); return *this; }
        CRayStream         &operator >>( unsigned int &nValue )   { nValue = ReadUInt32(); return *this; }
        CRayStream         &operator >>( long &nValue )           { nValue = (long)(int32_t)ReadUInt32(); return *this; }
        CRayStream         &operator >>( unsigned long &nValue )  { nValue = ReadUInt32(); return *this; }
        CRayStream         &operator >>( float &nValue )          { nValue = ReadFloat(); return *this; }
        CRayStream         &operator >>( double &nValue )         { nValue = ReadDouble(); return *this; }
        CRayStream         &operator >>( IASLib::CString &strValue ) { strValue = ReadString(); return *this; }

    private:
                            CRayStream( const CRayStream & );
        CRayStream         &operator =( const CRayStream & );
};

#endif // __RAYSTREAM_H__
