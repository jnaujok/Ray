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
