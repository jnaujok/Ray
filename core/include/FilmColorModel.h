/*************************************************************************
*   CFilmColorModel                                                      *
*                                                                        *
*   Photographic film: turns the linear colors a sampler worked out into *
*   what a slide or print of a real film stock shows. A stock has a      *
*   characteristic curve (density against the logarithm of exposure), a  *
*   spectral sensitivity for each of its layers (one for black and white *
*   film, three for color), grain, and a spread of light in the emulsion *
*   (irradiation, the scatter between the silver grains, and halation,   *
*   the light reflected back from the film base).                        *
*                                                                        *
*   Exposure comes from the film speed, the shutter time and the f-stop: *
*   a linear value v gets exposure v * 256 * shutter * ISO / fStop^2, so *
*   by the "sunny 16" rule (f/16 at 1 / ISO seconds) a value of 0.18     *
*   lands on the stock's mid gray. Ray's lenses keep an image as bright  *
*   as a pinhole whatever their stop, so the film's own fStop sets it.   *
*                                                                        *
*   Grain and the spread need the whole image, so they are applied once  *
*   it is finished (Finish), by whatever made the image: the master for  *
*   threads and networked engines alike. Until then each pixel is shown  *
*   through the curve alone.                                             *
*                                                                        *
*   The stocks are approximations of the Kodak films flown on Apollo,    *
*   from their published speeds and kinds; their curves, grain and       *
*   spread are representative values, not measured data. The renderer's *
*   colors are red, green and blue, so a layer's spectral sensitivity is *
*   a mix of them.                                                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __FILMCOLORMODEL_H__
#define __FILMCOLORMODEL_H__

#include "ColorModel.h"

#include <string>
#include <vector>

class CFilmColorModel : public CColorModel
{
    public:
            // A film stock.
        struct SStock
        {
            const char     *strName;
            const char     *strDescription;
            BOOL            bColor;             // three layers; FALSE: black and white
            FLT             nIso;
            FLT             anSensitivity[ 3 ][ 3 ];// each layer's response to red, green, blue
            FLT             anGamma[ 3 ];       // each layer's contrast: density per decade of exposure
            FLT             nMinDensity;        // the clear film base (shows as white)
            FLT             nMaxDensity;
            FLT             nMidDensity;        // the density mid gray (0.18) comes out at
            FLT             nGrain;             // RMS density at density 1 through a 48 um aperture
            FLT             nIrradiation;       // the share of light scattered in the emulsion
            FLT             nIrradiationRadius; // its spread (a Gaussian's sigma), in mm
            FLT             nHalation;          // the share reflected back from the base
            FLT             nHalationRadius;    // in mm
            FLT             anHalationTint[ 3 ];// halation's strength in each layer (red, green, blue)
        };

    protected:
        std::string         m_strStock;
        const SStock       *m_pStock;
        FLT                 m_nIso;             // 0: the stock's
        FLT                 m_nShutter;         // seconds
        FLT                 m_nFStop;
        FLT                 m_nWidth;           // the image's width on the film, in mm
        FLT                 m_nGrain;           // scale on the stock's grain
        FLT                 m_nSpread;          // scale on the stock's irradiation and halation
        int                 m_nSeed;

            /// The exposure factor: 256 * shutter * ISO / fStop^2.
        FLT                 ExposureFactor( void ) const;
            /// Each layer's exposure for a linear color.
        void                LayerExposures( const CColor &oColor, FLT anExposure[ 3 ] ) const;
            /// The bytes for a pixel from its layers' densities.
        void                DensitiesToRGB( const FLT anDensity[ 3 ], BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const;

    public:
                            CFilmColorModel( void );

        virtual CColorModel    *Clone( void ) const { return new CFilmColorModel( *this ); }
        virtual const char     *GetName( void ) const { return "film"; }

        virtual void            ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const;
        virtual CColor          ToLinear( const CColor &oColor ) const;
        virtual BOOL            NeedsWholeImage( void ) const { return TRUE; }
        virtual void            Finish( CImageFile &oImage ) const;

        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );

            /// Uses a stock by name; FALSE if there is none.
        BOOL                    SetStock( const char *strName );
        const SStock           &GetStock( void ) const { return *m_pStock; }
        void                    SetExposure( FLT nShutter, FLT nFStop ) { m_nShutter = nShutter; m_nFStop = nFStop; }
        void                    SetWidth( FLT nMillimeters ) { m_nWidth = nMillimeters; }

            /// A layer's density at exposure nExposure (its characteristic curve).
        FLT                     Density( int nLayer, FLT nExposure ) const;
            /// What a density shows, from 0 (black) to 1 (the clear base), linear.
        FLT                     Transmission( FLT nDensity ) const;
            /// The normalized Gaussian kernel for a spread of nSigma pixels
            /// (one side, center first), as Finish uses it.
        static std::vector<FLT> Kernel( FLT nSigma );
            /// Spreads one layer of exposures in place: (1 - a - b) of it
            /// stays, a goes into a Gaussian of nSigmaA pixels and b into one
            /// of nSigmaB. The total is kept.
        static void             Spread( std::vector<FLT> &anLayer, int nWidth, int nHeight,
                                        FLT nShareA, FLT nSigmaA, FLT nShareB, FLT nSigmaB );

        static const SStock    *GetStocks( int &nCount );
};

#endif // __FILMCOLORMODEL_H__
