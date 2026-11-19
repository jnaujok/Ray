/*************************************************************************
*   CVector                                                              *
*                                                                        *
*       Defines a mathematical vector which is a force with a direction. *
* In the ray-tracer it is used far more generically, including as a      *
* vector from the origin to the position of objects.                     *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Objects.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define VECTOR_VERSION      ((DWORD)1)

CVector::CVector( void )
{
    m_nX = 0.0;
    m_nY = 0.0;
    m_nZ = 0.0;
    m_bInside = FALSE;
}

CVector::CVector( FLT nX, FLT nY, FLT nZ )
{
    m_nX = nX;
    m_nY = nY;
    m_nZ = nZ;
    m_bInside = FALSE;
}

CVector::CVector( const CVector &oSource )
{
    m_nX = oSource.m_nX;
    m_nY = oSource.m_nY;
    m_nZ = oSource.m_nZ;
    m_bInside = oSource.m_bInside;
}

CVector::~CVector( void )
{
}

void CVector::Set( FLT nX, FLT nY, FLT nZ, FLT nH )
{
    FLT   nDivisor = 1.0 / nH;

    m_nX = nX / nDivisor;
    m_nY = nY / nDivisor;
    m_nZ = nZ / nDivisor;
}

CVector CVector::operator *( FLT nScalar )
{
    CVector     oResult( *this );

    oResult.m_nX *= nScalar;
    oResult.m_nY *= nScalar;
    oResult.m_nZ *= nScalar;

    return oResult;
}

CVector CVector::operator -( CVector &oSubtract )
{
    CVector     oResult;

    oResult.m_nX = m_nX - oSubtract.m_nX;
    oResult.m_nY = m_nY - oSubtract.m_nY;
    oResult.m_nZ = m_nZ - oSubtract.m_nZ;

    return oResult;
}

CVector CVector::operator +( CVector &oAdd )
{
    CVector     oResult;

    oResult.m_nX = m_nX + oAdd.m_nX;
    oResult.m_nY = m_nY + oAdd.m_nY;
    oResult.m_nZ = m_nZ + oAdd.m_nZ;

    return oResult;
}

CVector &CVector::operator =( const CVector &oSource )
{
    m_nX = oSource.m_nX;
    m_nY = oSource.m_nY;
    m_nZ = oSource.m_nZ;

    return *this;
}

FLT CVector::Dot( CVector &oDot )
{
    FLT     nResult;

    nResult = (m_nX * oDot.m_nX) + ( m_nY * oDot.m_nY ) + ( m_nZ * oDot.m_nZ );

    return nResult;
}

void CVector::Normalize( void )
{
    FLT     nMagnitude;

    nMagnitude = SQRT( (m_nX * m_nX) + (m_nY * m_nY) + (m_nZ * m_nZ) );

    if ( nMagnitude != 0.0 )
    {
        m_nX /= nMagnitude;
        m_nY /= nMagnitude;
        m_nZ /= nMagnitude;
    }
}

CVector CVector::Cross( CVector &oOther )
{
    return CVector( ( m_nY * oOther.m_nZ ) - ( m_nZ * oOther.m_nY ),
                    ( m_nZ * oOther.m_nX ) - ( m_nX * oOther.m_nZ ),
                    ( m_nX * oOther.m_nY ) - ( m_nY * oOther.m_nX ) );
}

FLT CVector::Magnitude( void )
{
    FLT     nMagnitude;

    nMagnitude = SQRT( (m_nX * m_nX) + (m_nY * m_nY) + (m_nZ * m_nZ) );

    return nMagnitude;
}

void CVector::Inside( BOOL bInside )
{
    m_bInside = bInside;
}

void CVector::Rotate( CVector &oRotate )
{
    FLT     nMatrix[3][3];
    FLT     cosX = COS( oRotate.GetX() );
    FLT     cosY = COS( oRotate.GetY() );
    FLT     cosZ = COS( oRotate.GetZ() );
    FLT     sinX = SIN( oRotate.GetX() );
    FLT     sinY = SIN( oRotate.GetY() );
    FLT     sinZ = SIN( oRotate.GetZ() );
    FLT     nX, nY, nZ;

    nMatrix[ 0 ][ 0 ] = (cosZ * cosY) + (sinZ * -sinX * -sinY);
    nMatrix[ 1 ][ 0 ] = (cosY * -sinZ) + (cosZ * -sinX * -sinY);
    nMatrix[ 2 ][ 0 ] = (cosX * sinY);

    nMatrix[ 0 ][ 1 ] = (cosX * sinZ);

    nMatrix[ 1 ][ 1 ] = (cosX * cosZ);

    nMatrix[ 2 ][ 1 ] = sinX;

    nMatrix[ 0 ][ 2 ] = ( -sinY * cosZ ) +
                        ( -sinX * cosY * sinZ );

    nMatrix[ 1 ][ 2 ] = ( -sinY * -sinZ ) +
                        ( -sinX * cosY * cosZ );

    nMatrix[ 2 ][ 2 ] = cosX * cosY;

    nX = (m_nX * nMatrix[ 0 ][ 0 ]) + (m_nY * nMatrix[ 0 ][ 1 ]) + ( m_nZ * nMatrix[ 0 ][ 2 ] );
    nY = (m_nX * nMatrix[ 1 ][ 0 ]) + (m_nY * nMatrix[ 1 ][ 1 ]) + ( m_nZ * nMatrix[ 1 ][ 2 ] );
    nZ = (m_nX * nMatrix[ 2 ][ 0 ]) + (m_nY * nMatrix[ 2 ][ 1 ]) + ( m_nZ * nMatrix[ 2 ][ 2 ] );

    m_nX = nX;
    m_nY = nY;
    m_nZ = nZ;
}

void CVector::Rotate( FLT nXRad, FLT nYRad, FLT nZRad )
{
    CVector oTemp( nXRad, nYRad, nZRad );

    Rotate( oTemp );
}
