#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Light.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CLight::CLight( void )
{
}

CLight::CLight( FLT nRed, FLT nGreen, FLT nBlue, FLT nX, FLT nY, FLT nZ ) :
m_oColor( nRed, nGreen, nBlue ), m_oPosition( nX, nY, nZ )
{
}

CLight::~CLight( void )
{
}