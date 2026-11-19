/*
 * This code file implements the CString class used to provide automatic
 * string functionality using a C++ class. All strings are guaranteed
 * zero terminated and will always allocate enough space for string 
 * storage. Strings grow and shrink on the fly.
 *
 * Original Author: Jeffrey R. Naujok <naujok@execpc.com>
 *
 * Original Revision: February 10th, 1996
 *
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "Globals.h"

CString::CString( const char *pszString )
{
  m_pchData = new char[ strlen( pszString ) + 1 ];
  m_nBufferSize = (unsigned long )((unsigned long )strlen( pszString ) + (unsigned long )1);
  m_nLength = (unsigned long )(m_nBufferSize - (unsigned long )1);
  strcpy( m_pchData, pszString );
}

CString::CString( const CString &oSource )
{
  m_pchData = new char [oSource.m_nBufferSize];
  m_nBufferSize = oSource.m_nBufferSize;
  m_nLength = oSource.m_nLength;
  strcpy( m_pchData, oSource.m_pchData );
}

CString::~CString( void )
{
  if ( m_pchData )
    delete m_pchData;
}


const CString &CString::operator =( const CString &oSource )
{
  char *pTemp = new char[ oSource.m_nBufferSize ];

  delete m_pchData;
  m_pchData = pTemp;
  m_nBufferSize = (unsigned long )(oSource.m_nLength + (unsigned long )1);
  m_nLength = oSource.m_nLength;
  strcpy( m_pchData, oSource.m_pchData );

  return *this;
}

const CString &CString::operator =( const unsigned char*pszSource )
{
  char *pTemp = new char[ strlen( (const char *)pszSource) + 1 ];

  delete m_pchData;
  m_pchData = pTemp;
  m_nLength = (unsigned long )strlen( (const char *)pszSource );
  m_nBufferSize = (unsigned long )(m_nLength + (unsigned long )1);
  strcpy( m_pchData, (const char *)pszSource );

  return *this;
}


CString operator +( const CString &string1, const CString &string2 )
{
  CString     strTemp( string1 );
  char       *pTemp;

  strTemp.m_nLength += string2.m_nLength;
  strTemp.m_nBufferSize += (unsigned long )(string2.m_nBufferSize - (unsigned long )1);
  pTemp = new char[ strTemp.m_nBufferSize ];
  delete strTemp.m_pchData;
  strTemp.m_pchData = pTemp;
  strcpy( strTemp.m_pchData, string1.m_pchData );
  strcat( strTemp.m_pchData, string2.m_pchData );

  return strTemp;
}

CString operator +( const CString &string, char *psz )
{
  CString     strTemp( string );
  char       *pTemp;

  strTemp.m_nLength += (unsigned long )strlen( psz );
  strTemp.m_nBufferSize += (unsigned long )strlen( psz );
  pTemp = new char[ strTemp.m_nBufferSize ];
  delete strTemp.m_pchData;
  strTemp.m_pchData = pTemp;
  strcpy( strTemp.m_pchData, string.m_pchData );
  strcat( strTemp.m_pchData, psz );

  return strTemp;
}

CString operator +( char *psz, const CString &string )
{
  CString     strTemp( psz );
  char       *pTemp;

  strTemp.m_nLength += string.m_nLength;
  strTemp.m_nBufferSize += string.m_nBufferSize;
  pTemp = new char[ strTemp.m_nBufferSize ];
  delete strTemp.m_pchData;
  strTemp.m_pchData = pTemp;
  strcpy( strTemp.m_pchData, psz );
  strcat( strTemp.m_pchData, string.m_pchData );

  return strTemp;
}
    
const CString &CString::operator +=( const CString &oSource )
{
  char *pTemp;
  m_nLength += oSource.m_nLength;
  m_nBufferSize += (unsigned long )(oSource.m_nBufferSize - (unsigned long )1);
  pTemp = new char[ m_nBufferSize ];
  strcpy( pTemp, m_pchData );
  delete m_pchData;
  m_pchData = pTemp;
  strcat( m_pchData, oSource.m_pchData );

  return *this;
}

const CString &CString::operator +=( char *pszSource )
{
  CString oSource(pszSource);
  char    *pTemp;

  m_nLength += oSource.m_nLength;
  m_nBufferSize += oSource.m_nBufferSize;
  pTemp = new char[ m_nBufferSize ];
  strcpy( pTemp, m_pchData );
  delete m_pchData;
  m_pchData = pTemp;
  strcat( m_pchData, oSource.m_pchData );

  return *this;
}

const CString &CString::operator +=( char chSource )
{
  char *    pTemp;
  m_nLength += (unsigned long )1;
  m_nBufferSize += (unsigned long )1;
  pTemp = new char[ m_nBufferSize ];
  strcpy( pTemp, m_pchData );
  delete m_pchData;
  m_pchData = pTemp;
  m_pchData[ m_nLength - 1] = chSource;
  m_pchData[ m_nLength ] = 0;

  return *this;
}

CString::operator char *()
{
  return (char *)m_pchData;
}

unsigned long  CString::GetLength( void ) const
{
  return m_nLength;
}

char CString::operator []( int nIndex ) const
{
  return m_pchData[ nIndex ];
}

BOOL CString::operator ==( const CString& s1 )
{
  return ( strcmp( m_pchData, s1.m_pchData ) == 0 );
}

BOOL CString::operator ==( char *s2 )
{
  return ( strcmp( m_pchData, s2 ) == 0 );
}

BOOL CString::operator !=( const CString& s1 )
{
  return ( strcmp( m_pchData, s1.m_pchData ) != 0 );
}

BOOL CString::operator !=( char *s1 )
{
  return ( strcmp( m_pchData, s1 ) != 0 );
}

BOOL CString::operator <( const CString& s1 )
{
  return ( strcmp( m_pchData, s1.m_pchData ) < 0 );
}

BOOL CString::operator <( char *s1 )
{
  return ( strcmp( m_pchData, s1 ) < 0 );
}

BOOL CString::operator >( const CString& s1 )
{
  return ( strcmp( m_pchData, s1.m_pchData ) > 0 );
}

BOOL CString::operator >( char *s1 )
{
  return ( strcmp( m_pchData, s1 ) > 0 );
}

BOOL CString::operator <=( const CString& s1 )
{
  return ( strcmp( m_pchData, s1.m_pchData ) <= 0 );
}

BOOL CString::operator <=( char *s1 )
{
  return ( strcmp( m_pchData, s1 ) <= 0 );
}

BOOL CString::operator >=( const CString& s1 )
{
  return ( strcmp( m_pchData, s1.m_pchData ) >= 0 );
}

BOOL CString::operator >=( char *s1 )
{
  return ( strcmp( m_pchData, s1 ) >= 0 );
}

int CString::Compare( const char *lpString2 )
{
    size_t      nLength;
    size_t      nLength2 = strlen( lpString2 );
    int         nRetVal;

    if (nLength2 > m_nLength )
        nLength = m_nLength;
    else
        nLength = nLength2;

    nRetVal = memcmp( m_pchData, lpString2, nLength );
    if ( nRetVal == 0 )
        nRetVal = ( nLength2 > m_nLength ) ? -1 : 0;
    if ( nRetVal == 0 )
        nRetVal = ( nLength2 == m_nLength ) ? 0 : 1;

    return nRetVal;
}

void CString::MakeUpper( void )
{
  unsigned long   nCount;

  nCount = 0;

  while ( nCount < m_nLength )
  {
    m_pchData[nCount] = (char)toupper( m_pchData[nCount] );
    nCount++;
  }
}

void CString::MakeLower( void )
{
  unsigned long   nCount;

  nCount = 0;

  while ( nCount < m_nLength )
  {
    m_pchData[nCount] = (char)tolower( m_pchData[nCount] );
    nCount++;
  }
}

void CString::Capitalize( void )
{
  if ( m_nLength )
    m_pchData[0] = (char)toupper( m_pchData[0] );
}

long CString::GetValue( void )
{
  long    lnValue = 0;
  BOOL    bNegative = FALSE;
  char   *pchTemp;

  pchTemp = m_pchData;

  while ( *pchTemp )
  {
    if ( ( *pchTemp == '-' ) && ( lnValue == 0 ) )
      bNegative = TRUE;

    if ( isdigit( *pchTemp ) )
    {
      lnValue *= 10;
      lnValue += (long)( *pchTemp - '0' );
    }
    pchTemp++;
  }
  
  if ( bNegative )
    lnValue = -lnValue;

  return lnValue;
}

void CString::MakeString( long lnValue, long lnSize )
{
  long    lnIndex;
  int     nTemp;
  char    chTemp;
  BOOL    bNegative;
  long    lnStepper;

  if ( lnSize > 10 )
    lnSize = 10;
  m_nLength = 0;
  m_nBufferSize = 12;
  delete m_pchData;
  m_pchData = new char[ m_nBufferSize ];
  memset( m_pchData, 0, m_nBufferSize );

  if ( lnValue < 0 )
  {
    bNegative = TRUE;
    lnValue = -lnValue;
  }
  else
    bNegative = FALSE;
  lnIndex = 0;
  while ( ( lnValue ) && (lnIndex < lnSize ) )
  {
    nTemp = lnValue % 10;
    chTemp = (char)((char)'0' + (char)nTemp);
    lnValue /= 10;
    lnStepper = 10;
    while ( lnStepper > 0 )
    {
      m_pchData[ lnStepper ] = m_pchData[ lnStepper - 1 ];
      lnStepper--;
    }
    m_pchData[ 0 ] = chTemp;
    lnIndex++;
  }

  while ( lnIndex < lnSize )
  {
    lnStepper = 10;
    while ( lnStepper > 0 )
    {
      m_pchData[ lnStepper ] = m_pchData[ lnStepper - 1 ];
      lnStepper--;
    }
    m_pchData[ 0 ] = '0';
    lnIndex++;
  }

  if ( bNegative )
  {
    lnStepper = 10;
    while ( lnStepper > 0 )
    {
      m_pchData[ lnStepper ] = m_pchData[ lnStepper - 1 ];
      lnStepper--;
    }
    m_pchData[ 0 ] = '-';
  }
}

CString CString::Left( int nLast )
{
    CString     strTemp( m_pchData );

    if ( ( nLast >= 0 ) && ( nLast <= (int)strTemp.m_nLength ) )
    {
        strTemp.m_nLength = nLast;
        strTemp.m_pchData[ nLast ] = 0;
    }
    return strTemp;
}

CString CString::Right( int nLength )
{
    CString     strTemp;
    int         nLast;

    nLast = m_nLength - nLength;

    if ( ( nLast >= 0 ) && ( nLast <= (int)m_nLength ) )
    {
        strTemp = &(m_pchData[ nLast ]);
    }
    return strTemp;
}

CString CString::Mid( int nStart, int nLength )
{
    CString     strTemp;
    char        chTempStore = 0;

    if ( ( nStart >= 0 ) && ( nStart <= (int)m_nLength ) )
    {
        if ( nLength != 0 )
        {
            nLength += nStart;
            if ( ( nLength > 0 ) && ( nLength <= (int)m_nLength ) )
            {
                chTempStore = m_pchData[ nLength ];
                m_pchData[ nLength ] = 0;
            }
        }
        strTemp = &(m_pchData[ nStart ] );
        if ( chTempStore )
        {
            m_pchData[ nLength ] = chTempStore;
        }
    }
    return strTemp;
}

long CString::Find( char chFind )
{
    long nPos;

    nPos = 0;

    while ( ( (unsigned long)nPos < m_nLength ) && ( m_pchData[ nPos ] != chFind ) )
        nPos++;

    if ( nPos >= (int)m_nLength )
        return -1;

    return nPos;
}

long CString::Find( char *strString )
{
    char   *strRet;
    long     nPos;

    strRet = strstr( m_pchData, strString );

    if ( strRet )
    {
        nPos = (long)(strRet - m_pchData );
    }
    else
        nPos = -1;

    return nPos;
}

long CString::Find( const CString &strFind )
{
    return Find( strFind.m_pchData );
}

long CString::TrimLeft( void )
{
    CString         strTemp;
    unsigned long   nCount;

    nCount = 0;

    while ( m_pchData[ nCount ] == ' ' )
        nCount++;

    strTemp = &(m_pchData[nCount]);

    *this = strTemp;

    return (long)nCount;
}

long CString::TrimRight( void )
{
    unsigned long   nCount;

    nCount = 0;

    while ( ( nCount < m_nLength ) && ( m_pchData[ (m_nLength - nCount) - 1 ] == ' ' ) )
    {
        m_pchData[ (m_nLength - nCount) - 1 ] = 0;
        nCount++;
    }

    m_nLength -= nCount;

    return nCount;
}

long CString::Trim( void )
{
    return TrimLeft() + TrimRight();
}

#ifdef _DEBUG
#include <stdio.h>

void CString::Dump( void )
{
  printf( "CString( m_nLength = %ld\n", m_nLength );
  printf( "m_nBufferSize = %ld\n", m_nBufferSize );
  printf( "m_pchData = [%s]\n", m_pchData );
  printf( " [End of CString] )\n" );
}
#endif