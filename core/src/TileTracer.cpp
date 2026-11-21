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
#include "Lens.h"
#include "Sampling.h"
#include "Spectrum.h"

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
    for ( ;; )
    {
            // With channels apart, the sampler's own ray is only where the
            // sample falls; it needn't go through the lens.
        oCamera.SetAimOnly( nChannels > 1 ? TRUE : FALSE );

        BOOL bMore = oSampler.NextRay( oScene, oRay );

        oCamera.SetAimOnly( FALSE );
        if ( ! bMore )
            break;

        CColor oColor;

        if ( nChannels == 1 )
        {
            oScene.TraceRay( oRay, oColor, NULL );
            if ( oRay.GetWeight() != 1.0 )
                oColor = oColor * oRay.GetWeight();
            nRays++;
        }
        else if ( oCamera.IsSpectral() )
        {
                // Traced spectrally, the lens is sent four single
                // wavelengths, one from each quarter of the range, each weighed
                // by how its wavelength looks (see Spectrum.h).
            FLT anSum[ 3 ] = { 0.0, 0.0, 0.0 };
            FLT nWhere = oRay.WavelengthOffset();

            for ( int nPart = 0; nPart < RaySpectrum::PARTS; nPart++ )
            {
                FLT     nNanometers;
                FLT     anWeight[ 3 ];

                RaySpectrum::PartWeights( nPart, nWhere, nNanometers, anWeight );

                int     nChannel = CLens::WavelengthChannel( nNanometers );
                CRay    oChannelRay;
                CColor  oSeen;

                oChannelRay.CreateRayAt( oScene, oRay.GetImageX(), oRay.GetImageY(), oRay.GetPixelX(), oRay.GetPixelY(), 1.0, nChannel );
                oChannelRay.CopySampleIndex( oRay );
                oScene.TraceRay( oChannelRay, oSeen, NULL );
                anSum[ 0 ] += oSeen.GetR() * anWeight[ 0 ] * oChannelRay.GetWeight();
                anSum[ 1 ] += oSeen.GetG() * anWeight[ 1 ] * oChannelRay.GetWeight();
                anSum[ 2 ] += oSeen.GetB() * anWeight[ 2 ] * oChannelRay.GetWeight();
                nRays++;
            }
            oColor.Set( anSum[ 0 ] / RaySpectrum::PARTS, anSum[ 1 ] / RaySpectrum::PARTS, anSum[ 2 ] / RaySpectrum::PARTS );
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
        if ( oCamera.GetClamp() > 0.0 )
            oColor = oCamera.ClampSample( oColor );
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
