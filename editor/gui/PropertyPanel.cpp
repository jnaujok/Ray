// wxWidgets first: on Windows it must include <windows.h> with GDI (for
// OpenGL), before IASLib includes a lean one.
#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "Globals.h"
#include "PropertyPanel.h"

#include "SceneJson.h"
#include "Presets.h"

#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/props.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>

namespace
{
        // Lists longer than this, or of anything but numbers, vectors and
        // objects, are left to the JSON page.
    const int   MAX_LIST = 8;

    bool IsVector( const IASLib::CJsonNode &oNode )
    {
        if ( ! oNode.isArray() || oNode.size() != 3 )
            return false;
        for ( int i = 0; i < 3; i++ )
        {
            if ( ! oNode.get( i ) || ! oNode.get( i )->isNumber() )
                return false;
        }
        return true;
    }

    std::vector<std::string> SplitPath( const std::string &strPath )
    {
        std::vector<std::string> astrOut;
        std::stringstream        oStream( strPath );
        std::string              strItem;

        while ( std::getline( oStream, strItem, '/' ) )
            astrOut.push_back( strItem );
        return astrOut;
    }

    bool IsIndex( const std::string &strText )
    {
        if ( strText.empty() )
            return false;
        for ( char ch : strText )
        {
            if ( ch < '0' || ch > '9' )
                return false;
        }
        return true;
    }

        // A lone value as a node of its own (taken from a scratch array).
    template <class T> IASLib::CJsonNode *Value( T oValue )
    {
        IASLib::CArrayNode oScratch;

        oScratch.add( oValue );
        return oScratch.release( 0 );
    }
}

CPropertyPanel::CPropertyPanel( wxWindow *pParent, RayEditor::CEditorDocument &oDocument )
    : wxPanel( pParent ), m_oDocument( oDocument )
{
    wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

    m_pTitle = new wxStaticText( this, wxID_ANY, "Nothing selected" );
    m_pTitle->SetFont( m_pTitle->GetFont().Bold() );
    pSizer->Add( m_pTitle, 0, wxALL, 6 );

    m_pBook = new wxNotebook( this, wxID_ANY );

        // The property grid.
    m_pGrid = new wxPropertyGrid( m_pBook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED | wxPG_DEFAULT_STYLE );
    m_pBook->AddPage( m_pGrid, "Properties" );

        // The JSON text.
    wxPanel    *pJson = new wxPanel( m_pBook );
    wxBoxSizer *pJsonSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );

    m_pText = new wxTextCtrl( pJson, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL | wxTE_DONTWRAP );
    m_pText->SetFont( wxFont( wxFontInfo( 10 ).Family( wxFONTFAMILY_TELETYPE ) ) );
    pJsonSizer->Add( m_pText, 1, wxEXPAND | wxALL, 4 );

    wxButton *pApply = new wxButton( pJson, wxID_APPLY, "Apply" );
    wxButton *pRevert = new wxButton( pJson, wxID_REVERT_TO_SAVED, "Revert" );

    pButtons->Add( pApply, 0, wxRIGHT, 6 );
    pButtons->Add( pRevert, 0 );
    pJsonSizer->Add( pButtons, 0, wxALL, 4 );
    pJson->SetSizer( pJsonSizer );
    m_pBook->AddPage( pJson, "JSON" );

    pSizer->Add( m_pBook, 1, wxEXPAND | wxLEFT | wxRIGHT, 4 );

    m_pError = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_pError->SetForegroundColour( wxColour( 200, 30, 30 ) );
    pSizer->Add( m_pError, 0, wxEXPAND | wxALL, 6 );
    SetSizer( pSizer );

    m_pGrid->Bind( wxEVT_PG_CHANGED, &CPropertyPanel::OnGridChanged, this );
    pApply->Bind( wxEVT_BUTTON, &CPropertyPanel::OnApply, this );
    pRevert->Bind( wxEVT_BUTTON, &CPropertyPanel::OnRevert, this );
}

void CPropertyPanel::ShowError( const std::string &strError )
{
    m_pError->SetLabel( strError.empty() ? wxString() : wxString::FromUTF8( ( "Not changed: " + strError ).c_str() ) );
    m_pError->Wrap( (std::max)( 100, GetClientSize().x - 12 ) );
    Layout();
}

void CPropertyPanel::ShowPart( const std::string &strPart, int nMaterial )
{
    m_strPart = strPart;
    ShowError( std::string() );
    Fill();
    if ( nMaterial >= 0 )
    {
        wxPGProperty *pMaterial = m_pGrid->GetPropertyByName( wxString::Format( "materials/%d", nMaterial ) );

        if ( pMaterial )
        {
            m_pGrid->EnsureVisible( pMaterial );
            m_pGrid->SelectProperty( pMaterial );
        }
    }
}

void CPropertyPanel::Fill( void )
{
    m_pGrid->Clear();
    m_pText->Clear();
    if ( m_strPart.empty() )
    {
        m_pTitle->SetLabel( "Nothing selected" );
        return;
    }

    std::string                        strJson = m_oDocument.PartJson( m_strPart );
    std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( strJson ) );

    m_pTitle->SetLabel( wxString::FromUTF8( m_strPart.c_str() ) );
    m_pText->SetValue( wxString::FromUTF8( strJson.c_str() ) );
    if ( pNode && pNode->isObject() )
    {
        IASLib::CObjectNode *pObject = static_cast<IASLib::CObjectNode *>( pNode.get() );

        for ( int i = 0; i < pObject->size(); i++ )
            AddNode( NULL, pObject->fieldNameString( i ), pObject->fieldNameString( i ), *pObject->fieldValue( i ), 0 );
    }
    m_pGrid->FitColumns();
}

void CPropertyPanel::AddNode( wxPGProperty *pParent, const std::string &strPath, const std::string &strLabel,
                              const IASLib::CJsonNode &oNode, int nDepth )
{
    wxString        strName = wxString::FromUTF8( strPath.c_str() );
    wxString        strShown = wxString::FromUTF8( strLabel.c_str() );
    wxPGProperty   *pProperty = NULL;

    auto Append = [ & ]( wxPGProperty *pNew )
    {
        return pParent ? m_pGrid->AppendIn( pParent, pNew ) : m_pGrid->Append( pNew );
    };

    if ( oNode.isNumber() )
        pProperty = Append( new wxFloatProperty( strShown, strName, oNode.asDouble() ) );
    else if ( oNode.isBoolean() )
    {
        pProperty = Append( new wxBoolProperty( strShown, strName, oNode.asBoolean() ) );
        m_pGrid->SetPropertyAttribute( pProperty, wxPG_BOOL_USE_CHECKBOX, true );
    }
    else if ( oNode.isTextual() && strLabel == "preset" )
    {
            // A preset: picked from the library's list (or typed).
        wxPGChoices     oChoices;
        int             nCount;
        const RayPresets::SPreset *aoPresets = RayPresets::GetPresets( nCount );

        for ( int i = 0; i < nCount; i++ )
            oChoices.Add( wxString::FromUTF8( aoPresets[ i ].strName ) );
        pProperty = Append( new wxEditEnumProperty( strShown, strName, oChoices, wxString::FromUTF8( oNode.textString().c_str() ) ) );
    }
    else if ( oNode.isTextual() )
    {
        pProperty = Append( new wxStringProperty( strShown, strName, wxString::FromUTF8( oNode.textString().c_str() ) ) );
            // An object's kind is what it is; change it on the JSON page.
        if ( strLabel == "type" )
            m_pGrid->SetPropertyReadOnly( pProperty );
    }
    else if ( IsVector( oNode ) )
    {
        wxString strValue = wxString::Format( "%g, %g, %g", oNode.get( 0 )->asDouble(), oNode.get( 1 )->asDouble(), oNode.get( 2 )->asDouble() );
        bool     bColor = ( strLabel.find( "olor" ) != std::string::npos || strLabel == "zenith" || strLabel == "horizon" ||
                            strLabel == "ground" || strLabel == "background" );
        const char *astrAxes[] = { "x", "y", "z" };
        const char *astrColors[] = { "red", "green", "blue" };

        pProperty = Append( new wxStringProperty( strShown, strName, "<composed>" ) );
        for ( int i = 0; i < 3; i++ )
            m_pGrid->AppendIn( pProperty, new wxFloatProperty( bColor ? astrColors[ i ] : astrAxes[ i ], strName + wxString::Format( "/%d", i ),
                                                              oNode.get( i )->asDouble() ) );
        m_pGrid->Collapse( pProperty );
        (void)strValue;
    }
    else if ( oNode.isObject() && nDepth < 4 )
    {
        const IASLib::CObjectNode &oObject = static_cast<const IASLib::CObjectNode &>( oNode );

        pProperty = Append( new wxPropertyCategory( strShown, strName ) );
        for ( int i = 0; i < oObject.size(); i++ )
            AddNode( pProperty, strPath + "/" + oObject.fieldNameString( i ), oObject.fieldNameString( i ), *oObject.fieldValue( i ), nDepth + 1 );
    }
    else if ( oNode.isArray() && oNode.size() <= MAX_LIST && nDepth < 4 )
    {
        pProperty = Append( new wxPropertyCategory( strShown, strName ) );
        for ( int i = 0; i < oNode.size(); i++ )
        {
            const IASLib::CJsonNode *pItem = oNode.get( i );
            std::string              strItem = strLabel + " " + std::to_string( i + 1 );

            if ( pItem && pItem->isObject() && pItem->get( "type" ) && pItem->get( "type" )->isTextual() )
                strItem += ": " + pItem->get( "type" )->textString();
            if ( pItem )
                AddNode( pProperty, strPath + "/" + std::to_string( i ), strItem, *pItem, nDepth + 1 );
        }
    }
    else
    {
            // Too much for the grid (a mesh's vertices, a long list).
        wxString strSummary = oNode.isArray() ? wxString::Format( "%d items: edit on the JSON page", oNode.size() ) :
                                                wxString( "edit on the JSON page" );

        pProperty = Append( new wxStringProperty( strShown, strName, strSummary ) );
        m_pGrid->SetPropertyReadOnly( pProperty );
    }
}

bool CPropertyPanel::Apply( const std::string &strText )
{
    std::string strError;

    if ( ! m_oDocument.SetPart( m_strPart, strText, strError ) )
    {
        ShowError( strError );
        return false;
    }
    ShowError( std::string() );
    if ( m_fnChanged )
        m_fnChanged();
    return true;
}

void CPropertyPanel::OnGridChanged( wxPropertyGridEvent &oEvent )
{
    wxPGProperty *pProperty = oEvent.GetProperty();

    if ( ! pProperty || m_strPart.empty() )
        return;

    std::string                        strPath = pProperty->GetName().ToStdString();
    std::vector<std::string>           astrPath = SplitPath( strPath );
    std::unique_ptr<IASLib::CJsonNode> pRoot( RayJson::Parse( m_oDocument.PartJson( m_strPart ) ) );
    IASLib::CJsonNode                 *pParent = pRoot.get();

    if ( ! pRoot || astrPath.empty() )
        return;
    for ( size_t i = 0; pParent && i + 1 < astrPath.size(); i++ )
        pParent = IsIndex( astrPath[ i ] ) ? const_cast<IASLib::CJsonNode *>( pParent->get( std::stoi( astrPath[ i ] ) ) )
                                           : pParent->get( astrPath[ i ].c_str() );
    if ( ! pParent )
        return;

    wxVariant          oValue = pProperty->GetValue();
    IASLib::CJsonNode *pNew = NULL;

    if ( oValue.GetType() == "double" )
        pNew = Value( oValue.GetDouble() );
    else if ( oValue.GetType() == "long" )
        pNew = Value( (double)oValue.GetLong() );
    else if ( oValue.GetType() == "bool" )
        pNew = Value( oValue.GetBool() );
    else
        pNew = Value( std::string( oValue.GetString().ToUTF8() ) );

    const std::string &strLeaf = astrPath.back();

    if ( pParent->isArray() && IsIndex( strLeaf ) )
        static_cast<IASLib::CArrayNode *>( pParent )->set( std::stoi( strLeaf ), pNew );
    else if ( pParent->isObject() )
        static_cast<IASLib::CObjectNode *>( pParent )->set( strLeaf, pNew );
    else
    {
        delete pNew;
        return;
    }
    if ( ! Apply( RayJson::Write( *pRoot ) ) )
    {
            // Put the grid back as the scene has it, keeping the reason shown.
        wxString strShown = m_pError->GetLabel();

        CallAfter( [ this, strShown ]()
        {
            Fill();
            m_pError->SetLabel( strShown );
            Layout();
        } );
    }
    else
        CallAfter( [ this ]() { m_pText->SetValue( wxString::FromUTF8( m_oDocument.PartJson( m_strPart ).c_str() ) ); } );
}

void CPropertyPanel::OnApply( wxCommandEvent & )
{
    if ( m_strPart.empty() )
        return;
    if ( Apply( std::string( m_pText->GetValue().ToUTF8() ) ) )
        Fill();
}

void CPropertyPanel::OnRevert( wxCommandEvent & )
{
    ShowError( std::string() );
    Fill();
}
