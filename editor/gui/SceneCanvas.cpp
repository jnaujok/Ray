// wxWidgets first: on Windows it must include <windows.h> with GDI (for
// OpenGL), before IASLib includes a lean one.
#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "Globals.h"
#include "SceneCanvas.h"

#include "Camera.h"
#include "Lens.h"
#include "Scene.h"
#include "SceneJson.h"

#include <wx/dcclient.h>

#include <algorithm>
#include <cmath>

using namespace RayEditor;

namespace
{
    const int   CANVAS_ATTRIBUTES[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 24, 0 };

    double JsonNumber( const IASLib::CJsonNode &oNode, const char *strKey, double nDefault )
    {
        const IASLib::CJsonNode *pValue = oNode.isObject() ? oNode.get( strKey ) : NULL;

        return ( pValue && pValue->isNumber() ) ? pValue->asDouble( nDefault ) : nDefault;
    }
}

CSceneCanvas::CSceneCanvas( wxWindow *pParent )
    : wxGLCanvas( pParent, wxID_ANY, CANVAS_ATTRIBUTES, wxDefaultPosition, wxSize( 640, 480 ), wxFULL_REPAINT_ON_RESIZE ),
      m_bListsDirty( true ), m_bWireframe( false ), m_bSceneCamera( false ), m_nSelected( -1 ), m_nSelectedLight( -1 ),
      m_bDragging( false ), m_bMoved( false ), m_oTimer( this ), m_nShownVersion( 0 ), m_bShowRender( false ),
      m_nTexture( 0 ), m_nTextureWidth( 0 ), m_nTextureHeight( 0 ), m_pScene( NULL )
{
    m_anBackground[ 0 ] = 0.55f;
    m_anBackground[ 1 ] = 0.7f;
    m_anBackground[ 2 ] = 0.95f;
    m_pContext.reset( new wxGLContext( this ) );

    Bind( wxEVT_PAINT, &CSceneCanvas::OnPaint, this );
    Bind( wxEVT_SIZE, &CSceneCanvas::OnSize, this );
    Bind( wxEVT_LEFT_DOWN, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_LEFT_UP, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_RIGHT_DOWN, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_RIGHT_UP, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_MIDDLE_DOWN, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_MIDDLE_UP, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_MOTION, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_MOUSEWHEEL, &CSceneCanvas::OnMouse, this );
    Bind( wxEVT_TIMER, &CSceneCanvas::OnTimer, this );
    Bind( wxEVT_ERASE_BACKGROUND, []( wxEraseEvent & ) {} );
}

CSceneCanvas::~CSceneCanvas( void )
{
    m_oTimer.Stop();
    m_oJob.Stop();
}

wxSize CSceneCanvas::PixelSize( void ) const
{
    wxSize  oSize = GetClientSize();
    double  nScale = GetContentScaleFactor();

    return wxSize( std::max( 1, (int)std::lround( oSize.x * nScale ) ), std::max( 1, (int)std::lround( oSize.y * nScale ) ) );
}

void CSceneCanvas::SetScene( CScene &oScene, bool bFrame )
{
        // A render of the scene as it was is out of date.
    if ( m_bShowRender || m_oJob.IsRunning() )
        StopRender();
    m_pScene = &oScene;
    TessellateScene( oScene, m_oDrawing );
    m_bListsDirty = true;

    CColor &oBackground = oScene.GetBackground();

    m_anBackground[ 0 ] = (float)std::min( 1.0, std::max( 0.0, (double)oBackground.GetR() ) );
    m_anBackground[ 1 ] = (float)std::min( 1.0, std::max( 0.0, (double)oBackground.GetG() ) );
    m_anBackground[ 2 ] = (float)std::min( 1.0, std::max( 0.0, (double)oBackground.GetB() ) );

        // The scene's lights, for OpenGL's eight.
    m_aanLights.clear();
    for ( int i = 0; i < oScene.GetLightCount() && m_aanLights.size() < 8; i++ )
    {
        std::unique_ptr<IASLib::CJsonNode> pLight( RayJson::Parse( oScene.PartToJson( "lights/" + std::to_string( i ) ) ) );
        SVec3                              oPosition, oDirection, oColor( 1, 1, 1 );
        std::vector<float>                 anLight( 12, 0.0f );        // x y z w, r g b, spot x y z, cutoff, exponent

        if ( ! pLight )
            continue;
        JsonVector( pLight->get( "color" ), oColor );
        if ( JsonVector( pLight->get( "position" ), oPosition ) )
        {
            anLight[ 0 ] = (float)oPosition.x;
            anLight[ 1 ] = (float)oPosition.y;
            anLight[ 2 ] = (float)oPosition.z;
            anLight[ 3 ] = 1.0f;
        }
        else if ( JsonVector( pLight->get( "direction" ), oDirection ) )
        {
                // Sunlight: a light at infinity, back along its direction.
            anLight[ 0 ] = (float)-oDirection.x;
            anLight[ 1 ] = (float)-oDirection.y;
            anLight[ 2 ] = (float)-oDirection.z;
            anLight[ 3 ] = 0.0f;
        }
        else
            continue;
        anLight[ 4 ] = (float)std::min( 1.0, oColor.x );
        anLight[ 5 ] = (float)std::min( 1.0, oColor.y );
        anLight[ 6 ] = (float)std::min( 1.0, oColor.z );
        anLight[ 10 ] = 180.0f;
        if ( anLight[ 3 ] == 1.0f && JsonVector( pLight->get( "direction" ), oDirection ) )
        {
            anLight[ 7 ] = (float)oDirection.x;
            anLight[ 8 ] = (float)oDirection.y;
            anLight[ 9 ] = (float)oDirection.z;
            anLight[ 10 ] = (float)std::min( 90.0, JsonNumber( *pLight, "angle", 30.0 ) );
            anLight[ 11 ] = 2.0f;
        }
        m_aanLights.push_back( anLight );
    }

    if ( m_bSceneCamera )
        SetSceneCamera( true );
    else if ( bFrame )
        FrameAll();
    Refresh();
}

void CSceneCanvas::SetSelected( int nObject, int nLight )
{
    m_nSelected = nObject;
    m_nSelectedLight = nLight;
    Refresh();
}

void CSceneCanvas::SetSceneCamera( bool bOn )
{
    m_bSceneCamera = bOn;
    if ( bOn && m_pScene )
    {
        std::unique_ptr<IASLib::CJsonNode> pCamera( RayJson::Parse( m_pScene->PartToJson( "camera" ) ) );
        SVec3                              oEye, oTarget, oUp( 0, 1, 0 );

        if ( pCamera && JsonVector( pCamera->get( "position" ), oEye ) && JsonVector( pCamera->get( "lookAt" ), oTarget ) )
        {
            double  nFov = JsonNumber( *pCamera, "verticalAngle", 30.0 );
            CLens  *pLens = m_pScene->GetCamera().GetLens();

            JsonVector( pCamera->get( "up" ), oUp );
            if ( pLens && pLens->GetFocalLength() > 0.0 )
            {
                    // A real lens sees by its focal length and the sensor.
                const IASLib::CJsonNode *pLensNode = pCamera->get( "lens" );
                double                   nSensor = pLensNode ? JsonNumber( *pLensNode, "sensorWidth", 36.0 ) : 36.0;
                wxSize                   oSize = PixelSize();
                double                   nAspect = (double)oSize.x / (double)oSize.y;

                nFov = 2.0 * std::atan( ( nSensor / nAspect ) * 0.5 / pLens->GetFocalLength() ) * 180.0 / EDITOR_PI;
            }
            m_oView.Set( oEye, oTarget, oUp, nFov );
        }
    }
    ViewChanged();
}

void CSceneCanvas::FrameAll( void )
{
    if ( m_oDrawing.bHasBounds )
        m_oView.Frame( m_oDrawing.oMinimum, m_oDrawing.oMaximum );
    ViewChanged();
}

void CSceneCanvas::ViewChanged( void )
{
    if ( m_bShowRender || m_oJob.IsRunning() )
        StopRender();
    Refresh();
}

void CSceneCanvas::OnSize( wxSizeEvent &oEvent )
{
    if ( m_bShowRender || m_oJob.IsRunning() )
        StopRender();
    Refresh();
    oEvent.Skip();
}

void CSceneCanvas::BuildLists( void )
{
    if ( ! m_anLists.empty() )
        glDeleteLists( m_anLists[ 0 ], (GLsizei)m_anLists.size() );
    m_anLists.clear();
    if ( m_oDrawing.aoMeshes.empty() )
        return;

    GLuint nFirst = glGenLists( (GLsizei)m_oDrawing.aoMeshes.size() );

    for ( size_t i = 0; i < m_oDrawing.aoMeshes.size(); i++ )
    {
        const SDrawMesh &oMesh = m_oDrawing.aoMeshes[ i ];

        m_anLists.push_back( nFirst + (GLuint)i );
        glNewList( nFirst + (GLuint)i, GL_COMPILE );
        glEnableClientState( GL_VERTEX_ARRAY );
        glEnableClientState( GL_NORMAL_ARRAY );
        glVertexPointer( 3, GL_FLOAT, 0, oMesh.anPositions.data() );
        glNormalPointer( GL_FLOAT, 0, oMesh.anNormals.data() );
        glDrawArrays( GL_TRIANGLES, 0, (GLsizei)( oMesh.anPositions.size() / 3 ) );
        glDisableClientState( GL_NORMAL_ARRAY );
        glDisableClientState( GL_VERTEX_ARRAY );
        glEndList();
    }
    m_bListsDirty = false;
}

void CSceneCanvas::DrawMeshes( bool bTranslucent )
{
    for ( size_t i = 0; i < m_oDrawing.aoMeshes.size() && i < m_anLists.size(); i++ )
    {
        const SDrawMesh &oMesh = m_oDrawing.aoMeshes[ i ];
        bool             bClear = oMesh.bGhost || oMesh.anColor[ 3 ] < 1.0f;

        if ( bClear != bTranslucent )
            continue;

        float nAlpha = oMesh.bGhost ? std::min( 0.3f, oMesh.anColor[ 3 ] ) : oMesh.anColor[ 3 ];

        glColor4f( oMesh.anColor[ 0 ], oMesh.anColor[ 1 ], oMesh.anColor[ 2 ], nAlpha );
        glCallList( m_anLists[ i ] );
    }
}

void CSceneCanvas::OnPaint( wxPaintEvent & )
{
    wxPaintDC oDC( this );

    if ( ! IsShownOnScreen() )
        return;
    SetCurrent( *m_pContext );

    wxSize oSize = PixelSize();

    glViewport( 0, 0, oSize.x, oSize.y );
    glClearColor( m_anBackground[ 0 ], m_anBackground[ 1 ], m_anBackground[ 2 ], 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    if ( m_bShowRender && ! m_abRender.empty() )
    {
        DrawRender();
        SwapBuffers();
        return;
    }
    if ( m_bListsDirty )
        BuildLists();

        // How far the scene reaches sets the near and far planes.
    double  nSize = std::max( 1.0, ( m_oDrawing.oMaximum - m_oDrawing.oMinimum ).Length() );
    double  nDistance = ( m_oView.GetEye() - ( m_oDrawing.oMinimum + m_oDrawing.oMaximum ) * 0.5 ).Length();
    double  nNear = std::max( nSize * 1e-4, ( nDistance - nSize * 4.0 ) * 0.5 );
    double  nFar = nDistance + nSize * 8.0;
    double  anMatrix[ 16 ];

    if ( nNear > nDistance * 0.1 )
        nNear = std::max( 1e-3, nDistance * 0.1 );
    glMatrixMode( GL_PROJECTION );
    m_oView.Projection( (double)oSize.x / (double)oSize.y, nNear, nFar, anMatrix );
    glLoadMatrixd( anMatrix );
    glMatrixMode( GL_MODELVIEW );
    m_oView.View( anMatrix );
    glLoadMatrixd( anMatrix );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_LIGHTING );
    glEnable( GL_NORMALIZE );
    glEnable( GL_COLOR_MATERIAL );
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );

    GLfloat anAmbient[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat anSpecular[] = { 0.25f, 0.25f, 0.25f, 1.0f };

    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, anAmbient );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, anSpecular );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 32.0f );
    for ( int i = 0; i < 8; i++ )
        glDisable( GL_LIGHT0 + i );
    if ( m_aanLights.empty() )
    {
            // No lights: a lamp at the eye.
        GLfloat anHead[] = { (GLfloat)m_oView.GetEye().x, (GLfloat)m_oView.GetEye().y, (GLfloat)m_oView.GetEye().z, 1.0f };
        GLfloat anWhite[] = { 0.8f, 0.8f, 0.8f, 1.0f };

        glEnable( GL_LIGHT0 );
        glLightfv( GL_LIGHT0, GL_POSITION, anHead );
        glLightfv( GL_LIGHT0, GL_DIFFUSE, anWhite );
        glLightfv( GL_LIGHT0, GL_SPECULAR, anWhite );
        glLightf( GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f );
    }
    for ( size_t i = 0; i < m_aanLights.size(); i++ )
    {
        const std::vector<float> &anLight = m_aanLights[ i ];
        GLenum                    nLight = (GLenum)( GL_LIGHT0 + (int)i );
        GLfloat                   anPosition[] = { anLight[ 0 ], anLight[ 1 ], anLight[ 2 ], anLight[ 3 ] };
        GLfloat                   anColor[] = { anLight[ 4 ], anLight[ 5 ], anLight[ 6 ], 1.0f };
        GLfloat                   anSpot[] = { anLight[ 7 ], anLight[ 8 ], anLight[ 9 ] };

        glEnable( nLight );
        glLightfv( nLight, GL_POSITION, anPosition );
        glLightfv( nLight, GL_DIFFUSE, anColor );
        glLightfv( nLight, GL_SPECULAR, anColor );
        glLightf( nLight, GL_SPOT_CUTOFF, anLight[ 10 ] );
        if ( anLight[ 10 ] < 180.0f )
        {
            glLightfv( nLight, GL_SPOT_DIRECTION, anSpot );
            glLightf( nLight, GL_SPOT_EXPONENT, anLight[ 11 ] );
        }
    }

        // Solid objects, then glass and csg parts, see-through.
    glEnable( GL_POLYGON_OFFSET_FILL );
    glPolygonOffset( 1.0f, 1.0f );
    DrawMeshes( false );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask( GL_FALSE );
    DrawMeshes( true );
    glDepthMask( GL_TRUE );
    glDisable( GL_POLYGON_OFFSET_FILL );

        // Lines: the wireframe, the selection, the icons.
    glDisable( GL_LIGHTING );
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    if ( m_bWireframe )
    {
        glColor4f( 0.1f, 0.1f, 0.15f, 0.6f );
        for ( size_t i = 0; i < m_anLists.size(); i++ )
        {
            if ( ! m_oDrawing.aoMeshes[ i ].bPlane )
                glCallList( m_anLists[ i ] );
        }
    }
    glLineWidth( 2.0f );
    glColor4f( 1.0f, 0.85f, 0.1f, 1.0f );
    for ( size_t i = 0; i < m_anLists.size(); i++ )
    {
        if ( m_oDrawing.aoMeshes[ i ].nObject == m_nSelected && m_nSelected >= 0 && ! m_oDrawing.aoMeshes[ i ].bPlane )
            glCallList( m_anLists[ i ] );
    }
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    glEnableClientState( GL_VERTEX_ARRAY );
    for ( const SDrawLines &oLines : m_oDrawing.aoLines )
    {
        bool bSelected = ( oLines.nLight >= 0 && oLines.nLight == m_nSelectedLight );

        glLineWidth( bSelected ? 3.0f : 1.5f );
        if ( bSelected )
            glColor4f( 1.0f, 0.85f, 0.1f, 1.0f );
        else
            glColor4fv( oLines.anColor );
        glVertexPointer( 3, GL_FLOAT, 0, oLines.anPoints.data() );
        glDrawArrays( GL_LINES, 0, (GLsizei)( oLines.anPoints.size() / 3 ) );
    }
    glDisableClientState( GL_VERTEX_ARRAY );
    glLineWidth( 1.0f );
    glDisable( GL_BLEND );

    SwapBuffers();
}

void CSceneCanvas::DrawRender( void )
{
    int nWidth = m_oJob.GetWidth(), nHeight = m_oJob.GetHeight();

    if ( (size_t)nWidth * (size_t)nHeight * 3 != m_abRender.size() )
        return;
    if ( m_nTexture == 0 )
        glGenTextures( 1, &m_nTexture );
    glBindTexture( GL_TEXTURE_2D, m_nTexture );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    if ( nWidth != m_nTextureWidth || nHeight != m_nTextureHeight )
    {
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, nWidth, nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_abRender.data() );
        m_nTextureWidth = nWidth;
        m_nTextureHeight = nHeight;
    }
    else
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, nWidth, nHeight, GL_RGB, GL_UNSIGNED_BYTE, m_abRender.data() );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_LIGHTING );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, 1.0, 0.0, 1.0, -1.0, 1.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glEnable( GL_TEXTURE_2D );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        // The image's rows run from the top.
    glBegin( GL_QUADS );
    glTexCoord2f( 0.0f, 1.0f ); glVertex2f( 0.0f, 0.0f );
    glTexCoord2f( 1.0f, 1.0f ); glVertex2f( 1.0f, 0.0f );
    glTexCoord2f( 1.0f, 0.0f ); glVertex2f( 1.0f, 1.0f );
    glTexCoord2f( 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
    glEnd();
    glDisable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, 0 );
}

void CSceneCanvas::OnMouse( wxMouseEvent &oEvent )
{
    wxPoint oPoint = oEvent.GetPosition();

    if ( oEvent.GetEventType() == wxEVT_MOUSEWHEEL )
    {
        int nSteps = oEvent.GetWheelRotation() / std::max( 1, oEvent.GetWheelDelta() );

        if ( nSteps != 0 )
        {
            if ( m_bSceneCamera )
            {
                m_bSceneCamera = false;
                if ( m_fnLeftSceneCamera )
                    m_fnLeftSceneCamera();
            }
            m_oView.Zoom( std::pow( 0.85, nSteps ) );
            ViewChanged();
        }
        return;
    }
    if ( oEvent.ButtonDown() )
    {
        m_oDragStart = m_oLast = oPoint;
        m_bDragging = true;
        m_bMoved = false;
        SetFocus();
        if ( ! HasCapture() )
            CaptureMouse();
        return;
    }
    if ( oEvent.ButtonUp() )
    {
        if ( HasCapture() )
            ReleaseMouse();
        if ( m_bDragging && ! m_bMoved && oEvent.GetButton() == wxMOUSE_BTN_LEFT && m_pScene )
        {
                // A click: what's under it?
            wxSize oClient = GetClientSize();
            int    nPicked = PickObject( *m_pScene, m_oView, oPoint.x, oPoint.y, oClient.x, oClient.y );

            if ( m_fnPicked )
                m_fnPicked( nPicked );
        }
        m_bDragging = false;
        return;
    }
    if ( oEvent.Dragging() && m_bDragging )
    {
        wxPoint oDelta = oPoint - m_oLast;

        if ( std::abs( oPoint.x - m_oDragStart.x ) + std::abs( oPoint.y - m_oDragStart.y ) > 3 )
            m_bMoved = true;
        if ( ! m_bMoved )
            return;
        if ( m_bSceneCamera )
        {
            m_bSceneCamera = false;
            if ( m_fnLeftSceneCamera )
                m_fnLeftSceneCamera();
        }

        wxSize oClient = GetClientSize();

        if ( oEvent.LeftIsDown() && ! oEvent.ShiftDown() )
            m_oView.Orbit( -oDelta.x * 0.4, oDelta.y * 0.4 );
        else
            m_oView.Pan( (double)oDelta.x / std::max( 1, oClient.y ), (double)oDelta.y / std::max( 1, oClient.y ) );
        m_oLast = oPoint;
        ViewChanged();
    }
}

bool CSceneCanvas::StartRender( wxString &strError )
{
    if ( ! m_pScene )
        return false;

    wxSize               oSize = PixelSize();
    CRenderJob::SRequest oRequest = CRenderJob::ForView( m_oView, oSize.x, oSize.y );
    std::string          strWhy;

    oRequest.bSceneCamera = m_bSceneCamera;
    m_abRender.assign( (size_t)oSize.x * (size_t)oSize.y * 3, 0 );
    m_nShownVersion = 0;
    if ( ! m_oJob.Start( *m_pScene, oRequest, strWhy ) )
    {
        strError = wxString::FromUTF8( strWhy.c_str() );
        return false;
    }
    m_bShowRender = true;
    m_oTimer.Start( 100 );
    if ( m_fnStatus )
        m_fnStatus( "Rendering: quick pass..." );
    Refresh();
    return true;
}

void CSceneCanvas::StopRender( void )
{
    bool bWas = m_bShowRender;

    m_oTimer.Stop();
    m_oJob.Stop();
    m_bShowRender = false;
    if ( bWas && m_fnStatus )
        m_fnStatus( "" );
    Refresh();
}

void CSceneCanvas::OnTimer( wxTimerEvent & )
{
    if ( m_oJob.GetVersion() != m_nShownVersion )
    {
        m_nShownVersion = m_oJob.GetVersion();
        m_oJob.CopyPixels( m_abRender );
        Refresh( false );
    }
    if ( ! m_oJob.IsRunning() )
    {
        m_oTimer.Stop();
        m_oJob.CopyPixels( m_abRender );
        if ( m_fnStatus )
        {
            std::string strError = m_oJob.GetError();

            m_fnStatus( m_oJob.IsDone() ? wxString( "Render finished" ) :
                        wxString( "Render stopped" ) + ( strError.empty() ? wxString() : ": " + wxString::FromUTF8( strError.c_str() ) ) );
        }
        Refresh( false );
    }
    else if ( m_fnStatus )
        m_fnStatus( m_oJob.GetPass() <= 1 ? "Rendering: quick pass..." : "Rendering: full pass..." );
}

bool CSceneCanvas::GetRender( std::vector<unsigned char> &abPixels, int &nWidth, int &nHeight )
{
    if ( ! m_bShowRender )
        return false;
    m_oJob.CopyPixels( abPixels );
    nWidth = m_oJob.GetWidth();
    nHeight = m_oJob.GetHeight();
    return ! abPixels.empty();
}
