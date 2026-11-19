#ifdef _WINDOWS
    #include "StdAfx.h"
    #include "resource.h"
    #include "CameraPropertiesDlg.h"
#else
    #include "Globals.h"
#endif

#include "Camera.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define CAMERA_VERSION      ((DWORD)1)

CCamera::CCamera( void )
{
    m_oPosition.Set( -2.0, 1.5, -7.0 );
    m_oPointAt.Set( 0.0, 0.0, 1.0 );
    m_oUp.Set( 0.0, 1.0, 0.0 );
    m_nVerticalAngle = 30.0 * RADIANS;
    m_nHorizontalAngle = 40.0 * RADIANS;
    m_nAspectRatio = 4.0 / 3.0;
    CalcRotate();
}

CCamera::CCamera( CVector oPosition, CVector oPointAt, FLT nVerticalAngle, FLT nHorizontalAngle, FLT nAspectRatio, CVector *poUp )
{
    m_oPosition = oPosition;
    m_oPointAt = oPointAt;
    m_nVerticalAngle = nVerticalAngle;
    m_nHorizontalAngle = nHorizontalAngle;
    if ( nAspectRatio == 0.0 )
        m_nAspectRatio = m_nHorizontalAngle / m_nVerticalAngle;
    else
        m_nAspectRatio = nAspectRatio;
    if ( poUp )
        m_oUp = *poUp;
    else
        m_oUp.Set( 0.0, 1.0, 0.0 );
    CalcRotate();
}

CCamera::CCamera( const CCamera &oSource )
{
    m_oPosition = oSource.m_oPosition;
    m_oPointAt = oSource.m_oPointAt;
    m_oUp = oSource.m_oUp;
    m_nVerticalAngle = oSource.m_nVerticalAngle;
    m_nHorizontalAngle = oSource.m_nHorizontalAngle;
    m_nAspectRatio = oSource.m_nAspectRatio;
    CalcRotate();
}

CCamera &CCamera::operator =( const CCamera &oSource )
{
    m_oPosition = oSource.m_oPosition;
    m_oPointAt = oSource.m_oPointAt;
    m_oUp = oSource.m_oUp;
    m_nVerticalAngle = oSource.m_nVerticalAngle;
    m_nHorizontalAngle = oSource.m_nHorizontalAngle;
    m_nAspectRatio = oSource.m_nAspectRatio;
    CalcRotate();
    return *this;
}


CCamera::~CCamera( void )
{
}

DYNAMIC_IMPLEMENTS( CCamera );

CStream &operator >>( CStream &oStream, CCamera &oCamera )
{
    DWORD       dwVersion;

    oStream >> dwVersion;

    switch ( dwVersion )
    {
        case CAMERA_VERSION:
            oStream >> oCamera.m_oPosition;
            oStream >> oCamera.m_oPointAt;
            oStream >> oCamera.m_oUp;
            oStream >> oCamera.m_nVerticalAngle;
            oStream >> oCamera.m_nHorizontalAngle;
            oStream >> oCamera.m_nAspectRatio;
            oCamera.CalcRotate();
            break;

        default:
            break;
    }
    return oStream;
}

CStream &operator <<( CStream &oStream, CCamera &oCamera )
{
    oStream << CAMERA_VERSION;
    oStream << oCamera.m_oPosition;
    oStream << oCamera.m_oPointAt;
    oStream << oCamera.m_oUp;
    oStream << oCamera.m_nVerticalAngle;
    oStream << oCamera.m_nHorizontalAngle;
    oStream << oCamera.m_nAspectRatio;

    return oStream;
}

#ifdef _INC_WINDOWS

void CCamera::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddObject( this, 12, "Camera", hUnder );
}

void CCamera::SetProperties( void )
{
    CCameraPropertiesDlg     oProp( &m_oPosition, &m_oPointAt );

    oProp.m_nCameraAngle = ( m_nHorizontalAngle < m_nVerticalAngle ) ? m_nHorizontalAngle : m_nVerticalAngle;

    oProp.m_nCameraAngle /= RADIANS;

    if ( oProp.DoModal() == IDOK )
    {
        oProp.m_nCameraAngle *= RADIANS;
        m_nVerticalAngle = oProp.m_nCameraAngle;
        m_nHorizontalAngle = oProp.m_nCameraAngle * m_nAspectRatio;
    }
    
    CalcRotate();
}

#endif    

void CCamera::CalcRotate( void )
{
    CVector oCenterRay( m_oPointAt - m_oPosition );
    CVector oAxisX( 1.0, 0.0, 0.0 );
    CVector oAxisY( 0.0, 1.0, 0.0 );
    CVector oAxisZ( 0.0, 0.0, 1.0 );

    oCenterRay.Normalize();
    m_nXRotate = -ASIN( oAxisY.Dot( oCenterRay ) );
    m_nYRotate = -ASIN( oAxisX.Dot( oCenterRay ) );

    if ( oAxisZ.Dot( oCenterRay ) < 0 )
        m_nYRotate += PI;
}
