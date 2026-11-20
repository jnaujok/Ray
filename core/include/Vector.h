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

#include "Globals.h"

class CVector
{
    protected:
        FLT                 m_nX;
        FLT                 m_nY;
        FLT                 m_nZ;
        BOOL                m_bInside;
    public:
            // A small value type: everything but the rotations is inline, so
            // the compiler can keep vectors in registers.
                            CVector( void ) { m_nX = 0.0; m_nY = 0.0; m_nZ = 0.0; m_bInside = FALSE; }
                            CVector( FLT nX, FLT nY, FLT nZ ) { m_nX = nX; m_nY = nY; m_nZ = nZ; m_bInside = FALSE; }
                            CVector( const CVector &oSource )
                            {
                                m_nX = oSource.m_nX;
                                m_nY = oSource.m_nY;
                                m_nZ = oSource.m_nZ;
                                m_bInside = oSource.m_bInside;
                            }
                           ~CVector( void ) {}

        FLT                 GetX( void ) { return m_nX; };
        FLT                 GetY( void ) { return m_nY; };
        FLT                 GetZ( void ) { return m_nZ; };
        void                Set( FLT nX, FLT nY, FLT nZ, FLT nH = (FLT)1.0 )
                            {
                                FLT nDivisor = 1.0 / nH;

                                m_nX = nX / nDivisor;
                                m_nY = nY / nDivisor;
                                m_nZ = nZ / nDivisor;
                            }
        CVector             operator *( FLT nScalar )
                            {
                                CVector oResult( *this );

                                oResult.m_nX *= nScalar;
                                oResult.m_nY *= nScalar;
                                oResult.m_nZ *= nScalar;
                                return oResult;
                            }
            // Assignment copies the point, not the inside flag (as in 1997).
        CVector            &operator =( const CVector &oSource )
                            {
                                m_nX = oSource.m_nX;
                                m_nY = oSource.m_nY;
                                m_nZ = oSource.m_nZ;
                                return *this;
                            }
        CVector             operator -( CVector &oSubtract )
                            {
                                CVector oResult;

                                oResult.m_nX = m_nX - oSubtract.m_nX;
                                oResult.m_nY = m_nY - oSubtract.m_nY;
                                oResult.m_nZ = m_nZ - oSubtract.m_nZ;
                                return oResult;
                            }
        CVector             operator +( CVector &oAdd )
                            {
                                CVector oResult;

                                oResult.m_nX = m_nX + oAdd.m_nX;
                                oResult.m_nY = m_nY + oAdd.m_nY;
                                oResult.m_nZ = m_nZ + oAdd.m_nZ;
                                return oResult;
                            }
        void                Normalize( void )
                            {
                                FLT nMagnitude = SQRT( (m_nX * m_nX) + (m_nY * m_nY) + (m_nZ * m_nZ) );

                                if ( nMagnitude != 0.0 )
                                {
                                    m_nX /= nMagnitude;
                                    m_nY /= nMagnitude;
                                    m_nZ /= nMagnitude;
                                }
                            }
        FLT                 Dot( CVector &oDot ) { return (m_nX * oDot.m_nX) + ( m_nY * oDot.m_nY ) + ( m_nZ * oDot.m_nZ ); }
        FLT                 Magnitude( void ) { return SQRT( (m_nX * m_nX) + (m_nY * m_nY) + (m_nZ * m_nZ) ); }
            // The cross product this x oOther.
        CVector             Cross( CVector &oOther )
                            {
                                return CVector( ( m_nY * oOther.m_nZ ) - ( m_nZ * oOther.m_nY ),
                                                ( m_nZ * oOther.m_nX ) - ( m_nX * oOther.m_nZ ),
                                                ( m_nX * oOther.m_nY ) - ( m_nY * oOther.m_nX ) );
                            }
        void                Inside( BOOL bInside = FALSE ) { m_bInside = bInside; }
        BOOL                IsInside( void ) { return m_bInside; }
        void                Rotate( CVector &oRotate );
        void                Rotate( FLT nXRad, FLT nYRad, FLT nZRad );

};

#endif // __VECTOR_H__
