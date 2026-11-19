/*
 *
 * This header file defines the CString class used to provide automatic
 * string functionality using a C++ class. All strings are guaranteed
 * zero terminated and will always allocate enough space for string 
 * storage. Strings grow and shrink on the fly.
 *
 * Original Author: Jeffrey R. Naujok <naujok@execpc.com>
 *
 * Original Revision: February 4th, 1996
 *
 */

#ifndef __CSTRING_H__
#define __CSTRING_H__

class CString
{
  private:
    char               *m_pchData;
    unsigned long       m_nLength;
    unsigned long       m_nBufferSize;

  public:
                        CString( const char *pszString = "" );
                        CString( const CString &oSource );
    virtual            ~CString( void );

    const CString      &operator =( const CString &strSource );
    const CString      &operator =( const unsigned char*pszSource );

    friend CString      operator +( const CString &string1, const CString &string2 );
    friend CString      operator +( const CString &string, char *psz );
    friend CString      operator +( char *psz, const CString &string );

    const CString      &operator +=( const CString &oSource );
    const CString      &operator +=( char *pszSource );
    const CString      &operator +=( char chSource );

                        operator char *();

    unsigned long       GetLength( void ) const;
    char                operator []( int nIndex ) const;

    BOOL                operator ==( const CString& s1 );
    BOOL                operator ==( char *s2 );
    BOOL                operator !=( const CString& s1 );
    BOOL                operator !=( char *s1 );
    BOOL                operator <( const CString& s1 );
    BOOL                operator <( char *s1 );
    BOOL                operator >( const CString& s1 );
    BOOL                operator >( char *s1 );
    BOOL                operator <=( const CString& s1 );
    BOOL                operator <=( char *s1 );
    BOOL                operator >=( const CString& s1 );
    BOOL                operator >=( char *s1 );

    int                 Compare( const char *lpString2 );
    void                MakeUpper( void );
    void                MakeLower( void );
    void                Capitalize( void );

    long                GetValue( void );
    void                MakeString( long lnValue, long lnSize );

    long                Find( char chFind );
    long                Find( char *strFind );
    long                Find( const CString &strFind );

    CString             Left( int nLast );
    CString             Right( int nFirst );
    CString             Mid( int nFirst, int nLength );

    long                TrimRight( void );
    long                TrimLeft( void );
    long                Trim( void );
#ifdef _DEBUG
    void                Dump( void );
#endif

};

#endif
