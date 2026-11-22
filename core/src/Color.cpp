/*************************************************************************
*   CColor                                                               *
*                                                                        *
*       This class defines the behavior of colors in the ray tracer. This*
* class handles colors as they are used within the raytracer, and also   *
* includes the conversion to Windows RGB colorspace.                     *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Objects.h"


#define COLOR_VERSION   ((DWORD)1)

CColor::CColor( CVector &oSource )
{
    m_nRed = oSource.GetX();
    m_nGreen = oSource.GetY();
    m_nBlue = oSource.GetZ();
}

      
FLT CColor::Distance( const CColor &oCompare )
{
    FLT nResult;

    nResult = ABS(m_nRed - oCompare.m_nRed) +
              ABS(m_nGreen - oCompare.m_nGreen ) +
              ABS(m_nBlue - oCompare.m_nBlue );

    return nResult;
}

BOOL CColor::operator ==( const CColor &oCompare )
{
    if ( ( m_nRed == oCompare.m_nRed ) &&
         ( m_nGreen == oCompare.m_nGreen ) &&
         ( m_nBlue == oCompare.m_nBlue ) )
        return TRUE;
    return FALSE;
}

BOOL CColor::operator !=( const CColor &oCompare )
{
    if ( ( m_nRed == oCompare.m_nRed ) &&
         ( m_nGreen == oCompare.m_nGreen ) &&
         ( m_nBlue == oCompare.m_nBlue ) )
        return FALSE;
    return TRUE;
}


CColor &CColor::operator =( CVector &oSource )
{
    m_nRed = oSource.GetX();
    m_nGreen = oSource.GetY();
    m_nBlue = oSource.GetZ();

    return *this;
}
    

void CColor::Set( FLT nRed, FLT nGreen, FLT nBlue, FLT nGamma )
{
    FLT     nInverseGamma = 1.0 / nGamma;

    if ( nGamma != 1.0 )
    {
        m_nRed = POW( nRed, nInverseGamma );
        m_nGreen = POW( nGreen, nInverseGamma );
        m_nBlue = POW( nBlue, nInverseGamma );
    }
    else
    {
        m_nRed = nRed;
        m_nGreen = nGreen;
        m_nBlue = nBlue;
    }
}

void CColor::Clip( void )
{
    if ( m_nRed > 1.0 )
        m_nRed = 1.0;

    if ( m_nGreen > 1.0 )
        m_nGreen = 1.0;

    if ( m_nBlue > 1.0 )
        m_nBlue = 1.0;
}


