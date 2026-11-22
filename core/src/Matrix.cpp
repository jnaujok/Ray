/*************************************************************************
*   CMatrix                                                              *
*                                                                        *
*       This is the base class for matrix math used for 3-d transforms.  *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Objects.h"
#include "Matrix.h"



CMatrix::CMatrix( void )
{
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++)
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         m_aafMatrix[nX][nY] = (FLT)0.0;
      }
   }
}

CMatrix::CMatrix( BOOL bIdentity )       
{
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++)
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         if ( ( bIdentity ) && ( nX == nY ) )
         {
            m_aafMatrix[nX][nY] = (FLT)1.0;
         }
         else
         {
            m_aafMatrix[nX][nY] = (FLT)0.0;
         }
      }
   }
}
         
CMatrix::CMatrix( FLT x11, FLT x12, FLT x13, FLT x14,
                  FLT x21, FLT x22, FLT x23, FLT x24,
                  FLT x31, FLT x32, FLT x33, FLT x34,
                  FLT x41, FLT x42, FLT x43, FLT x44 )
{
   m_aafMatrix[ 0 ][ 0 ] = x11;
   m_aafMatrix[ 1 ][ 0 ] = x12;
   m_aafMatrix[ 2 ][ 0 ] = x13;
   m_aafMatrix[ 3 ][ 0 ] = x14;
   m_aafMatrix[ 0 ][ 1 ] = x21;
   m_aafMatrix[ 1 ][ 1 ] = x22;
   m_aafMatrix[ 2 ][ 1 ] = x23;
   m_aafMatrix[ 3 ][ 1 ] = x24;
   m_aafMatrix[ 0 ][ 2 ] = x31;
   m_aafMatrix[ 1 ][ 2 ] = x32;
   m_aafMatrix[ 2 ][ 2 ] = x33;
   m_aafMatrix[ 3 ][ 2 ] = x34;
   m_aafMatrix[ 0 ][ 3 ] = x41;
   m_aafMatrix[ 1 ][ 3 ] = x42;
   m_aafMatrix[ 2 ][ 3 ] = x43;
   m_aafMatrix[ 3 ][ 3 ] = x44;
}

CMatrix::CMatrix( const CMatrix &oSource )
{
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++)
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         m_aafMatrix[nX][nY] = oSource.m_aafMatrix[nX][nY];
      }
   }
}

CMatrix::~CMatrix( void )
{
   // Nothing to do here
}

void CMatrix::Zero( void )
{
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++)
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         m_aafMatrix[nX][nY] = (FLT)0.0;
      }
   }
}

void CMatrix::Identity( void )
{
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++)
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         if ( nX == nY )
         {
            m_aafMatrix[nX][nY] = (FLT)1.0;
         }
         else
         {
            m_aafMatrix[nX][nY] = (FLT)0.0;
         }
      }
   }
}

CMatrix  &CMatrix::operator =( const CMatrix &oSource )
{
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++)
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         m_aafMatrix[nX][nY] = oSource.m_aafMatrix[nX][nY];
      }
   }
   return *this;
}

CMatrix  CMatrix::operator *( const CMatrix &oMultiplier )
{
   CMatrix  oResult;
   int nX, nY, nZ;
   
   for ( nX = 0; nX < 4; nX++ )
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         oResult.m_aafMatrix[ nX ][ nY ] = (FLT)0.00;
         for ( nZ = 0; nZ < 4; nZ++ )
         {
            oResult.m_aafMatrix[ nX ][ nY ] += m_aafMatrix[ nX ][ nZ ] * oMultiplier.m_aafMatrix[ nX ][ nY ];
         }
      }
   }
   return oResult;
}


CMatrix CMatrix::operator *( const FLT nScalar )
{
   CMatrix     oResult( *this );
   int         nX, nY;
   
   for ( nX = 0; nX < 4; nX++ )
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         oResult.m_aafMatrix[ nX ][ nY ] *= nScalar;
      }
   }
   return oResult;
}
   
CMatrix CMatrix::operator +( const CMatrix &oAdd )
{
   CMatrix  oResult;
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++ )
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         oResult.m_aafMatrix[ nX ][ nY ] = m_aafMatrix[ nX ][ nY ] + oAdd.m_aafMatrix[ nX ][ nY ];
      }
   }
   return oResult;
}

CMatrix CMatrix::operator -( const CMatrix &oSubtract )
{
   CMatrix  oResult;
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++ )
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         oResult.m_aafMatrix[ nX ][ nY ] = m_aafMatrix[ nX ][ nY ] - oSubtract.m_aafMatrix[ nX ][ nY ];
      }
   }
   return oResult;
}

CMatrix CMatrix::Transpose( void )
{
   CMatrix  oTemp;
   int   nX, nY;

   for ( nX = 0; nX < 4; nX++ )
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         oTemp.m_aafMatrix[ nX ][ nY ] = m_aafMatrix[ nY ][ nX ];
      }
   }
   return oTemp;
}

   