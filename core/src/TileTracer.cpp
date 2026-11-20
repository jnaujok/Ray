/*************************************************************************
*   CTileTracer                                                          *
*                                                                        *
*   Traces one tile of an image. See TileTracer.h.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "TileTracer.h"
#include "Scene.h"
#include "PixelSampler.h"
#include "Rays.h"
#include "Color.h"

CTileTracer::CTileTracer( void ) :
    m_pColors( NULL )
{
    m_oTile.nLeft = m_oTile.nTop = m_oTile.nWidth = m_oTile.nHeight = 0;
}

long long CTileTracer::Trace( CScene &oScene, const STile &oTile, std::vector<double> &anColors )
{
    CCamera        &oCamera = oScene.GetCamera();
    CPixelSampler  &oSampler = oCamera.GetSampler();
    int             nChannels = oCamera.GetChannels();
    CRay            oRay;
    long long       nRays = 0;

    m_oTile = oTile;
    m_pColors = &anColors;
    anColors.assign( (size_t)( oTile.nWidth > 0 ? oTile.nWidth : 0 ) * (size_t)( oTile.nHeight > 0 ? oTile.nHeight : 0 ) * 3, 0.0 );

    oSampler.Begin( oTile.nLeft, oTile.nTop, oTile.nWidth, oTile.nHeight );
    while ( oSampler.NextRay( oScene, oRay ) )
    {
        CColor oColor;

        if ( nChannels == 1 )
        {
            oScene.TraceRay( oRay, oColor, NULL );
            if ( oRay.GetWeight() != 1.0 )
                oColor = oColor * oRay.GetWeight();
            nRays++;
        }
        else
        {
                // A chromatic lens sends each wavelength its own way: remake
                // the sampler's ray once for each channel, and keep only that
                // channel of what it sees.
            FLT anChannel[ 3 ];

            for ( int nChannel = 1; nChannel <= 3; nChannel++ )
            {
                CRay    oChannelRay;
                CColor  oSeen;

                oChannelRay.CreateRayAt( oScene, oRay.GetImageX(), oRay.GetImageY(), oRay.GetPixelX(), oRay.GetPixelY(), 1.0, nChannel );
                oScene.TraceRay( oChannelRay, oSeen, NULL );
                anChannel[ nChannel - 1 ] = ( ( nChannel == 1 ) ? oSeen.GetR() : ( nChannel == 2 ) ? oSeen.GetG() : oSeen.GetB() ) *
                                            oChannelRay.GetWeight();
                nRays++;
            }
            oColor.Set( anChannel[ 0 ], anChannel[ 1 ], anChannel[ 2 ] );
        }
        oSampler.PutSample( *this, oRay.GetPixelX(), oRay.GetPixelY(), oColor );
    }
    oSampler.End();
    m_pColors = NULL;
    return nRays;
}

BOOL CTileTracer::PutPixel( int nX, int nY, const CColor &oColor )
{
    int nColumn = nX - m_oTile.nLeft;
    int nRow = nY - m_oTile.nTop;

    if ( m_pColors == NULL || nColumn < 0 || nRow < 0 || nColumn >= m_oTile.nWidth || nRow >= m_oTile.nHeight )
        return FALSE;

    CColor  oCopy( oColor );
    size_t  nAt = ( ( (size_t)nRow * (size_t)m_oTile.nWidth ) + (size_t)nColumn ) * 3;

    (*m_pColors)[ nAt ] = (double)oCopy.GetR();
    (*m_pColors)[ nAt + 1 ] = (double)oCopy.GetG();
    (*m_pColors)[ nAt + 2 ] = (double)oCopy.GetB();
    return TRUE;
}
