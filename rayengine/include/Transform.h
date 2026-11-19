/*************************************************************************
*   CTransform                                                           *
*                                                                        *
*       Used in 3-d to move and adjust 3-d coordinate triplets.          *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "Objects.h"
#include "Matrix.h"
#include "Vector.h"


class CTransform : public CMatrix
{
   protected:
      CMatrix              m_oInverse;
   public:
                           CTransform( void );
                           CTransform( const CTransform &oSource );
      virtual             ~CTransform( void );
      
      virtual CTransform  &operator =( const CTransform &oSource );
      virtual CVector      Transform( CVector &oVector );
      virtual CVector      InverseTransform( CVector &oVector );
      virtual CTransform  &Scale( CVector &oVector );
      virtual CTransform  &Rotate( CVector &oVector );
      virtual CTransform  &Translate( CVector &oVector );
      virtual CTransform  &LookAt( CVector &oVector, CVector &oUp, CVector &oRight );
      virtual CTransform  &operator *( CTransform &oMultiplier );
};

#endif // __TRANSFORM_H__

