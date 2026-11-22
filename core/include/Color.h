/*************************************************************************
*   CColor header                                                        *
*                                                                        *
* Defines the CColor class for use in the ray-tracer.                    *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __COLOR_H__
#define __COLOR_H__

class CVector;

class CColor
{
    protected:
        FLT         m_nRed;
        FLT         m_nGreen;
        FLT         m_nBlue;
   public:
                    CColor( FLT nRed = 0.00, FLT nGreen = 0.00, FLT nBlue = 0.00 ) { m_nRed = nRed; m_nGreen = nGreen; m_nBlue = nBlue; }
                    CColor( CVector &oSource );
                    CColor( const CColor &oSource ) { m_nRed = oSource.m_nRed; m_nGreen = oSource.m_nGreen; m_nBlue = oSource.m_nBlue; }
                   ~CColor( void ) {}
      
        FLT         Distance( const CColor &oCompare );
        CColor      operator +( const CColor &oAdd )
                    {
                        CColor oTemp;

                        oTemp.m_nRed = m_nRed + oAdd.m_nRed;
                        oTemp.m_nGreen = m_nGreen + oAdd.m_nGreen;
                        oTemp.m_nBlue = m_nBlue + oAdd.m_nBlue;
                        return oTemp;
                    }
        CColor      operator *( FLT nScale )
                    {
                        CColor oTemp;

                        oTemp.m_nRed = m_nRed * nScale;
                        oTemp.m_nGreen = m_nGreen * nScale;
                        oTemp.m_nBlue = m_nBlue * nScale;
                        return oTemp;
                    }
        CColor      operator *( const CColor &oMult )
                    {
                        CColor oTemp;

                        oTemp.m_nRed = m_nRed * oMult.m_nRed;
                        oTemp.m_nGreen = m_nGreen * oMult.m_nGreen;
                        oTemp.m_nBlue = m_nBlue * oMult.m_nBlue;
                        return oTemp;
                    }
        void        Clip( void );
        CColor     &operator =( const CColor &oSource )
                    {
                        m_nRed = oSource.m_nRed;
                        m_nGreen = oSource.m_nGreen;
                        m_nBlue = oSource.m_nBlue;
                        return *this;
                    }
        CColor     &operator =( CVector &oSource );
        BOOL        operator ==( const CColor &oCompare );
        BOOL        operator !=( const CColor &oCompare );
        FLT         GetR( void ) {return m_nRed; }
        FLT         GetG( void ) {return m_nGreen; }
        FLT         GetB( void ) {return m_nBlue; }
        void        Set( FLT nRed, FLT nGreen, FLT nBlue, FLT nGamma = 1.0 );


};

#endif // __COLOR_H__