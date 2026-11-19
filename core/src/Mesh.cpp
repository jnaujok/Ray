#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "resource.h"
#else
    #include "Globals.h"
#endif

#include "Mesh.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CMesh, "mesh" );

CMesh::CMesh( void ) :
    m_bDoubleSided( FALSE ),
    m_bSmooth( FALSE ),
    m_pHitFace( NULL )
{
    m_oMaterials.Add( new CDiffuseMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CMesh::~CMesh( void )
{
    DeleteFaces();
}

void CMesh::DeleteFaces( void )
{
    for ( size_t nIndex = 0; nIndex < m_apFaces.size(); nIndex++ )
        delete m_apFaces[ nIndex ];
    m_apFaces.clear();
    m_pHitFace = NULL;
}

BOOL CMesh::SetGeometry( const std::vector<CVector> &aoVertices, const std::vector<int> &anFaces, BOOL bSmooth, BOOL bDoubleSided )
{
    for ( size_t nIndex = 0; nIndex < anFaces.size(); nIndex++ )
    {
        if ( anFaces[ nIndex ] < 0 || anFaces[ nIndex ] >= (int)aoVertices.size() )
            return FALSE;
    }

    DeleteFaces();
    m_aoVertices = aoVertices;
    m_anFaces = anFaces;
    m_bSmooth = bSmooth;
    m_bDoubleSided = bDoubleSided;

        // Each vertex's normal is the sum of the normals of the faces around
        // it, then made a unit vector.
    std::vector<CVector> aoNormals( m_aoVertices.size(), CVector( 0.0, 0.0, 0.0 ) );
    std::vector<size_t>  anKeptFrom;        // where each kept face's indices start

    for ( size_t nFace = 0; nFace + 2 < m_anFaces.size(); nFace += 3 )
    {
        CTriangle *pFace = new CTriangle( m_aoVertices[ m_anFaces[ nFace ] ], m_aoVertices[ m_anFaces[ nFace + 1 ] ],
                                          m_aoVertices[ m_anFaces[ nFace + 2 ] ], m_bDoubleSided, m_bSmooth, &m_oMaterials );

        if ( pFace->GetSurfaceNormal().Magnitude() == 0.0 )
        {
            delete pFace;       // its corners are in a line: it has no surface
            continue;
        }
        m_apFaces.push_back( pFace );
        anKeptFrom.push_back( nFace );
        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            CVector &oSum = aoNormals[ m_anFaces[ nFace + nCorner ] ];
            oSum = oSum + pFace->GetSurfaceNormal();
        }
    }

    for ( size_t nIndex = 0; nIndex < aoNormals.size(); nIndex++ )
        aoNormals[ nIndex ].Normalize();

    if ( m_bSmooth )
    {
        for ( size_t nKept = 0; nKept < m_apFaces.size(); nKept++ )
        {
            size_t nFace = anKeptFrom[ nKept ];

            m_apFaces[ nKept ]->SetCornerNormals( aoNormals[ m_anFaces[ nFace ] ], aoNormals[ m_anFaces[ nFace + 1 ] ],
                                                  aoNormals[ m_anFaces[ nFace + 2 ] ] );
        }
    }

        // The bounding box, a little larger than the vertices so rays that
        // graze an edge are not lost to rounding.
    if ( ! m_aoVertices.empty() )
    {
        FLT anMin[ 3 ] = { 1e300, 1e300, 1e300 };
        FLT anMax[ 3 ] = { -1e300, -1e300, -1e300 };

        for ( size_t nIndex = 0; nIndex < m_aoVertices.size(); nIndex++ )
        {
            FLT anValue[ 3 ] = { m_aoVertices[ nIndex ].GetX(), m_aoVertices[ nIndex ].GetY(), m_aoVertices[ nIndex ].GetZ() };

            for ( int nAxis = 0; nAxis < 3; nAxis++ )
            {
                if ( anValue[ nAxis ] < anMin[ nAxis ] ) anMin[ nAxis ] = anValue[ nAxis ];
                if ( anValue[ nAxis ] > anMax[ nAxis ] ) anMax[ nAxis ] = anValue[ nAxis ];
            }
        }
        FLT nMargin = 1e-6 * ( 1.0 + ABS( anMax[ 0 ] - anMin[ 0 ] ) + ABS( anMax[ 1 ] - anMin[ 1 ] ) + ABS( anMax[ 2 ] - anMin[ 2 ] ) );
        m_oMinimum.Set( anMin[ 0 ] - nMargin, anMin[ 1 ] - nMargin, anMin[ 2 ] - nMargin );
        m_oMaximum.Set( anMax[ 0 ] + nMargin, anMax[ 1 ] + nMargin, anMax[ 2 ] + nMargin );
    }
    return TRUE;
}

    // The slab test: whether the ray passes through the box before its
    // current closest hit.
BOOL CMesh::HitsBox( CRay &oRay )
{
    FLT anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
    FLT anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };
    FLT anMin[ 3 ] = { m_oMinimum.GetX(), m_oMinimum.GetY(), m_oMinimum.GetZ() };
    FLT anMax[ 3 ] = { m_oMaximum.GetX(), m_oMaximum.GetY(), m_oMaximum.GetZ() };
    FLT nNear = 0.0;
    FLT nFar = oRay.GetT();

    if ( m_apFaces.empty() )
        return FALSE;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        if ( anDirection[ nAxis ] == 0.0 )
        {
            if ( anOrigin[ nAxis ] < anMin[ nAxis ] || anOrigin[ nAxis ] > anMax[ nAxis ] )
                return FALSE;
            continue;
        }

        FLT nT1 = ( anMin[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];
        FLT nT2 = ( anMax[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];

        if ( nT1 > nT2 )
        {
            FLT nSwap = nT1;
            nT1 = nT2;
            nT2 = nSwap;
        }
        if ( nT1 > nNear )
            nNear = nT1;
        if ( nT2 < nFar )
            nFar = nT2;
        if ( nNear > nFar )
            return FALSE;
    }
    return TRUE;
}

BOOL CMesh::Intersect( CRay &oRay, CVector &oIntersect )
{
    return Intersect( oRay, oIntersect, NULL );
}

BOOL CMesh::Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore )
{
    CVector oHit;
    BOOL    bHit = FALSE;

    if ( pIgnore == this || ! HitsBox( oRay ) )
        return FALSE;

        // Each face only reports a hit closer than the ray's closest so far,
        // so the last face that reports one is the closest.
    for ( size_t nIndex = 0; nIndex < m_apFaces.size(); nIndex++ )
    {
        CTriangle *pFace = m_apFaces[ nIndex ];

        if ( pFace != pIgnore && pFace->Intersect( oRay, oHit ) )
        {
            bHit = TRUE;
            m_pHitFace = pFace;
            oIntersect = oHit;
        }
    }
    return bHit;
}

CColor CMesh::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
        // The scene shades the object whose Intersect hit closest, straight
        // after asking, so m_pHitFace is that hit's face. Read it before
        // shading, which traces more rays.
    CTriangle *pFace = m_pHitFace;

    if ( pFace == NULL )
        return CColor( 0, 0, 0 );
    return pFace->Shade( oScene, oRay, oIntersect );
}

#ifdef RAY_GUI
void CMesh::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hMesh;

    hMesh = pTree->AddObject( this, 24, "Mesh", hUnder );
    CObjects::MatToTree( pTree, hMesh );
}
#endif

void CMesh::ToJson( IASLib::CObjectNode &oNode )
{
    IASLib::CArrayNode *pVertices = oNode.putArray( "vertices" );
    IASLib::CArrayNode *pFaces = oNode.putArray( "faces" );

    RayJson::PutBool( oNode, "smooth", m_bSmooth );
    RayJson::PutBool( oNode, "doubleSided", m_bDoubleSided );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );

    for ( size_t nIndex = 0; nIndex < m_aoVertices.size(); nIndex++ )
        RayJson::AddVector( *pVertices, m_aoVertices[ nIndex ] );
    for ( size_t nIndex = 0; nIndex + 2 < m_anFaces.size(); nIndex += 3 )
        pFaces->addArray()->add( m_anFaces[ nIndex ] )->add( m_anFaces[ nIndex + 1 ] )->add( m_anFaces[ nIndex + 2 ] );
}

BOOL CMesh::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "vertices", "faces", "smooth", "doubleSided", "color", "materials", NULL };
    std::vector<CVector>    aoVertices;
    std::vector<int>        anFaces;
    BOOL                    bSmooth = m_bSmooth;
    BOOL                    bDoubleSided = m_bDoubleSided;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetBool( oNode, "smooth", bSmooth, FALSE ) ||
         ! RayJson::GetBool( oNode, "doubleSided", bDoubleSided, FALSE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! m_oMaterials.FromJson( oNode, "materials" ) )
        return FALSE;

    const IASLib::CJsonNode *pVertices = RayJson::Field( oNode, "vertices", TRUE );
    const IASLib::CJsonNode *pFaces = RayJson::Field( oNode, "faces", TRUE );

    if ( pVertices == NULL || pFaces == NULL )
        return FALSE;

    {
        RayJson::CWhere oWhere( "vertices" );

        if ( ! pVertices->isArray() )
            return RayJson::Fail( "must be an array of [x, y, z] arrays" );
        aoVertices.resize( (size_t)pVertices->size() );
        for ( int nIndex = 0; nIndex < pVertices->size(); nIndex++ )
        {
            RayJson::CWhere oElement( "", nIndex );

            if ( ! RayJson::ToVector( *pVertices->get( nIndex ), aoVertices[ (size_t)nIndex ] ) )
                return FALSE;
        }
    }

    {
        RayJson::CWhere oWhere( "faces" );

        if ( ! pFaces->isArray() )
            return RayJson::Fail( "must be an array of [a, b, c] vertex indices" );
        anFaces.reserve( (size_t)pFaces->size() * 3 );
        for ( int nIndex = 0; nIndex < pFaces->size(); nIndex++ )
        {
            RayJson::CWhere             oElement( "", nIndex );
            const IASLib::CJsonNode    &oFace = *pFaces->get( nIndex );

            if ( ! oFace.isArray() || oFace.size() != 3 )
                return RayJson::Fail( "must be an array of three vertex indices" );
            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                const IASLib::CJsonNode *pIndex = oFace.get( nCorner );
                double                   dIndex = ( pIndex && pIndex->isNumber() ) ? pIndex->doubleValue() : -1.0;

                if ( dIndex != floor( dIndex ) || dIndex < 0 || dIndex >= (double)aoVertices.size() )
                    return RayJson::Fail( "vertex indices must be whole numbers from 0 to " +
                                          std::to_string( (long)aoVertices.size() - 1 ) );
                anFaces.push_back( (int)dIndex );
            }
        }
    }

    return SetGeometry( aoVertices, anFaces, bSmooth, bDoubleSided );
}
