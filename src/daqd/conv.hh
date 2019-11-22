#ifndef CONV_HH
#define CONV_HH

/*
 * Some simple conversions that are useful in a few places.
 */

namespace conv
{

    static inline float
    s_to_ms( float s )
    {
        return s * 1000.0;
    }

    static inline int
    s_to_ms_int( float s )
    {
        return static_cast< int >( s_to_ms( s ) );
    }

} // namespace conv

#endif // CONV_HH
