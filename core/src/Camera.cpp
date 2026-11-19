#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "resource.h"
    #include "CameraPropertiesDlg.h"
#else
    #include "Globals.h"
#endif

#include "Camera.h"
#include "CenterSampler.h"
#include "ClipColorModel.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
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
    m_pSampler.reset( oSource.m_pSampler ? oSource.m_pSampler->Clone() : NULL );
    m_pColorModel.reset( oSource.m_pColorModel ? oSource.m_pColorModel->Clone() : NULL );
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
    if ( this != &oSource )
    {
        m_pSampler.reset( oSource.m_pSampler ? oSource.m_pSampler->Clone() : NULL );
        m_pColorModel.reset( oSource.m_pColorModel ? oSource.m_pColorModel->Clone() : NULL );
    }
    CalcRotate();
    return *this;
}


CCamera::~CCamera( void )
{
}

CPixelSampler &CCamera::GetSampler( void )
{
    if ( ! m_pSampler )
        m_pSampler.reset( new CCenterSampler );
    return *m_pSampler;
}

void CCamera::SetSampler( CPixelSampler *pSampler )
{
    m_pSampler.reset( pSampler );
}

CColorModel &CCamera::GetColorModel( void )
{
    if ( ! m_pColorModel )
        m_pColorModel.reset( new CClipColorModel );
    return *m_pColorModel;
}

void CCamera::SetColorModel( CColorModel *pColorModel )
{
    m_pColorModel.reset( pColorModel );
}

DYNAMIC_IMPLEMENTS( CCamera, "camera" );

CRayStream &operator >>( CRayStream &oStream, CCamera &oCamera )
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

CRayStream &operator <<( CRayStream &oStream, CCamera &oCamera )
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

#ifdef RAY_GUI

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

static CVector Cross( CVector &oA, CVector &oB )
{
    return CVector( ( oA.GetY() * oB.GetZ() ) - ( oA.GetZ() * oB.GetY() ),
                    ( oA.GetZ() * oB.GetX() ) - ( oA.GetX() * oB.GetZ() ),
                    ( oA.GetX() * oB.GetY() ) - ( oA.GetY() * oB.GetX() ) );
}

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

        // The viewing basis. The scenes use left-handed coordinates: looking
        // down +Z with +Y up, +X is to the right.
    m_oForward = oCenterRay;
    if ( m_oForward.Magnitude() == 0.0 )
        m_oForward = oAxisZ;            // the camera looks at its own position

    CVector oUp( m_oUp );
    m_oRight = Cross( oUp, m_oForward );
    if ( m_oRight.Magnitude() < 1e-9 )
    {
            // Looking straight along the up vector: let the top of the image
            // face +Z when looking down and -Z when looking up, so +X stays
            // to the right.
        oUp.Set( 0.0, 0.0, ( m_oForward.GetY() < 0.0 ) ? 1.0 : -1.0 );
        m_oRight = Cross( oUp, m_oForward );
    }
    m_oRight.Normalize();
    m_oTop = Cross( m_oForward, m_oRight );
    m_oTop.Normalize();
}

CVector CCamera::RayDirection( FLT nScreenX, FLT nScreenY, FLT nAspect )
{
    FLT     nHalfHeight = TAN( m_nVerticalAngle / 2.0 );
    FLT     nHalfWidth = nHalfHeight * nAspect;
    CVector oRight( m_oRight * ( nScreenX * nHalfWidth ) );
    CVector oTop( m_oTop * ( nScreenY * nHalfHeight ) );
    CVector oDirection( m_oForward + oRight );

    oDirection = oDirection + oTop;
    oDirection.Normalize();

    return oDirection;
}

    // A sampler or color model: just its name when it has no settings, else
    // an object with its "type" and settings.
template <class T> static void PutModel( IASLib::CObjectNode &oNode, const char *strKey, const T &oModel )
{
    IASLib::CObjectNode *pModel = new IASLib::CObjectNode;

    pModel->put( "type", oModel.GetName() );
    oModel.ToJson( *pModel );
    if ( pModel->size() == 1 )
    {
        delete pModel;
        oNode.put( strKey, oModel.GetName() );
    }
    else
        oNode.set( strKey, pModel );
}

    // Reads oNode[ strKey ], if it is there, as a T made by T::Create.
template <class T> static BOOL GetModel( const IASLib::CJsonNode &oNode, const char *strKey, T *&pModel, const char *strWhat )
{
    const IASLib::CJsonNode *pField = oNode.get( strKey );
    std::string              strType;

    pModel = NULL;
    if ( pField == NULL )
        return TRUE;

    RayJson::CWhere oWhere( strKey );

    if ( ! RayJson::GetType( *pField, strType ) )
        return FALSE;

    pModel = T::Create( strType.c_str() );
    if ( pModel == NULL )
        return RayJson::Fail( std::string( "unknown " ) + strWhat + " \"" + strType + "\"" );
    if ( ! pModel->FromJson( *pField ) )
    {
        delete pModel;
        pModel = NULL;
        return FALSE;
    }
    return TRUE;
}

void CCamera::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutVector( oNode, "lookAt", m_oPointAt );
    RayJson::PutVector( oNode, "up", m_oUp );
    RayJson::PutAngle( oNode, "verticalAngle", m_nVerticalAngle );
    RayJson::PutAngle( oNode, "horizontalAngle", m_nHorizontalAngle );
    RayJson::PutNumber( oNode, "aspectRatio", m_nAspectRatio );
    PutModel( oNode, "sampler", GetSampler() );
    PutModel( oNode, "colorModel", GetColorModel() );
}

BOOL CCamera::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "position", "lookAt", "up", "verticalAngle", "verticalAngleRadians",
                                              "horizontalAngle", "horizontalAngleRadians", "aspectRatio",
                                              "sampler", "colorModel", NULL };
    CPixelSampler  *pSampler;
    CColorModel    *pColorModel;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, TRUE ) ||
         ! RayJson::GetVector( oNode, "lookAt", m_oPointAt, TRUE ) ||
         ! RayJson::GetVector( oNode, "up", m_oUp, FALSE ) ||
         ! RayJson::GetAngle( oNode, "verticalAngle", m_nVerticalAngle, FALSE ) ||
         ! RayJson::GetAngle( oNode, "horizontalAngle", m_nHorizontalAngle, FALSE ) ||
         ! RayJson::GetNumber( oNode, "aspectRatio", m_nAspectRatio, FALSE ) )
        return FALSE;

    if ( ! ( m_nVerticalAngle > 0.0 ) || ! ( m_nVerticalAngle < PI ) )
        return RayJson::Fail( "verticalAngle must be between 0 and 180 degrees" );

    if ( ! GetModel( oNode, "sampler", pSampler, "sampler" ) )
        return FALSE;
    if ( pSampler )
        SetSampler( pSampler );

    if ( ! GetModel( oNode, "colorModel", pColorModel, "color model" ) )
        return FALSE;
    if ( pColorModel )
        SetColorModel( pColorModel );

    CalcRotate();
    return TRUE;
}
