/*************************************************************************
*   CCsg                                                                 *
*                                                                        *
*   Constructive solid geometry: union, intersection and difference of   *
*   closed shapes. See Csg.h.                                            *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Csg.h"
#include "Material.h"
#include "SceneJson.h"

#include <algorithm>
#include <math.h>
#include <memory>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CCsg, "csg" );

    // The most crossings asked of one shape along a ray.
#define CSG_MAX_CROSSINGS   256

static const char *const s_astrOperations[] = { "union", "intersection", "difference" };

CCsg::CCsg( void ) :
    m_nOperation( CSG_UNION ),
    m_bChildMaterials( FALSE ),
    m_oColor( 0.5, 0.5, 0.5 ),
    m_bPrepared( FALSE ),
    m_bHasBounds( FALSE ),
    m_nEpsilon( 1e-7 ),
    m_pHitLeaf( NULL ),
    m_bHitFlip( FALSE ),
    m_pShadeLeaf( NULL )
{
    m_oMaterials.Add( new CDiffuseMaterial, 1.0 );
}

CCsg::~CCsg( void )
{
    for ( size_t nIndex = 0; nIndex < m_apParts.size(); nIndex++ )
        delete m_apParts[ nIndex ];
}

void CCsg::Prepare( void )
{
    CVector oLow;
    CVector oHigh;
    CVector oAllLow;
    CVector oAllHigh;
    BOOL    bAll = TRUE;

    m_bPrepared = TRUE;
    m_bHasBounds = FALSE;
    for ( size_t nIndex = 0; nIndex < m_apParts.size(); nIndex++ )
    {
        if ( ! m_apParts[ nIndex ]->GetBounds( oLow, oHigh ) )
        {
            bAll = FALSE;
            continue;
        }
        if ( nIndex == 0 || ! m_bHasBounds )
        {
            oAllLow = oLow;
            oAllHigh = oHigh;
        }
        else
        {
            oAllLow.Set( fmin( oAllLow.GetX(), oLow.GetX() ), fmin( oAllLow.GetY(), oLow.GetY() ), fmin( oAllLow.GetZ(), oLow.GetZ() ) );
            oAllHigh.Set( fmax( oAllHigh.GetX(), oHigh.GetX() ), fmax( oAllHigh.GetY(), oHigh.GetY() ), fmax( oAllHigh.GetZ(), oHigh.GetZ() ) );
        }

            // The solid's own box: everything for a union, what every shape
            // shares for an intersection, the first shape for a difference.
        if ( ! m_bHasBounds )
        {
            m_oMinimum = oLow;
            m_oMaximum = oHigh;
        }
        else if ( m_nOperation == CSG_UNION )
        {
            m_oMinimum = oAllLow;
            m_oMaximum = oAllHigh;
        }
        else if ( m_nOperation == CSG_INTERSECTION )
        {
            m_oMinimum.Set( fmax( m_oMinimum.GetX(), oLow.GetX() ), fmax( m_oMinimum.GetY(), oLow.GetY() ), fmax( m_oMinimum.GetZ(), oLow.GetZ() ) );
            m_oMaximum.Set( fmin( m_oMaximum.GetX(), oHigh.GetX() ), fmin( m_oMaximum.GetY(), oHigh.GetY() ), fmin( m_oMaximum.GetZ(), oHigh.GetZ() ) );
        }
        m_bHasBounds = TRUE;
    }

        // An unbounded shape (none should be) leaves the solid unbounded,
        // except as a difference's subtracted shape.
    if ( ! bAll && ! ( m_nOperation == CSG_DIFFERENCE && m_apParts.size() > 0 && m_bHasBounds ) )
        m_bHasBounds = FALSE;
    if ( m_bHasBounds )
    {
            // An empty intersection still gets a box (a point), so the scene
            // can skip it.
        m_oMaximum.Set( fmax( m_oMaximum.GetX(), m_oMinimum.GetX() ), fmax( m_oMaximum.GetY(), m_oMinimum.GetY() ),
                        fmax( m_oMaximum.GetZ(), m_oMinimum.GetZ() ) );

        CVector oSize = oAllHigh - oAllLow;

        m_nEpsilon = 1e-7 * ( 1.0 + oSize.Magnitude() );
    }
    else
        m_nEpsilon = 1e-7;
}

BOOL CCsg::Inside( const std::vector<char> &abInside ) const
{
    size_t nParts = abInside.size();

    if ( nParts == 0 )
        return FALSE;
    if ( m_nOperation == CSG_UNION )
    {
        for ( size_t nIndex = 0; nIndex < nParts; nIndex++ )
            if ( abInside[ nIndex ] )
                return TRUE;
        return FALSE;
    }
    if ( m_nOperation == CSG_INTERSECTION )
    {
        for ( size_t nIndex = 0; nIndex < nParts; nIndex++ )
            if ( ! abInside[ nIndex ] )
                return FALSE;
        return TRUE;
    }
    if ( ! abInside[ 0 ] )
        return FALSE;
    for ( size_t nIndex = 1; nIndex < nParts; nIndex++ )
        if ( abInside[ nIndex ] )
            return FALSE;
    return TRUE;
}

    // Whether the ray's stretch from 0 to nLimit (in its own units) meets the
    // box at all.
static BOOL RayMeetsBox( const CVector &oLow, const CVector &oHigh, const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLimit )
{
    CVector oL( oLow );
    CVector oH( oHigh );
    FLT     anLow[ 3 ] = { oL.GetX(), oL.GetY(), oL.GetZ() };
    FLT     anHigh[ 3 ] = { oH.GetX(), oH.GetY(), oH.GetZ() };
    FLT     nNear = 0.0;
    FLT     nFar = nLimit;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        if ( anDirection[ nAxis ] == 0.0 )
        {
            if ( anOrigin[ nAxis ] < anLow[ nAxis ] || anOrigin[ nAxis ] > anHigh[ nAxis ] )
                return FALSE;
            continue;
        }

        FLT nT1 = ( anLow[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];
        FLT nT2 = ( anHigh[ nAxis ] - anOrigin[ nAxis ] ) / anDirection[ nAxis ];

        if ( nT1 > nT2 )
            std::swap( nT1, nT2 );
        if ( nT1 > nNear )
            nNear = nT1;
        if ( nT2 < nFar )
            nFar = nT2;
        if ( nNear > nFar )
            return FALSE;
    }
    return TRUE;
}

namespace
{
    struct SCrossing
    {
        FLT         nT;
        int         nPart;
        CObjects   *pLeaf;
        BOOL        bFlip;
    };
}

BOOL CCsg::Find( CRay &oRay, CVector &oIntersect, FLT nMinimum )
{
    if ( ! m_bPrepared )
        Prepare();
    if ( m_apParts.empty() )
        return FALSE;

    FLT     anOrigin[ 3 ] = { oRay.GetX(), oRay.GetY(), oRay.GetZ() };
    FLT     anDirection[ 3 ] = { oRay.GetI(), oRay.GetJ(), oRay.GetK() };
    FLT     nLength = SQRT( ( anDirection[ 0 ] * anDirection[ 0 ] ) + ( anDirection[ 1 ] * anDirection[ 1 ] ) +
                            ( anDirection[ 2 ] * anDirection[ 2 ] ) );
    FLT     nLimit = oRay.GetT();

    if ( ! ( nLength > 0.0 ) )
        return FALSE;
    if ( m_bHasBounds && ! RayMeetsBox( m_oMinimum, m_oMaximum, anOrigin, anDirection, nLimit ) )
        return FALSE;

    FLT                     nFrom = nMinimum / nLength;
    FLT                     nStep = m_nEpsilon / nLength;
    std::vector<SCrossing>  aoCrossings;
    std::vector<char>       abInside( m_apParts.size(), 0 );

    aoCrossings.reserve( m_apParts.size() * 2 );

        // Every crossing of every shape ahead of the ray's start: each found
        // by asking the shape again from just past the one before.
    for ( size_t nPart = 0; nPart < m_apParts.size(); nPart++ )
    {
        CObjects   *pPart = m_apParts[ nPart ];
        FLT         nAt = 0.0;
        int         nCount = 0;

        while ( nCount < CSG_MAX_CROSSINGS )
        {
            CRay    oProbe;
            CVector oHit;

            oProbe.Set( anOrigin[ 0 ] + ( anDirection[ 0 ] * nAt ), anOrigin[ 1 ] + ( anDirection[ 1 ] * nAt ),
                        anOrigin[ 2 ] + ( anDirection[ 2 ] * nAt ), anDirection[ 0 ], anDirection[ 1 ], anDirection[ 2 ],
                        1e100, 1.0, -1, -1 );
            if ( ! pPart->Intersect( oProbe, oHit ) )
                break;

            SCrossing   oCrossing;
            BOOL        bFlip = FALSE;

            oCrossing.nT = nAt + oProbe.GetT();
            oCrossing.nPart = (int)nPart;
            oCrossing.pLeaf = pPart->HitPart( bFlip );
            oCrossing.bFlip = bFlip;
            aoCrossings.push_back( oCrossing );
            nCount++;
            nAt = oCrossing.nT + nStep;
        }
            // An odd number of crossings ahead: the ray starts inside.
        abInside[ nPart ] = (char)( nCount & 1 );

            // A ray that never meets a shape an intersection needs (or a
            // difference's first) never meets the solid.
        if ( nCount == 0 && ( m_nOperation == CSG_INTERSECTION || ( m_nOperation == CSG_DIFFERENCE && nPart == 0 ) ) )
            return FALSE;
    }

    std::stable_sort( aoCrossings.begin(), aoCrossings.end(),
                      []( const SCrossing &oA, const SCrossing &oB ) { return oA.nT < oB.nT || ( oA.nT == oB.nT && oA.nPart < oB.nPart ); } );

    BOOL bInside = Inside( abInside );

    for ( size_t nIndex = 0; nIndex < aoCrossings.size(); nIndex++ )
    {
        const SCrossing &oCrossing = aoCrossings[ nIndex ];

        if ( oCrossing.nT >= nLimit )
            break;
        abInside[ (size_t)oCrossing.nPart ] ^= 1;

        BOOL bNow = Inside( abInside );

        if ( bNow == bInside )
            continue;
        bInside = bNow;
        if ( oCrossing.nT < nFrom )
            continue;

            // The solid's surface: the crossed shape's, inside out where it
            // is subtracted.
        BOOL bSubtracted = ( m_nOperation == CSG_DIFFERENCE && oCrossing.nPart > 0 ) ? TRUE : FALSE;

        m_pHitLeaf = oCrossing.pLeaf;
        m_bHitFlip = ( oCrossing.bFlip != bSubtracted ) ? TRUE : FALSE;
        oRay.SetT( oCrossing.nT );
        oIntersect.Set( anOrigin[ 0 ] + ( anDirection[ 0 ] * oCrossing.nT ), anOrigin[ 1 ] + ( anDirection[ 1 ] * oCrossing.nT ),
                        anOrigin[ 2 ] + ( anDirection[ 2 ] * oCrossing.nT ) );
        return TRUE;
    }
    return FALSE;
}

BOOL CCsg::Intersect( CRay &oRay, CVector &oIntersect )
{
    return Find( oRay, oIntersect, 0.0 );
}

BOOL CCsg::Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore )
{
    if ( pIgnore != this )
        return Find( oRay, oIntersect, 0.0 );
    if ( ! m_bPrepared )
        Prepare();
    return Find( oRay, oIntersect, m_nEpsilon * 10.0 );
}

BOOL CCsg::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    if ( ! m_bPrepared )
        Prepare();
    if ( ! m_bHasBounds )
        return FALSE;
    oMinimum = m_oMinimum;
    oMaximum = m_oMaximum;
    return TRUE;
}

BOOL CCsg::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    CObjects *pLeaf = m_pShadeLeaf ? m_pShadeLeaf : m_pHitLeaf;

    if ( pLeaf == NULL )
    {
        nU = nV = 0.0;
        return FALSE;
    }
    return pLeaf->SurfaceCoords( oPoint, nU, nV );
}

BOOL CCsg::SurfaceNormal( CVector &oPoint, CVector &oNormal )
{
    if ( m_pHitLeaf == NULL || ! m_pHitLeaf->SurfaceNormal( oPoint, oNormal ) )
        return FALSE;
    if ( m_bHitFlip )
        oNormal = oNormal * -1.0;
    return TRUE;
}

CColor CCsg::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
        // Read the hit before shading, which traces more rays (and may hit
        // this csg again).
    CObjects   *pLeaf = m_pHitLeaf;
    BOOL        bFlip = m_bHitFlip;
    CVector     oNormal;

    if ( pLeaf == NULL || ! pLeaf->SurfaceNormal( oIntersect, oNormal ) )
        return CColor( 0, 0, 0 );
    if ( bFlip )
        oNormal = oNormal * -1.0;

        // The shape being shaded gives surface coordinates (and, with
        // childMaterials, materials) until shading is done; a ray that hits
        // this csg again on the way puts back what it found.
    CObjects       *pOldLeaf = m_pShadeLeaf;
    CMaterialList  *pOldMaterials = m_pShadeMaterials;

    m_pShadeLeaf = pLeaf;
    if ( m_bChildMaterials )
        m_pShadeMaterials = &pLeaf->GetMaterials();

    CColor oColor = BaseShade( oScene, oRay, oIntersect, oNormal );

    m_pShadeLeaf = pOldLeaf;
    m_pShadeMaterials = pOldMaterials;
    return oColor;
}

#ifdef RAY_GUI
void CCsg::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hCsg;

    hCsg = pTree->AddObject( this, 8, "CSG", hUnder );
    CObjects::MatToTree( pTree, hCsg );
}
#endif

void CCsg::ToJson( IASLib::CObjectNode &oNode )
{
    oNode.put( "operation", s_astrOperations[ m_nOperation ] );

    IASLib::CArrayNode *pParts = oNode.putArray( "objects" );

    for ( size_t nIndex = 0; nIndex < m_apParts.size(); nIndex++ )
    {
        CObjects               *pPart = m_apParts[ nIndex ];
        IASLib::CObjectNode    *pNode = pParts->addObject();
        LPCSTR                  strType = g_pDynamicList->JsonTypeOf( pPart->ClassType() );

        pNode->put( "type", strType ? strType : pPart->ClassType() );
        pPart->ToJson( *pNode );
    }
    if ( m_bChildMaterials )
        RayJson::PutBool( oNode, "childMaterials", m_bChildMaterials );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CCsg::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "operation", "objects", "childMaterials", "color", "materials", "bump", NULL };
    std::string strOperation;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) || ! RayJson::GetString( oNode, "operation", strOperation, TRUE ) )
        return FALSE;

    int nOperation = -1;

    for ( int nIndex = 0; nIndex < 3; nIndex++ )
    {
        if ( strOperation == s_astrOperations[ nIndex ] )
            nOperation = nIndex;
    }
    if ( nOperation < 0 )
        return RayJson::Fail( "operation must be \"union\", \"intersection\" or \"difference\"" );

    const IASLib::CJsonNode *pArray = RayJson::Field( oNode, "objects", TRUE );

    if ( pArray == NULL )
        return FALSE;

    std::vector<CObjects *> apRead;
    BOOL                    bRead = TRUE;

    {
        RayJson::CWhere oWhere( "objects" );

        if ( ! pArray->isArray() || pArray->size() < 2 )
            bRead = RayJson::Fail( "must be an array of at least two objects" );
        for ( int nIndex = 0; bRead && nIndex < pArray->size(); nIndex++ )
        {
            RayJson::CWhere             oElement( "", nIndex );
            const IASLib::CJsonNode    &oPart = *pArray->get( nIndex );
            std::string                 strType;

            if ( ! RayJson::GetString( oPart, "type", strType, TRUE ) )
            {
                bRead = FALSE;
                break;
            }

            CObjects *pPart = g_pDynamicList->Create( strType.c_str() );

            if ( pPart == NULL || pPart->IsCamera() || pPart->IsALight() )
            {
                delete pPart;
                bRead = RayJson::Fail( "unknown object type \"" + strType + "\"" );
                break;
            }
            apRead.push_back( pPart );
            if ( ! pPart->FromJson( oPart ) )
                bRead = FALSE;
            else if ( ! pPart->IsClosed() )
                bRead = RayJson::Fail( "a csg combines only closed shapes: spheres, boxes, capped cylinders and cones, "
                                       "meshes and csgs, not \"" + strType + "\"" );
        }
    }

    if ( bRead )
    {
        bRead = RayJson::GetBool( oNode, "childMaterials", m_bChildMaterials, FALSE ) &&
                RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
                m_oMaterials.FromJson( oNode, "materials" ) &&
                BumpFromJson( oNode );
    }
    if ( ! bRead )
    {
        for ( size_t nIndex = 0; nIndex < apRead.size(); nIndex++ )
            delete apRead[ nIndex ];
        return FALSE;
    }

    for ( size_t nIndex = 0; nIndex < m_apParts.size(); nIndex++ )
        delete m_apParts[ nIndex ];
    m_apParts.swap( apRead );
    m_nOperation = nOperation;
    m_bPrepared = FALSE;
    m_pHitLeaf = NULL;
    m_pShadeLeaf = NULL;
    return TRUE;
}
