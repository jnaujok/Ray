/*************************************************************************
*   CVector                                                              *
*                                                                        *
*       Defines a mathematical vector which is a force with a direction. *
* In the ray-tracer it is used far more generically, including as a      *
* vector from the origin to the position of objects.                     *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "RayStream.h"

class CVector
{
    protected:
        FLT                 m_nX;
        FLT                 m_nY;
        FLT                 m_nZ;
        BOOL                m_bInside;
    public:
                            CVector( void );
                            CVector( FLT nX, FLT nY, FLT nZ );
                            CVector( const CVector &oSource );
        virtual            ~CVector( void );

        virtual FLT         GetX( void ) { return m_nX; };
        virtual FLT         GetY( void ) { return m_nY; };
        virtual FLT         GetZ( void ) { return m_nZ; };
        void                Set( FLT nX, FLT nY, FLT nZ, FLT nH = (FLT)1.0 );
        virtual CVector     operator *( FLT nScalar );
        CVector            &operator = ( const CVector &oSource );
        virtual CVector     operator -( CVector &oSubtract );
        virtual CVector     operator +( CVector &oAdd );
    	virtual void        Normalize( void );
        virtual FLT         Dot( CVector &oDot );
        virtual FLT         Magnitude( void );
        virtual void        Inside( BOOL bInside = FALSE );
        virtual BOOL        IsInside( void ) { return m_bInside; }
        virtual void        Rotate( CVector &oRotate );
        virtual void        Rotate( FLT nXRad, FLT nYRad, FLT nZRad );

        friend CRayStream  &operator >>( CRayStream &oStream, CVector &oVector );
        friend CRayStream  &operator <<( CRayStream &oStream, CVector &oVector );
};

#endif // __VECTOR_H__
