// wxWidgets first: on Windows it must include <windows.h> with GDI (for
// OpenGL), before IASLib includes a lean one.
#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "Globals.h"
#include "MainFrame.h"

#include "PropertyPanel.h"
#include "SceneCanvas.h"
#include "SceneJson.h"

#include <wx/aboutdlg.h>
#include <wx/filedlg.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/statusbr.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace RayEditor;

namespace
{
    enum
    {
        ID_IMPORT = wxID_HIGHEST + 1,
        ID_ADD_ENVIRONMENT,
        ID_DUPLICATE,
        ID_WIREFRAME,
        ID_SCENE_CAMERA,
        ID_FRAME_ALL,
        ID_CAMERA_FROM_VIEW,
        ID_RENDER,
        ID_STOP_RENDER,
        ID_SAVE_RENDER,
        ID_ADD_FIRST = wxID_HIGHEST + 100
    };

    const char *SCENE_FILES = "Ray scenes (*.ray)|*.ray|JSON scenes (*.json)|*.json|All files|*";

        // What a tree item stands for.
    class CPartData : public wxTreeItemData
    {
        public:
            std::string     strPart;
            int             nMaterial;

                            CPartData( const std::string &strThePart, int nTheMaterial ) : strPart( strThePart ), nMaterial( nTheMaterial ) {}
    };

    int IndexIn( const std::string &strPart, const char *strList )
    {
        std::string strPrefix = std::string( strList ) + "/";

        if ( strPart.compare( 0, strPrefix.size(), strPrefix ) != 0 )
            return -1;
        return std::atoi( strPart.c_str() + strPrefix.size() );
    }

    IASLib::CArrayNode *Vector( const SVec3 &v )
    {
        IASLib::CArrayNode *pArray = new IASLib::CArrayNode;

        pArray->add( v.x )->add( v.y )->add( v.z );
        return pArray;
    }
}

CMainFrame::CMainFrame( void )
    : wxFrame( NULL, wxID_ANY, "Ray Editor", wxDefaultPosition, wxSize( 1400, 860 ) ), m_bFilling( false )
{
        // Menus.
    wxMenuBar  *pBar = new wxMenuBar;
    wxMenu     *pFile = new wxMenu;
    wxMenu     *pEdit = new wxMenu;
    wxMenu     *pAddObject = new wxMenu;
    wxMenu     *pAddLight = new wxMenu;
    wxMenu     *pView = new wxMenu;
    wxMenu     *pRender = new wxMenu;
    wxMenu     *pHelp = new wxMenu;
    int         nId = ID_ADD_FIRST;

    pFile->Append( wxID_NEW, "&New\tCtrl+N" );
    pFile->Append( wxID_OPEN, "&Open...\tCtrl+O" );
    pFile->Append( wxID_SAVE, "&Save\tCtrl+S" );
    pFile->Append( wxID_SAVEAS, "Save &As...\tCtrl+Shift+S" );
    pFile->AppendSeparator();
    pFile->Append( ID_IMPORT, "&Import Model (STL, OBJ)...\tCtrl+I" );
    pFile->AppendSeparator();
    pFile->Append( wxID_EXIT, "E&xit\tCtrl+Q" );

    for ( const std::string &strType : CEditorDocument::ObjectTypes() )
    {
        m_mapAddTypes[ nId ] = strType;
        pAddObject->Append( nId++, wxString::FromUTF8( strType.c_str() ).Capitalize() );
    }
    for ( const std::string &strType : CEditorDocument::LightTypes() )
    {
        m_mapAddTypes[ nId ] = strType;
        pAddLight->Append( nId++, wxString::FromUTF8( strType.c_str() ).Capitalize() );
    }
    pEdit->Append( wxID_UNDO, "&Undo\tCtrl+Z" );
    pEdit->Append( wxID_REDO, "&Redo\tCtrl+Y" );
    pEdit->AppendSeparator();
    pEdit->AppendSubMenu( pAddObject, "Add &Object" );
    pEdit->AppendSubMenu( pAddLight, "Add &Light" );
    pEdit->Append( ID_ADD_ENVIRONMENT, "Add &Environment (Sky)" );
    pEdit->AppendSeparator();
    pEdit->Append( ID_DUPLICATE, "D&uplicate\tCtrl+D" );
    pEdit->Append( wxID_DELETE, "&Delete\tDel" );

    pView->AppendCheckItem( ID_WIREFRAME, "&Wireframe Overlay\tW" );
    pView->AppendCheckItem( ID_SCENE_CAMERA, "Look Through Scene &Camera\tC" );
    pView->Append( ID_FRAME_ALL, "&Frame All\tF" );
    pView->AppendSeparator();
    pView->Append( ID_CAMERA_FROM_VIEW, "&Set Scene Camera to This View" );

    pRender->Append( ID_RENDER, "&Render View\tF5" );
    pRender->Append( ID_STOP_RENDER, "&Stop Render\tEsc" );
    pRender->Append( ID_SAVE_RENDER, "Save Render as &PNG..." );

    pHelp->Append( wxID_ABOUT, "&About Ray Editor" );

    pBar->Append( pFile, "&File" );
    pBar->Append( pEdit, "&Edit" );
    pBar->Append( pView, "&View" );
    pBar->Append( pRender, "&Render" );
    pBar->Append( pHelp, "&Help" );
    SetMenuBar( pBar );
    CreateStatusBar( 2 );

        // Tree | view | properties.
    wxSplitterWindow *pOuter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D );
    wxSplitterWindow *pInner = new wxSplitterWindow( pOuter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D );

    m_pTree = new wxTreeCtrl( pOuter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_SINGLE );
    m_pCanvas = new CSceneCanvas( pInner );
    m_pProperties = new CPropertyPanel( pInner, m_oDocument );
    pInner->SplitVertically( m_pCanvas, m_pProperties, -380 );
    pInner->SetSashGravity( 1.0 );
    pInner->SetMinimumPaneSize( 200 );
    pOuter->SplitVertically( m_pTree, pInner, 240 );
    pOuter->SetMinimumPaneSize( 120 );

    m_pCanvas->m_fnPicked = [ this ]( int nObject )
    {
        SelectPart( nObject >= 0 ? "objects/" + std::to_string( nObject ) : std::string() );
    };
    m_pCanvas->m_fnLeftSceneCamera = [ this ]() { GetMenuBar()->Check( ID_SCENE_CAMERA, false ); };
    m_pCanvas->m_fnStatus = [ this ]( const wxString &strText ) { SetStatusText( strText, 1 ); };
    m_pProperties->m_fnChanged = [ this ]()
    {
            // Rebuild after the grid's event has finished with its properties.
        CallAfter( [ this ]() { Rebuild( false ); } );
    };

    Bind( wxEVT_MENU, &CMainFrame::OnNew, this, wxID_NEW );
    Bind( wxEVT_MENU, &CMainFrame::OnOpen, this, wxID_OPEN );
    Bind( wxEVT_MENU, &CMainFrame::OnSave, this, wxID_SAVE );
    Bind( wxEVT_MENU, &CMainFrame::OnSaveAs, this, wxID_SAVEAS );
    Bind( wxEVT_MENU, &CMainFrame::OnImport, this, ID_IMPORT );
    Bind( wxEVT_MENU, &CMainFrame::OnQuit, this, wxID_EXIT );
    Bind( wxEVT_MENU, &CMainFrame::OnUndo, this, wxID_UNDO );
    Bind( wxEVT_MENU, &CMainFrame::OnRedo, this, wxID_REDO );
    Bind( wxEVT_MENU, &CMainFrame::OnAdd, this, ID_ADD_FIRST, nId - 1 );
    Bind( wxEVT_MENU, &CMainFrame::OnAddEnvironment, this, ID_ADD_ENVIRONMENT );
    Bind( wxEVT_MENU, &CMainFrame::OnDuplicate, this, ID_DUPLICATE );
    Bind( wxEVT_MENU, &CMainFrame::OnDelete, this, wxID_DELETE );
    Bind( wxEVT_MENU, &CMainFrame::OnWireframe, this, ID_WIREFRAME );
    Bind( wxEVT_MENU, &CMainFrame::OnSceneCamera, this, ID_SCENE_CAMERA );
    Bind( wxEVT_MENU, &CMainFrame::OnFrameAll, this, ID_FRAME_ALL );
    Bind( wxEVT_MENU, &CMainFrame::OnCameraFromView, this, ID_CAMERA_FROM_VIEW );
    Bind( wxEVT_MENU, &CMainFrame::OnRender, this, ID_RENDER );
    Bind( wxEVT_MENU, &CMainFrame::OnStopRender, this, ID_STOP_RENDER );
    Bind( wxEVT_MENU, &CMainFrame::OnSaveRender, this, ID_SAVE_RENDER );
    Bind( wxEVT_MENU, &CMainFrame::OnAbout, this, wxID_ABOUT );
    Bind( wxEVT_CLOSE_WINDOW, &CMainFrame::OnClose, this );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, wxID_SAVE );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, wxID_UNDO );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, wxID_REDO );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, ID_DUPLICATE );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, wxID_DELETE );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, ID_STOP_RENDER );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, ID_SAVE_RENDER );
    Bind( wxEVT_UPDATE_UI, &CMainFrame::OnUpdateUI, this, ID_ADD_ENVIRONMENT );
    m_pTree->Bind( wxEVT_TREE_SEL_CHANGED, &CMainFrame::OnTreeSelection, this );

    Rebuild( true );
    UpdateTitle();
    SetStatusText( "Drag to orbit, right-drag or Shift-drag to pan, wheel to zoom, click to select" );
}

void CMainFrame::OpenAtStart( const wxString &strPath )
{
    Open( strPath );
}

void CMainFrame::UpdateTitle( void )
{
    wxString strName = m_oDocument.Path().empty() ? wxString( "Untitled" ) : wxString::FromUTF8( m_oDocument.Path().c_str() );

    SetTitle( strName + ( m_oDocument.IsDirty() ? " *" : "" ) + " - Ray Editor" );
}

void CMainFrame::ReportError( const wxString &strWhat, const std::string &strWhy )
{
    wxMessageBox( strWhat + ":\n" + wxString::FromUTF8( strWhy.c_str() ), "Ray Editor", wxOK | wxICON_WARNING, this );
}

void CMainFrame::Rebuild( bool bFrame )
{
    std::string strSelected = m_pProperties->GetPart();

    m_bFilling = true;
    m_pTree->DeleteAllItems();

    wxTreeItemId oRoot = m_pTree->AddRoot( "Scene" );
    wxTreeItemId oObjects, oLights, oFound;

    for ( const SPart &oPart : m_oDocument.Parts() )
    {
        wxTreeItemId oUnder = oRoot;

        if ( IndexIn( oPart.strPart, "objects" ) >= 0 )
        {
            if ( ! oObjects.IsOk() )
                oObjects = m_pTree->AppendItem( oRoot, "Objects" );
            oUnder = oObjects;
        }
        else if ( IndexIn( oPart.strPart, "lights" ) >= 0 )
        {
            if ( ! oLights.IsOk() )
                oLights = m_pTree->AppendItem( oRoot, "Lights" );
            oUnder = oLights;
        }

        wxTreeItemId oItem = m_pTree->AppendItem( oUnder, wxString::FromUTF8( oPart.strLabel.c_str() ), -1, -1,
                                                  new CPartData( oPart.strPart, -1 ) );

        for ( size_t i = 0; i < oPart.astrMaterials.size(); i++ )
            m_pTree->AppendItem( oItem, wxString::FromUTF8( oPart.astrMaterials[ i ].c_str() ), -1, -1,
                                 new CPartData( oPart.strPart, (int)i ) );
        if ( oPart.strPart == strSelected )
            oFound = oItem;
    }
    if ( oObjects.IsOk() )
        m_pTree->Expand( oObjects );
    if ( oLights.IsOk() )
        m_pTree->Expand( oLights );
    if ( oFound.IsOk() )
        m_pTree->SelectItem( oFound );
    m_bFilling = false;

    if ( ! oFound.IsOk() && ! strSelected.empty() )
        m_pProperties->ShowPart( std::string() );
    else if ( oFound.IsOk() )
        m_pProperties->Reload();

    m_pCanvas->SetScene( m_oDocument.Scene(), bFrame );
    m_pCanvas->SetSelected( IndexIn( m_pProperties->GetPart(), "objects" ), IndexIn( m_pProperties->GetPart(), "lights" ) );
    UpdateTitle();
}

void CMainFrame::SelectPart( const std::string &strPart, int nMaterial )
{
    wxTreeItemIdValue oCookie;
    std::vector<wxTreeItemId> aoStack;

        // Find its item in the tree.
    aoStack.push_back( m_pTree->GetRootItem() );
    while ( ! aoStack.empty() )
    {
        wxTreeItemId oItem = aoStack.back();

        aoStack.pop_back();
        if ( ! oItem.IsOk() )
            continue;

        CPartData *pData = dynamic_cast<CPartData *>( m_pTree->GetItemData( oItem ) );

        if ( pData && pData->strPart == strPart && pData->nMaterial == nMaterial )
        {
            m_bFilling = true;
            m_pTree->SelectItem( oItem );
            m_pTree->EnsureVisible( oItem );
            m_bFilling = false;
            break;
        }
        for ( wxTreeItemId oChild = m_pTree->GetFirstChild( oItem, oCookie ); oChild.IsOk(); oChild = m_pTree->GetNextChild( oItem, oCookie ) )
            aoStack.push_back( oChild );
    }
    if ( strPart.empty() )
        m_pTree->UnselectAll();
    m_pProperties->ShowPart( strPart, nMaterial );
    m_pCanvas->SetSelected( IndexIn( strPart, "objects" ), IndexIn( strPart, "lights" ) );
}

std::string CMainFrame::SelectedPart( int *pnMaterial )
{
    wxTreeItemId oItem = m_pTree->GetSelection();
    CPartData   *pData = oItem.IsOk() ? dynamic_cast<CPartData *>( m_pTree->GetItemData( oItem ) ) : NULL;

    if ( pnMaterial )
        *pnMaterial = pData ? pData->nMaterial : -1;
    return pData ? pData->strPart : std::string();
}

void CMainFrame::OnTreeSelection( wxTreeEvent &oEvent )
{
    if ( m_bFilling )
        return;

    CPartData *pData = oEvent.GetItem().IsOk() ? dynamic_cast<CPartData *>( m_pTree->GetItemData( oEvent.GetItem() ) ) : NULL;
    std::string strPart = pData ? pData->strPart : std::string();

    m_pProperties->ShowPart( strPart, pData ? pData->nMaterial : -1 );
    m_pCanvas->SetSelected( IndexIn( strPart, "objects" ), IndexIn( strPart, "lights" ) );
}

bool CMainFrame::ConfirmDiscard( void )
{
    if ( ! m_oDocument.IsDirty() )
        return true;

    int nAnswer = wxMessageBox( "The scene has changes that aren't saved. Save them first?", "Ray Editor",
                                wxYES_NO | wxCANCEL | wxICON_QUESTION, this );

    if ( nAnswer == wxCANCEL )
        return false;
    if ( nAnswer == wxYES )
    {
        wxCommandEvent oEvent;

        OnSave( oEvent );
        return ! m_oDocument.IsDirty();
    }
    return true;
}

bool CMainFrame::Open( const wxString &strPath )
{
    std::string strError;

    m_pCanvas->StopRender();
    if ( ! m_oDocument.Load( std::string( strPath.ToUTF8() ), strError ) )
    {
        ReportError( "Can't open " + strPath, strError );
        return false;
    }
    m_pProperties->ShowPart( std::string() );
    Rebuild( true );
    return true;
}

bool CMainFrame::SaveTo( const wxString &strPath )
{
    std::string strError;

    if ( ! m_oDocument.Save( std::string( strPath.ToUTF8() ), strError ) )
    {
        ReportError( "Can't save " + strPath, strError );
        return false;
    }
    UpdateTitle();
    SetStatusText( "Saved " + strPath );
    return true;
}

void CMainFrame::OnNew( wxCommandEvent & )
{
    if ( ! ConfirmDiscard() )
        return;
    m_pCanvas->StopRender();
    m_oDocument.New();
    m_pProperties->ShowPart( std::string() );
    Rebuild( true );
}

void CMainFrame::OnOpen( wxCommandEvent & )
{
    if ( ! ConfirmDiscard() )
        return;

    wxFileDialog oDialog( this, "Open a scene", wxEmptyString, wxEmptyString, SCENE_FILES, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if ( oDialog.ShowModal() == wxID_OK )
        Open( oDialog.GetPath() );
}

void CMainFrame::OnSave( wxCommandEvent &oEvent )
{
    if ( m_oDocument.Path().empty() )
        OnSaveAs( oEvent );
    else
        SaveTo( wxString::FromUTF8( m_oDocument.Path().c_str() ) );
}

void CMainFrame::OnSaveAs( wxCommandEvent & )
{
    wxFileDialog oDialog( this, "Save the scene (.ray: an archive with its files; .json: plain JSON)", wxEmptyString,
                          "scene.ray", SCENE_FILES, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if ( oDialog.ShowModal() == wxID_OK )
        SaveTo( oDialog.GetPath() );
}

void CMainFrame::OnImport( wxCommandEvent & )
{
    wxFileDialog oDialog( this, "Import a model", wxEmptyString, wxEmptyString,
                          "Models (*.stl;*.obj)|*.stl;*.STL;*.obj;*.OBJ|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST );
    std::string  strError, strPart;

    if ( oDialog.ShowModal() != wxID_OK )
        return;
    if ( ! m_oDocument.ImportModel( std::string( oDialog.GetPath().ToUTF8() ), strPart, strError ) )
    {
        ReportError( "Can't import " + oDialog.GetPath(), strError );
        return;
    }
    Rebuild( false );
    SelectPart( strPart );
}

void CMainFrame::OnQuit( wxCommandEvent & )
{
    Close();
}

void CMainFrame::OnClose( wxCloseEvent &oEvent )
{
    if ( oEvent.CanVeto() && ! ConfirmDiscard() )
    {
        oEvent.Veto();
        return;
    }
    m_pCanvas->StopRender();
    Destroy();
}

void CMainFrame::OnUndo( wxCommandEvent & )
{
    if ( m_oDocument.Undo() )
        Rebuild( false );
}

void CMainFrame::OnRedo( wxCommandEvent & )
{
    if ( m_oDocument.Redo() )
        Rebuild( false );
}

void CMainFrame::OnAdd( wxCommandEvent &oEvent )
{
    std::string strList, strPart, strError;
    std::string strText = CEditorDocument::NewPartJson( m_mapAddTypes[ oEvent.GetId() ], strList );

    if ( strText.empty() )
        return;
    if ( ! m_oDocument.Add( strList, strText, strPart, strError ) )
    {
        ReportError( "Can't add it", strError );
        return;
    }
    Rebuild( false );
    SelectPart( strPart );
}

void CMainFrame::OnAddEnvironment( wxCommandEvent & )
{
    std::string strError;

    if ( ! m_oDocument.SetPart( "environment", "{ \"type\": \"sky\" }", strError ) )
    {
        ReportError( "Can't add a sky", strError );
        return;
    }
    Rebuild( false );
    SelectPart( "environment" );
}

void CMainFrame::OnDuplicate( wxCommandEvent & )
{
    std::string strNew, strError;

    if ( ! m_oDocument.Duplicate( SelectedPart(), strNew, strError ) )
    {
        ReportError( "Can't duplicate it", strError );
        return;
    }
    Rebuild( false );
    SelectPart( strNew );
}

void CMainFrame::OnDelete( wxCommandEvent & )
{
    std::string strError;

    if ( ! m_oDocument.Delete( SelectedPart(), strError ) )
    {
        ReportError( "Can't delete it", strError );
        return;
    }
    m_pProperties->ShowPart( std::string() );
    Rebuild( false );
}

void CMainFrame::OnWireframe( wxCommandEvent &oEvent )
{
    m_pCanvas->SetWireframe( oEvent.IsChecked() );
}

void CMainFrame::OnSceneCamera( wxCommandEvent &oEvent )
{
    m_pCanvas->SetSceneCamera( oEvent.IsChecked() );
}

void CMainFrame::OnFrameAll( wxCommandEvent & )
{
    GetMenuBar()->Check( ID_SCENE_CAMERA, false );
    m_pCanvas->SetSceneCamera( false );
    m_pCanvas->FrameAll();
}

void CMainFrame::OnCameraFromView( wxCommandEvent & )
{
    std::unique_ptr<IASLib::CJsonNode> pCamera( RayJson::Parse( m_oDocument.PartJson( "camera" ) ) );
    std::string                        strError;

    if ( ! pCamera || ! pCamera->isObject() )
        return;

    IASLib::CObjectNode       *pObject = static_cast<IASLib::CObjectNode *>( pCamera.get() );
    const CViewCamera         &oView = m_pCanvas->GetView();

    pObject->set( "position", Vector( oView.GetEye() ) );
    pObject->set( "lookAt", Vector( oView.GetTarget() ) );
    pObject->set( "up", Vector( oView.GetUp() ) );
    pObject->put( "verticalAngle", oView.GetFovY() );
    if ( ! m_oDocument.SetPart( "camera", RayJson::Write( *pObject ), strError ) )
    {
        ReportError( "Can't move the camera", strError );
        return;
    }
    Rebuild( false );
}

void CMainFrame::OnRender( wxCommandEvent & )
{
    wxString strError;

    if ( ! m_pCanvas->StartRender( strError ) )
        ReportError( "Can't render", std::string( strError.ToUTF8() ) );
}

void CMainFrame::OnStopRender( wxCommandEvent & )
{
    m_pCanvas->StopRender();
}

void CMainFrame::OnSaveRender( wxCommandEvent & )
{
    std::vector<unsigned char>  abPixels;
    int                         nWidth = 0, nHeight = 0;

    if ( ! m_pCanvas->GetRender( abPixels, nWidth, nHeight ) )
        return;

    wxFileDialog oDialog( this, "Save the render", wxEmptyString, "render.png", "PNG images (*.png)|*.png",
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if ( oDialog.ShowModal() != wxID_OK )
        return;

    wxImage oImage( nWidth, nHeight );

    memcpy( oImage.GetData(), abPixels.data(), abPixels.size() );
    if ( ! oImage.SaveFile( oDialog.GetPath(), wxBITMAP_TYPE_PNG ) )
        ReportError( "Can't save " + oDialog.GetPath(), "the image couldn't be written" );
}

void CMainFrame::OnAbout( wxCommandEvent & )
{
    wxAboutDialogInfo oInfo;

    oInfo.SetName( "Ray Editor" );
    oInfo.SetVersion( "2026-alpha-0.12" );
    oInfo.SetDescription( "The cross-platform scene editor for Ray, the distributed ray tracer of 1997, rebuilt on IASLib." );
    oInfo.SetCopyright( "(c) 1996-2026 Jeffrey R. Naujok. Apache License 2.0." );
    wxAboutBox( oInfo, this );
}

void CMainFrame::OnUpdateUI( wxUpdateUIEvent &oEvent )
{
    std::string strPart = ( oEvent.GetId() == ID_DUPLICATE || oEvent.GetId() == wxID_DELETE ) ? SelectedPart() : std::string();

    switch ( oEvent.GetId() )
    {
        case wxID_SAVE:             oEvent.Enable( m_oDocument.IsDirty() || m_oDocument.Path().empty() ); break;
        case wxID_UNDO:             oEvent.Enable( m_oDocument.CanUndo() ); break;
        case wxID_REDO:             oEvent.Enable( m_oDocument.CanRedo() ); break;
        case ID_DUPLICATE:          oEvent.Enable( IndexIn( strPart, "objects" ) >= 0 || IndexIn( strPart, "lights" ) >= 0 ); break;
        case wxID_DELETE:           oEvent.Enable( IndexIn( strPart, "objects" ) >= 0 || IndexIn( strPart, "lights" ) >= 0 || strPart == "environment" ); break;
        case ID_STOP_RENDER:        oEvent.Enable( m_pCanvas->IsRendering() || m_pCanvas->HasRender() ); break;
        case ID_SAVE_RENDER:        oEvent.Enable( m_pCanvas->HasRender() && ! m_pCanvas->IsRendering() ); break;
        case ID_ADD_ENVIRONMENT:    oEvent.Enable( m_oDocument.Scene().GetEnvironment() == NULL ); break;
        default:                    break;
    }
}
