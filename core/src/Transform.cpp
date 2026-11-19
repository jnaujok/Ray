/*************************************************************************
*   CTransform                                                           *
*                                                                        *
*       Used in 3-d to move and adjust 3-d coordinate triplets.          *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Objects.h"
#include "Vector.h"
#include "Matrix.h"
#include "Transform.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTransform::CTransform( void ) :
   CMatrix( TRUE )
{
   m_oInverse.Identity();
}

CTransform::CTransform( const CTransform &oSource )
{
   int  nX;
   int  nY;
   
   for ( nX = 0; nX < 4; nX++ )
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         m_aafMatrix[nX][nY] = oSource.m_aafMatrix[nX][nY];
      }
   }
   
   m_oInverse = oSource.m_oInverse;
}

CTransform::~CTransform( void )
{
}

CTransform &CTransform::operator =( const CTransform &oSource )
{
   int nX, nY;
   
   for ( nX = 0; nX < 4; nX++)
   {
      for ( nY = 0; nY < 4; nY++ )
      {
         m_aafMatrix[nX][nY] = oSource.m_aafMatrix[nX][nY];
      }
   }
   
   m_oInverse = oSource.m_oInverse;
   
   return *this;
}

CVector CTransform::Transform( CVector &oVector )
{
   CVector     oResult;
   int         nY;
   FLT       aAnswer[ 4 ];
   FLT       fX, fY, fZ;

   fX = oVector.GetX();
   fY = oVector.GetY();
   fZ = oVector.GetZ();
   
   for ( nY =0; nY < 4; nY++ )
   {
      aAnswer[ nY ] = (fX * m_aafMatrix[ 0 ][nY]) +
                      (fY * m_aafMatrix[ 1 ][nY]) +
                      (fZ * m_aafMatrix[ 2 ][nY]) +
                      m_aafMatrix[ 3 ][nY];
   }
   oResult.Set( aAnswer[0], aAnswer[1], aAnswer[2], aAnswer[3] );
   
   return oResult;
}

CVector CTransform::InverseTransform( CVector &oVector )
{
   CVector      oResult;
   int          nY;
   FLT        aAnswer[ 4 ];
   FLT        fX, fY, fZ;
   
   fX = oVector.GetX();
   fY = oVector.GetY();
   fZ = oVector.GetZ();

   for ( nY =0; nY < 4; nY++ )
   {
      aAnswer[ nY ] = (fX * m_oInverse.Get( 0, nY)) +
                      (fY * m_oInverse.Get( 1, nY)) +
                      (fZ * m_oInverse.Get( 2, nY)) +
                      m_oInverse.Get( 3, nY);
   }
   oResult.Set( aAnswer[0], aAnswer[1], aAnswer[2], aAnswer[3] );
   
   return oResult;
}

CTransform &CTransform::Scale( CVector &oVector )
{  
   Identity();
   m_oInverse.Identity();
   
   m_aafMatrix[ 0 ][ 0 ] = oVector.GetX();
   m_aafMatrix[ 1 ][ 1 ] = oVector.GetY();
   m_aafMatrix[ 2 ][ 2 ] = oVector.GetZ();

   m_oInverse.Set( 0, 0 ) = (FLT)1.0 / oVector.GetX();
   m_oInverse.Set( 1, 1 ) = (FLT)1.0 / oVector.GetY();
   m_oInverse.Set( 2, 2 ) = (FLT)1.0 / oVector.GetZ();

   return *this;
}
   
CTransform &CTransform::Rotate( CVector &oVector )
{
   CVector  oRadians;
   FLT      sinx, siny, sinz, cosx, cosy, cosz;
   CMatrix  oWork(TRUE), oAnswer(TRUE);
   CMatrix  oWorkI( TRUE ), oAnswerI( TRUE );
   int      nX, nY;
   
   oRadians = oVector * RADIANS;
   
   Identity();
   m_oInverse.Identity();
   
   sinx = SIN( oRadians.GetX() );
   siny = SIN( oRadians.GetY() );
   sinz = SIN( oRadians.GetZ() );
   cosx = COS( oRadians.GetX() );
   cosy = COS( oRadians.GetY() );
   cosz = COS( oRadians.GetZ() );
   
   oAnswer.Set( 1, 1) = oAnswer.Set( 2, 2 ) = cosx;
   oAnswer.Set( 1, 2) = sinx;
   oAnswer.Set( 2, 1) = -sinx;
   
   oAnswerI = oAnswer.Transpose();
   
   oWork.Set( 2, 2 ) = oWork.Set( 0, 0 ) = cosy;
   oWork.Set( 0, 2 ) = -siny;
   oWork.Set( 2, 0 ) = siny;
   
   oWorkI = oWork.Transpose();
   
   oAnswer = oAnswer * oWork;
   oAnswerI = oAnswerI * oWorkI;
   
   oWork.Identity();
   oWork.Set( 1, 1 ) = oWork.Set( 0, 0 ) = cosz;
   oWork.Set( 0, 1 ) = sinz;
   oWork.Set( 1, 0 ) = -sinz;
   
   oWorkI = oWork.Transpose();
   
   oAnswer = oAnswer * oWork;
   oAnswerI = oAnswerI * oWorkI;
   
   m_oInverse = oAnswerI;
   
   for ( nX = 0; nX < 4; nX++ )
   {
      for (nY = 0; nY < 4; nY++ )
      {
         m_aafMatrix[nX][nY] = oAnswer.Get( nX, nY );
      }
   }
   return *this;
}
   
CTransform &CTransform::Translate( CVector &oVector )
{
    Identity();
    m_oInverse.Identity();
    
    m_aafMatrix[ 3 ][ 0 ] = oVector.GetX();
    m_aafMatrix[ 3 ][ 1 ] = oVector.GetY();
    m_aafMatrix[ 3 ][ 2 ] = oVector.GetZ();

    m_oInverse.Set( 3, 0 ) = 0.0 - oVector.GetX();
    m_oInverse.Set( 3, 1 ) = 0.0 - oVector.GetY();
    m_oInverse.Set( 3, 2 ) = 0.0 - oVector.GetZ();

    return *this;
}

CTransform &CTransform::LookAt( CVector &oVector, CVector &oUp, CVector &oRight )
{
    Identity();
    m_oInverse.Identity();

    m_aafMatrix[ 0 ][ 0 ] = oRight.GetX();
    m_aafMatrix[ 0 ][ 1 ] = oRight.GetY();
    m_aafMatrix[ 0 ][ 2 ] = oRight.GetZ();
    m_aafMatrix[ 1 ][ 0 ] = oUp.GetX();
    m_aafMatrix[ 1 ][ 1 ] = oUp.GetY();
    m_aafMatrix[ 1 ][ 2 ] = oUp.GetZ();
    m_aafMatrix[ 2 ][ 0 ] = oVector.GetX();
    m_aafMatrix[ 2 ][ 1 ] = oVector.GetY();
    m_aafMatrix[ 2 ][ 2 ] = oVector.GetZ();

    m_oInverse = Transpose();
    return *this;
}

CTransform &CTransform::operator *( CTransform &oMultiplier )
{
    CMatrix     *pMatrix1 = (CMatrix *)this;
    CMatrix     *pMatrix2 = (CMatrix *)&oMultiplier;

    *pMatrix1 = (*pMatrix1) * (*pMatrix2);
    m_oInverse = m_oInverse * oMultiplier.m_oInverse;

    return *this;
}
    
