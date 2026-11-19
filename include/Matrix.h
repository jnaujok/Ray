/*************************************************************************
*   CMatrix                                                              *
*                                                                        *
*       This is the base class for matrix math used for 3-d transforms.  *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "Objects.h"

class CMatrix
{
   protected:
      FLT             m_aafMatrix[4][4];
      int               m_nDummy;
   public:              
                        CMatrix( void );
                        CMatrix( BOOL bIdentity );
                        CMatrix( FLT x11, FLT x12, FLT x13, FLT x14,
                                 FLT x21, FLT x22, FLT x23, FLT x24,
                                 FLT x31, FLT x32, FLT x33, FLT x34,
                                 FLT x41, FLT x42, FLT x43, FLT x44 );
                        CMatrix( const CMatrix &oSource );
      virtual          ~CMatrix( void );

      virtual void      Zero( void );
      virtual void      Identity( void );
      virtual CMatrix  &operator =( const CMatrix &oSource );
      virtual CMatrix   operator *( const CMatrix &oMultiplier );
      virtual CMatrix   operator *( const FLT nScalar );
      virtual CMatrix   operator +( const CMatrix &oAdd );
      virtual CMatrix   operator -( const CMatrix &oSubtract );
      virtual CMatrix   Transpose( void );
      virtual FLT     Get( int nY, int nX ) 
                        { 
                            ASSERT( nY >= 0 );
                            ASSERT( nY < 4 );
                            ASSERT( nX >= 0 );
                            ASSERT( nX < 4 );

                            return m_aafMatrix[ nY ][nX]; 
                        }

      virtual FLT    &Set( int nY, int nX )
                        { 
                            ASSERT( nY >= 0 );
                            ASSERT( nY < 4 );
                            ASSERT( nX >= 0 );
                            ASSERT( nX < 4 );

                            return m_aafMatrix[ nY ][nX]; 
                        }
};

#endif // __MATRIX_H__

                      