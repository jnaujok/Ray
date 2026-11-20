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
    m_oUVMap.nType = UVMAP_NONE;
    m_oUVMap.nAxis = 1;
    m_oUVMap.bCenterGiven = FALSE;
    m_oUVMap.anScale[ 0 ] = m_oUVMap.anScale[ 1 ] = 1.0;
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

BOOL CMesh::SetGeometry( const std::vector<CVector> &aoVertices, const std::vector<int> &anFaces, BOOL bSmooth, BOOL bDoubleSided,
                         const std::vector<CVector> &aoNormals, const std::vector<FLT> &anUVs )
{
    if ( ! aoNormals.empty() && aoNormals.size() != aoVertices.size() )
        return FALSE;
    if ( ! anUVs.empty() && anUVs.size() != aoVertices.size() * 2 )
        return FALSE;

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
    m_aoGivenNormals = aoNormals;
    m_anUVs = anUVs;

        // Each vertex's normal is the sum of the normals of the faces around
        // it, then made a unit vector.
    std::vector<CVector> aoSums( m_aoVertices.size(), CVector( 0.0, 0.0, 0.0 ) );
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
            CVector &oSum = aoSums[ m_anFaces[ nFace + nCorner ] ];
            oSum = oSum + pFace->GetSurfaceNormal();
        }
    }

    for ( size_t nIndex = 0; nIndex < aoSums.size(); nIndex++ )
        aoSums[ nIndex ].Normalize();

    const std::vector<CVector> &aoVertexNormals = m_aoGivenNormals.empty() ? aoSums : m_aoGivenNormals;

    if ( ! m_anUVs.empty() )
    {
        for ( size_t nKept = 0; nKept < m_apFaces.size(); nKept++ )
        {
            FLT anCorners[ 6 ];

            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                size_t nVertex = (size_t)m_anFaces[ anKeptFrom[ nKept ] + nCorner ];

                anCorners[ nCorner * 2 ] = m_anUVs[ nVertex * 2 ];
                anCorners[ ( nCorner * 2 ) + 1 ] = m_anUVs[ ( nVertex * 2 ) + 1 ];
            }
            m_apFaces[ nKept ]->SetCornerUVs( anCorners );
        }
    }

    if ( m_bSmooth )
    {
        for ( size_t nKept = 0; nKept < m_apFaces.size(); nKept++ )
        {
            size_t nFace = anKeptFrom[ nKept ];

            m_apFaces[ nKept ]->SetCornerNormals( aoVertexNormals[ m_anFaces[ nFace ] ], aoVertexNormals[ m_anFaces[ nFace + 1 ] ],
                                                  aoVertexNormals[ m_anFaces[ nFace + 2 ] ] );
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

    if ( m_anUVs.empty() && m_oUVMap.nType != UVMAP_NONE )
        MapUVs();
    return TRUE;
}

void CMesh::MapUVs( void )
{
        // For each axis, the two across it that u and v follow: along y, x
        // and z (a floor); along z, x and y (a wall); along x, z and y. Angles
        // about an axis run from the first toward the second, as on a sphere.
    static const int anAcross[ 3 ][ 2 ] = { { 2, 1 }, { 0, 2 }, { 0, 1 } };
    int     nA = m_oUVMap.nAxis;
    int     nB = anAcross[ nA ][ 0 ];
    int     nC = anAcross[ nA ][ 1 ];
    FLT     anMin[ 3 ] = { 1e300, 1e300, 1e300 };
    FLT     anMax[ 3 ] = { -1e300, -1e300, -1e300 };
    FLT     anCenter[ 3 ];
    FLT     anSize[ 3 ];

        // The vertices' own bounds (not the padded box rays are tested with).
    for ( size_t nIndex = 0; nIndex < m_aoVertices.size(); nIndex++ )
    {
        FLT anValue[ 3 ] = { m_aoVertices[ nIndex ].GetX(), m_aoVertices[ nIndex ].GetY(), m_aoVertices[ nIndex ].GetZ() };

        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            if ( anValue[ nAxis ] < anMin[ nAxis ] ) anMin[ nAxis ] = anValue[ nAxis ];
            if ( anValue[ nAxis ] > anMax[ nAxis ] ) anMax[ nAxis ] = anValue[ nAxis ];
        }
    }

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        anSize[ nAxis ] = ( anMax[ nAxis ] > anMin[ nAxis ] ) ? anMax[ nAxis ] - anMin[ nAxis ] : 1.0;
        anCenter[ nAxis ] = ( anMin[ nAxis ] + anMax[ nAxis ] ) / 2.0;
    }
    if ( m_oUVMap.bCenterGiven )
    {
        anCenter[ 0 ] = m_oUVMap.oCenter.GetX();
        anCenter[ 1 ] = m_oUVMap.oCenter.GetY();
        anCenter[ 2 ] = m_oUVMap.oCenter.GetZ();
    }

    for ( size_t nFace = 0; nFace < m_apFaces.size(); nFace++ )
    {
        CTriangle  *pFace = m_apFaces[ nFace ];
        FLT         anUVs[ 6 ];
        BOOL        abPole[ 3 ] = { FALSE, FALSE, FALSE };
        int         nProject = nA;      // for box: the axis the face looks along most

        if ( m_oUVMap.nType == UVMAP_BOX )
        {
            CVector &oNormal = pFace->GetSurfaceNormal();
            FLT      anNormal[ 3 ] = { ABS( oNormal.GetX() ), ABS( oNormal.GetY() ), ABS( oNormal.GetZ() ) };

            nProject = ( anNormal[ 0 ] >= anNormal[ 1 ] && anNormal[ 0 ] >= anNormal[ 2 ] ) ? 0 : ( anNormal[ 1 ] >= anNormal[ 2 ] ) ? 1 : 2;
        }

        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            CVector &oCorner = pFace->GetCorner( nCorner );
            FLT      anPoint[ 3 ] = { oCorner.GetX() - anCenter[ 0 ], oCorner.GetY() - anCenter[ 1 ], oCorner.GetZ() - anCenter[ 2 ] };
            FLT      nU = 0.0;
            FLT      nV = 0.0;

            switch ( m_oUVMap.nType )
            {
                case UVMAP_PLANAR:
                case UVMAP_BOX:
                {
                        // Projected along the axis onto the bounding box's
                        // side, 0 to 1 across it.
                    int nPB = anAcross[ nProject ][ 0 ];
                    int nPC = anAcross[ nProject ][ 1 ];

                    nU = ( anPoint[ nPB ] / anSize[ nPB ] ) + 0.5;
                    nV = ( anPoint[ nPC ] / anSize[ nPC ] ) + 0.5;
                    break;
                }

                case UVMAP_CYLINDRICAL:
                case UVMAP_SPHERICAL:
                {
                    FLT nAcross = SQRT( ( anPoint[ nB ] * anPoint[ nB ] ) + ( anPoint[ nC ] * anPoint[ nC ] ) );

                    abPole[ nCorner ] = ( nAcross < 1e-12 );
                    nU = 0.5 + ( (FLT)atan2( (double)anPoint[ nC ], (double)anPoint[ nB ] ) / ( 2.0 * PI ) );
                    if ( m_oUVMap.nType == UVMAP_CYLINDRICAL )
                        nV = ( anPoint[ nA ] / anSize[ nA ] ) + 0.5;
                    else
                    {
                        FLT nLength = SQRT( ( nAcross * nAcross ) + ( anPoint[ nA ] * anPoint[ nA ] ) );
                        nV = ( nLength > 0.0 ) ? 0.5 + ( ASIN( anPoint[ nA ] / nLength ) / PI ) : 0.5;
                    }
                    break;
                }
            }
            anUVs[ nCorner * 2 ] = nU;
            anUVs[ ( nCorner * 2 ) + 1 ] = nV;
        }

        if ( m_oUVMap.nType == UVMAP_CYLINDRICAL || m_oUVMap.nType == UVMAP_SPHERICAL )
        {
                // A corner on the axis has no angle of its own: give it the
                // others' average.
            FLT nSum = 0.0;
            int nCount = 0;

            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                if ( ! abPole[ nCorner ] )
                {
                    nSum += anUVs[ nCorner * 2 ];
                    nCount++;
                }
            }
                // A face across the seam (u wrapping from 1 to 0) is unwrapped
                // so its u values run on past 1 instead of back across the
                // whole texture.
            FLT nLow = 2.0, nHigh = -1.0;

            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                if ( abPole[ nCorner ] )
                    continue;
                if ( anUVs[ nCorner * 2 ] < nLow ) nLow = anUVs[ nCorner * 2 ];
                if ( anUVs[ nCorner * 2 ] > nHigh ) nHigh = anUVs[ nCorner * 2 ];
            }
            if ( nHigh - nLow > 0.5 )
            {
                nSum = 0.0;
                for ( int nCorner = 0; nCorner < 3; nCorner++ )
                {
                    if ( abPole[ nCorner ] )
                        continue;
                    if ( anUVs[ nCorner * 2 ] < 0.5 )
                        anUVs[ nCorner * 2 ] += 1.0;
                    nSum += anUVs[ nCorner * 2 ];
                }
            }
            for ( int nCorner = 0; nCorner < 3; nCorner++ )
            {
                if ( abPole[ nCorner ] && nCount > 0 )
                    anUVs[ nCorner * 2 ] = nSum / nCount;
            }
        }

        for ( int nCorner = 0; nCorner < 3; nCorner++ )
        {
            anUVs[ nCorner * 2 ] *= m_oUVMap.anScale[ 0 ];
            anUVs[ ( nCorner * 2 ) + 1 ] *= m_oUVMap.anScale[ 1 ];
        }
        pFace->SetCornerUVs( anUVs );
    }
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
    if ( ! m_aoGivenNormals.empty() )
    {
        IASLib::CArrayNode *pNormals = oNode.putArray( "normals" );

        for ( size_t nIndex = 0; nIndex < m_aoGivenNormals.size(); nIndex++ )
            RayJson::AddVector( *pNormals, m_aoGivenNormals[ nIndex ] );
    }
    if ( m_oUVMap.nType != UVMAP_NONE )
    {
        static const char *const astrTypes[] = { "none", "planar", "cylindrical", "spherical", "box" };
        static const char *const astrAxes[] = { "x", "y", "z" };
        IASLib::CObjectNode     *pMap = oNode.putObject( "uvMap" );

        pMap->put( "type", astrTypes[ m_oUVMap.nType ] );
        pMap->put( "axis", astrAxes[ m_oUVMap.nAxis ] );
        if ( m_oUVMap.bCenterGiven )
            RayJson::PutVector( *pMap, "center", m_oUVMap.oCenter );
        pMap->putArray( "scale" )->add( (double)m_oUVMap.anScale[ 0 ] )->add( (double)m_oUVMap.anScale[ 1 ] );
    }
    if ( ! m_anUVs.empty() )
    {
        IASLib::CArrayNode *pUVs = oNode.putArray( "uvs" );

        for ( size_t nIndex = 0; nIndex + 1 < m_anUVs.size(); nIndex += 2 )
            pUVs->addArray()->add( (double)m_anUVs[ nIndex ] )->add( (double)m_anUVs[ nIndex + 1 ] );
    }
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );

    for ( size_t nIndex = 0; nIndex < m_aoVertices.size(); nIndex++ )
        RayJson::AddVector( *pVertices, m_aoVertices[ nIndex ] );
    for ( size_t nIndex = 0; nIndex + 2 < m_anFaces.size(); nIndex += 3 )
        pFaces->addArray()->add( m_anFaces[ nIndex ] )->add( m_anFaces[ nIndex + 1 ] )->add( m_anFaces[ nIndex + 2 ] );
}

BOOL CMesh::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "vertices", "faces", "normals", "uvs", "uvMap", "smooth", "doubleSided", "color", "materials", NULL };
    std::vector<CVector>    aoVertices;
    std::vector<int>        anFaces;
    std::vector<CVector>    aoNormals;
    BOOL                    bGivenNormals = oNode.isObject() && ( oNode.get( "normals" ) != NULL );
        // Given normals are what smooth shading blends, so they make the
        // mesh smooth; saying "smooth": false with them is an error.
    BOOL                    bSmooth = bGivenNormals ? TRUE : m_bSmooth;
    BOOL                    bDoubleSided = m_bDoubleSided;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetBool( oNode, "smooth", bSmooth, FALSE ) ||
         ! RayJson::GetBool( oNode, "doubleSided", bDoubleSided, FALSE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! m_oMaterials.FromJson( oNode, "materials" ) )
        return FALSE;
    if ( bGivenNormals && ! bSmooth )
        return RayJson::Fail( "normals are only used by smooth shading, so \"smooth\" can't be false with them" );

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

    if ( bGivenNormals )
    {
        const IASLib::CJsonNode *pNormals = oNode.get( "normals" );
        RayJson::CWhere          oWhere( "normals" );

        if ( ! pNormals->isArray() || pNormals->size() != (int)aoVertices.size() )
            return RayJson::Fail( "must be an array of [x, y, z] normals, one for each of the " +
                                  std::to_string( (long)aoVertices.size() ) + " vertices" );
        aoNormals.resize( aoVertices.size() );
        for ( int nIndex = 0; nIndex < pNormals->size(); nIndex++ )
        {
            RayJson::CWhere oElement( "", nIndex );

            if ( ! RayJson::ToVector( *pNormals->get( nIndex ), aoNormals[ (size_t)nIndex ] ) )
                return FALSE;
            if ( aoNormals[ (size_t)nIndex ].Magnitude() == 0.0 )
                return RayJson::Fail( "a normal can't be [0, 0, 0]" );
            aoNormals[ (size_t)nIndex ].Normalize();
        }
    }

    std::vector<FLT>            anUVs;
    const IASLib::CJsonNode    *pUVs = oNode.get( "uvs" );

    if ( pUVs )
    {
        RayJson::CWhere oWhere( "uvs" );

        if ( ! pUVs->isArray() || pUVs->size() != (int)aoVertices.size() )
            return RayJson::Fail( "must be an array of [u, v] pairs, one for each of the " +
                                  std::to_string( (long)aoVertices.size() ) + " vertices" );
        anUVs.reserve( aoVertices.size() * 2 );
        for ( int nIndex = 0; nIndex < pUVs->size(); nIndex++ )
        {
            const IASLib::CJsonNode *pPair = pUVs->get( nIndex );

            if ( ! pPair->isArray() || pPair->size() != 2 || ! pPair->get( 0 )->isNumber() || ! pPair->get( 1 )->isNumber() )
            {
                RayJson::CWhere oElement( "", nIndex );
                return RayJson::Fail( "must be a [u, v] pair" );
            }
            anUVs.push_back( (FLT)pPair->get( 0 )->doubleValue() );
            anUVs.push_back( (FLT)pPair->get( 1 )->doubleValue() );
        }
    }

    const IASLib::CJsonNode *pMap = oNode.get( "uvMap" );

    if ( pMap )
    {
        static const char *const astrMapFields[] = { "axis", "center", "scale", NULL };
        static const char *const astrTypes[] = { "planar", "cylindrical", "spherical", "box" };
        RayJson::CWhere          oWhere( "uvMap" );
        std::string              strType;
        std::string              strAxis = "y";
        SUVMap                   oMap = m_oUVMap;

        if ( pUVs )
            return RayJson::Fail( "a mesh takes \"uvs\" or a \"uvMap\", not both" );
        if ( ! RayJson::GetString( *pMap, "type", strType, TRUE ) || ! RayJson::OnlyFields( *pMap, astrMapFields ) ||
             ! RayJson::GetString( *pMap, "axis", strAxis, FALSE ) )
            return FALSE;

        oMap.nType = UVMAP_NONE;
        for ( int nType = 0; nType < 4; nType++ )
        {
            if ( strType == astrTypes[ nType ] )
                oMap.nType = nType + 1;
        }
        if ( oMap.nType == UVMAP_NONE )
            return RayJson::Fail( "unknown uvMap type \"" + strType + "\" (planar, cylindrical, spherical or box)" );
        if ( strAxis != "x" && strAxis != "y" && strAxis != "z" )
            return RayJson::Fail( "axis must be \"x\", \"y\" or \"z\"" );
        oMap.nAxis = strAxis[ 0 ] - 'x';

        oMap.bCenterGiven = ( pMap->get( "center" ) != NULL );
        if ( ! RayJson::GetVector( *pMap, "center", oMap.oCenter, FALSE ) )
            return FALSE;

        const IASLib::CJsonNode *pScale = pMap->get( "scale" );

        if ( pScale )
        {
            RayJson::CWhere oScaleWhere( "scale" );

            if ( ! pScale->isArray() || pScale->size() != 2 || ! pScale->get( 0 )->isNumber() || ! pScale->get( 1 )->isNumber() )
                return RayJson::Fail( "must be an array of two numbers, [u, v]" );
            oMap.anScale[ 0 ] = (FLT)pScale->get( 0 )->doubleValue();
            oMap.anScale[ 1 ] = (FLT)pScale->get( 1 )->doubleValue();
        }
        m_oUVMap = oMap;
    }

    return SetGeometry( aoVertices, anFaces, bSmooth, bDoubleSided, aoNormals, anUVs );
}
