/*************************************************************************
*   CExpression                                                          *
*                                                                        *
*   A formula in x, y and z, such as "x^2 + y^2 + z^2 - 1", compiled to  *
*   a short program for a stack machine, so an implicit surface can be   *
*   worked out quickly and from many threads at once (evaluating it      *
*   changes nothing in the object).                                      *
*                                                                        *
*   The language:                                                        *
*                                                                        *
*       numbers     1, 2.5, .5, 1e-3                                     *
*       names       x, y, z; the constants pi and e; and parameters     *
*                   given by name                                        *
*       operators   + - * / ^ (power, right to left: 2^3^2 is 2^9),     *
*                   unary - and +, and parentheses. -x^2 is -(x^2).      *
*       functions   sin cos tan asin acos atan atan2(y, x) sqrt abs exp  *
*                   log pow(a, b) min(a, b) max(a, b) floor              *
*                   clamp(x, low, high) mix(a, b, t)                     *
*                   smoothstep(edge0, edge1, x)                          *
*                                                                        *
*   Value works out the formula at a point; Gradient works out its value *
*   and its slope along x, y and z together, exactly (by carrying the    *
*   derivatives through every step, not by differences), for normals.    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __EXPRESSION_H__
#define __EXPRESSION_H__

#include <map>
#include <string>
#include <vector>

class CExpression
{
    public:
        struct SOp
        {
            int                 nCode;
            double              nValue;
        };

                            CExpression( void );

            /// Compiles strText. FALSE, with a reason that starts with the
            /// column (from 1) in strError, if it isn't a formula. Names
            /// in aoParameters stand for their numbers.
        bool                Compile( const std::string &strText, const std::map<std::string, double> &aoParameters,
                                     std::string &strError );
        bool                IsCompiled( void ) const { return ! m_aoProgram.empty(); }

        double              Value( double x, double y, double z ) const;
            /// The value, and in anGradient the slope along x, y and z.
        double              Gradient( double x, double y, double z, double anGradient[ 3 ] ) const;

            /// Whether a name is taken by the language (x, y, z, pi, e or a
            /// function), so it can't be a parameter.
        static bool         IsReserved( const std::string &strName );

            /// The deepest the stack may go.
        enum { MAX_DEPTH = 64 };

    private:
        std::vector<SOp>    m_aoProgram;
};

#endif // __EXPRESSION_H__
