/*************************************************************************
*   CExtrusion                                                           *
*                                                                        *
*   A flat shape pushed back into a solid. See Extrusion.h.              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Extrusion.h"
#include "Extruder.h"
#include "SceneJson.h"

#include <cmath>


DYNAMIC_IMPLEMENTS( CExtrusion, "extrusion" );

CExtrusion::CExtrusion( void ) :
    m_bSingle( TRUE ),
    m_bSmooth( FALSE ),
    m_nDepth( 1.0 ),
    m_nBevel( 0.0 ),
    m_nBackBevel( 0.0 ),
    m_nTaper( 1.0 ),
    m_nTwist( 0.0 ),
    m_oPosition( 0.0, 0.0, 0.0 ),
    m_oShapeRotation( 0.0, 0.0, 0.0 )
{
}

CExtrusion::~CExtrusion( void )
{
}


BOOL CExtrusion::Build( std::string &strError )
{
    double nExtent = 0.0;

    for ( const Profile::SPath &oPath : m_aoOutlines )
    {
        double n = Profile::Extent( oPath );

        nExtent = ( n > nExtent ) ? n : nExtent;
    }
    if ( ! ( nExtent > 0.0 ) )
    {
        strError = "has no size";
        return FALSE;
    }

    std::vector<Outline::Polygon> aoPolygons;
    Outline::SShape               oShape;

    for ( const Profile::SPath &oPath : m_aoOutlines )
    {
        Outline::Polygon oPolygon = Profile::Flatten( oPath, 0.001 * nExtent );

        if ( oPolygon.size() < 3 || std::fabs( Outline::SignedArea2( oPolygon ) ) <= 1e-12 * nExtent * nExtent )
        {
            strError = "has an outline with no area";
            return FALSE;
        }
        aoPolygons.push_back( oPolygon );
    }
    if ( ! Outline::Fill( aoPolygons, oShape, strError ) )
    {
        strError = "can't fill its outlines: they " + strError;
        return FALSE;
    }

    Extruder::SBuilder oOut;

    if ( m_nTaper != 1.0 || m_nTwist != 0.0 )
        Extruder::ExtrudeTapered( oOut, oShape, m_nDepth, m_nTaper, m_nTwist * PI / 180.0 );
    else
        Extruder::Extrude( oOut, oShape, m_nDepth, m_nBevel, m_nBackBevel );
    Extruder::Place( oOut, m_oShapeRotation, m_oPosition );
    if ( ! SetGeometry( oOut.aoVertices, oOut.anFaces, TRUE, TRUE, oOut.aoNormals, oOut.anUVs ) )
    {
        strError = "made a bad mesh";
        return FALSE;
    }
    return TRUE;
}

void CExtrusion::ToJson( IASLib::CObjectNode &oNode )
{
    if ( m_bSingle && m_aoOutlines.size() == 1 )
        Profile::Write( *oNode.putArray( "outline" ), m_aoOutlines[ 0 ] );
    else
    {
        IASLib::CArrayNode *pOutlines = oNode.putArray( "outlines" );

        for ( const Profile::SPath &oPath : m_aoOutlines )
            Profile::Write( *pOutlines->addArray(), oPath );
    }
    if ( m_bSmooth )
        RayJson::PutBool( oNode, "smooth", m_bSmooth );
    RayJson::PutNumber( oNode, "depth", m_nDepth );
    if ( m_nBevel > 0.0 )
        RayJson::PutNumber( oNode, "bevel", m_nBevel );
    if ( m_nBackBevel > 0.0 )
        RayJson::PutNumber( oNode, "backBevel", m_nBackBevel );
    if ( m_nTaper != 1.0 )
        RayJson::PutNumber( oNode, "taper", m_nTaper );
    if ( m_nTwist != 0.0 )
        RayJson::PutNumber( oNode, "twist", m_nTwist );
    RayJson::PutVector( oNode, "position", m_oPosition );
    if ( m_oShapeRotation.GetX() != 0.0 || m_oShapeRotation.GetY() != 0.0 || m_oShapeRotation.GetZ() != 0.0 )
        RayJson::PutVector( oNode, "rotation", m_oShapeRotation );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CExtrusion::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "outline", "outlines", "smooth", "depth", "bevel", "backBevel", "taper", "twist",
                                              "position", "rotation", "color", "materials", "bump", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetBool( oNode, "smooth", m_bSmooth, FALSE ) ||
         ! RayJson::GetNumber( oNode, "depth", m_nDepth, FALSE ) ||
         ! RayJson::GetNumber( oNode, "bevel", m_nBevel, FALSE ) ||
         ! RayJson::GetNumber( oNode, "backBevel", m_nBackBevel, FALSE ) ||
         ! RayJson::GetNumber( oNode, "taper", m_nTaper, FALSE ) ||
         ! RayJson::GetNumber( oNode, "twist", m_nTwist, FALSE ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, FALSE ) ||
         ! RayJson::GetVector( oNode, "rotation", m_oShapeRotation, FALSE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! m_oMaterials.FromJson( oNode, "materials" ) ||
         ! BumpFromJson( oNode ) )
        return FALSE;
    if ( ! ( m_nDepth > 0.0 ) )
        return RayJson::Fail( "depth must be more than 0" );
    if ( m_nBevel < 0.0 || m_nBackBevel < 0.0 || ! ( m_nBevel + m_nBackBevel < m_nDepth ) )
        return RayJson::Fail( "bevel and backBevel must be at least 0 and add up to less than the depth" );
    if ( ! ( m_nTaper > 0.0 ) )
        return RayJson::Fail( "taper must be more than 0" );
    if ( ( m_nTaper != 1.0 || m_nTwist != 0.0 ) && ( m_nBevel > 0.0 || m_nBackBevel > 0.0 ) )
        return RayJson::Fail( "bevel and backBevel can't be used with a taper or a twist" );

    const IASLib::CJsonNode *pOne = oNode.get( "outline" );
    const IASLib::CJsonNode *pMany = oNode.get( "outlines" );

    if ( ( pOne != NULL ) == ( pMany != NULL ) )
        return RayJson::Fail( "give either outline (one) or outlines (a list of them)" );
    m_aoOutlines.clear();
    m_bSingle = pOne ? TRUE : FALSE;
    if ( pOne )
    {
        RayJson::CWhere oWhere( "outline" );
        Profile::SPath  oPath;

        if ( ! Profile::Read( *pOne, true, m_bSmooth ? true : false, 3, oPath ) )
            return FALSE;
        m_aoOutlines.push_back( oPath );
    }
    else
    {
        RayJson::CWhere oWhere( "outlines" );

        if ( ! pMany->isArray() || pMany->size() < 1 )
            return RayJson::Fail( "must be a list of outlines" );
        for ( int n = 0; n < pMany->size(); n++ )
        {
            RayJson::CWhere oElement( "", n );
            Profile::SPath  oPath;

            if ( ! Profile::Read( *pMany->get( n ), true, m_bSmooth ? true : false, 3, oPath ) )
                return FALSE;
            m_aoOutlines.push_back( oPath );
        }
    }

    std::string strError;

    if ( ! Build( strError ) )
        return RayJson::Fail( "the extrusion " + strError );
    return TRUE;
}
