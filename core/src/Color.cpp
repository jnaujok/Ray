/*************************************************************************
*   CColor                                                               *
*                                                                        *
*       This class defines the behavior of colors in the ray tracer. This*
* class handles colors as they are used within the raytracer, and also   *
* includes the conversion to Windows RGB colorspace.                     *
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

#define COLOR_VERSION   ((DWORD)1)

CColor::CColor( FLT nRed, FLT nGreen, FLT nBlue )
{
    m_nRed = nRed;
    m_nGreen = nGreen;
    m_nBlue = nBlue;

}

CColor::CColor( const CColor &oSource )
{
    m_nRed = oSource.m_nRed;
    m_nGreen = oSource.m_nGreen;
    m_nBlue = oSource.m_nBlue;
}

CColor::CColor( CVector &oSource )
{
    m_nRed = oSource.GetX();
    m_nGreen = oSource.GetY();
    m_nBlue = oSource.GetZ();
}

CColor::~CColor( void )
{
}
      
FLT CColor::Distance( const CColor &oCompare )
{
    FLT nResult;

    nResult = ABS(m_nRed - oCompare.m_nRed) +
              ABS(m_nGreen - oCompare.m_nGreen ) +
              ABS(m_nBlue - oCompare.m_nBlue );

    return nResult;
}

CColor CColor::operator +( const CColor &oAdd )
{
    CColor  oTemp;

    oTemp.m_nRed = m_nRed + oAdd.m_nRed;
    oTemp.m_nGreen = m_nGreen + oAdd.m_nGreen;
    oTemp.m_nBlue = m_nBlue + oAdd.m_nBlue;

    return oTemp;
}

CColor CColor::operator *( FLT nScale )
{
    CColor  oTemp;

    oTemp.m_nRed = m_nRed * nScale;
    oTemp.m_nGreen = m_nGreen * nScale;
    oTemp.m_nBlue = m_nBlue * nScale;

    return oTemp;
}

CColor CColor::operator *( const CColor &oMult )
{
    CColor  oTemp;

    oTemp.m_nRed = m_nRed * oMult.m_nRed;
    oTemp.m_nGreen = m_nGreen * oMult.m_nGreen;
    oTemp.m_nBlue = m_nBlue * oMult.m_nBlue;

    return oTemp;
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


CColor &CColor::operator =( const CColor &oSource )
{
    m_nRed = oSource.m_nRed;
    m_nGreen = oSource.m_nGreen;
    m_nBlue = oSource.m_nBlue;

    return *this;
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


#ifdef RAY_GUI
CColor::operator COLORREF( )
{
    int     nRed;
    int     nGreen;
    int     nBlue;

    nRed = (int)(m_nRed * 255.0 );
    nGreen = (int)(m_nGreen * 255.0 );
    nBlue = (int)(m_nBlue * 255.0);

    return RGB( nRed, nGreen, nBlue );
}

CColor &CColor::operator =( COLORREF crSource )
{
    m_nRed = (FLT)GetRValue( crSource ) / (FLT)255.0;
    m_nGreen = (FLT)GetGValue( crSource ) / (FLT)255.0;
    m_nBlue = (FLT)GetBValue( crSource ) / (FLT)255.0;

    return *this;
}

#endif // RAY_GUI
