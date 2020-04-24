/* CVS VERSION: $Id: inlineMath.h,v 1.4 2009/08/18 16:25:21 aivanov Exp $ */

#ifndef INLINE_MATH
#define INLINE_MATH

#define M_PI 3.14159265358979323846
#define M_TWO_PI 6.28318530717958647692

#include <asm/msr.h>
#define rdtscl( low ) __asm__ __volatile__( "rdtsc" : "=a"( low ) : : "edx" )

// These are already defined in kernel's msr.h
#if 0
#define rdtscll( val ) __asm__ __volatile__( "rdtsc" : "=A"( val ) )
#endif

#define __lrint_code                                                           \
    long int __lrintres;                                                       \
    __asm__ __volatile__( "fistpl %0"                                          \
                          : "=m"( __lrintres )                                 \
                          : "t"( __x )                                         \
                          : "st" );                                            \
    return __lrintres

inline int
_rintf( float __x )
{
    __lrint_code;
}
inline int
_rint( double __x )
{
    __lrint_code;
}

inline void
sincos( double __x, double* __sinx, double* __cosx )
{
    register long double __cosr;
    register long double __sinr;
    __asm __volatile__( "fsincos\n\t"
                        "fnstsw    %%ax\n\t"
                        "testl     $0x400, %%eax\n\t"
                        "jz        1f\n\t"
                        "fldpi\n\t"
                        "fadd      %%st(0)\n\t"
                        "fxch      %%st(1)\n\t"
                        "2: fprem1\n\t"
                        "fnstsw    %%ax\n\t"
                        "testl     $0x400, %%eax\n\t"
                        "jnz       2b\n\t"
                        "fstp      %%st(1)\n\t"
                        "fsincos\n\t"
                        "1:"
                        : "=t"( __cosr ), "=u"( __sinr )
                        : "0"( __x ) );
    *__sinx = __sinr;
    *__cosx = __cosr;
}

/* Fast Pentium FPU SQRT command */
inline double
lsqrt( double __x )
{
    register double __result;
    __asm __volatile__( "fsqrt" : "=t"( __result ) : "0"( __x ) );
    return __result;
}

/* Fast Pentium FPU 2^x command for -1<=x<=1*/
inline double
l2xr( double __x )
{
    register double __result;
    __asm __volatile__( "f2xm1\n\t fld1\n\t faddp\n\t"
                        : "=t"( __result )
                        : "0"( __x ) );
    return __result;
}

/* Fast Pentium FPU round to nearest integer command */
inline double
lrndint( double __x )
{
    register double __result;
    __asm __volatile__( "frndint" : "=t"( __result ) : "0"( __x ) );
    return __result;
}

/* Fast Pentium FPU to multiply with log2(10) */
inline double
lmullog210( double __x )
{
    register double __result;
    __asm __volatile__( "fldl2t\n\t  fmulp" : "=t"( __result ) : "0"( __x ) );
    return __result;
}

/* Fast Pentium FPU log10(x) command */
inline double
llog10( double __x )
{
    register double __result;
    __asm __volatile__( "fldlg2\n\t fxch %%st(1)\n\t fyl2x"
                        : "=t"( __result )
                        : "0"( __x ) );
    return __result;
}

/* Fast Pentium absolute value */
inline double
lfabs( double __x )
{
    register double __result;
    __asm __volatile__( "fabs" : "=t"( __result ) : "0"( __x ) );
    return __result;
}

/* Fast Pentium ATAN2 */
inline double
latan2( double __y, double __x )
{
    register long double __atanr;
    __asm __volatile( "fpatan\n\t" : "=t"( __atanr ) : "0"( __x ), "u"( __y ) );
    return __atanr;
}

/*

SSE MXCSR
The MXCSR register is a 32-bit register containing flags for control and status
information regarding SSE instructions. As of SSE3, only bits 0-15 have been
defined.

Pnemonic	Bit Location	Description
FZ	bit 15	Flush To Zero
R+	bit 14	Round Positive
R-	bit 13	Round Negative
RZ	bits 13 and 14	Round To Zero
RN	bits 13 and 14 are 0	Round To Nearest
PM	bit 12	Precision Mask
UM	bit 11	Underflow Mask
OM	bit 10	Overflow Mask
ZM	bit 9	Divide By Zero Mask
DM	bit 8	Denormal Mask
IM	bit 7	Invalid Operation Mask
DAZ	bit 6	Denormals Are Zero
PE	bit 5	Precision Flag
UE	bit 4	Underflow Flag
OE	bit 3	Overflow Flag
ZE	bit 2	Divide By Zero Flag
DE	bit 1	Denormal Flag
IE	bit 0	Invalid Operation Flag


FZ mode causes all underflowing operations to simply go to zero. This saves some
processing time, but loses precision.

The R+, R-, RN, and RZ rounding modes determine how the lowest bit is generated.
Normally, RN is used.

PM, UM, MM, ZM, DM, and IM are masks that tell the processor to ignore the
exceptions that happen, if they do. This keeps the program from having to deal
with problems, but might cause invalid results.

DAZ tells the CPU to force all Denormals to zero. A Denormal is a number that is
so small that FPU can't renormalize it due to limited exponent ranges. They're
just like normal numbers, but they take considerably longer to process. Note
that not all processors support DAZ.

PE, UE, ME, ZE, DE, and IE are the exception flags that are set if they happen,
and aren't unmasked. Programs can check these to see if something interesting
happened. These bits are "sticky", which means that once they're set, they stay
set forever until the program clears them. This means that the indicated
exception could have happened several operations ago, but nobody bothered to
clear it.

DAZ wasn't available in the first version of SSE. Since setting a reserved bit
in MXCSR causes a general protection fault, we need to be able to check the
availability of this feature without causing problems. To do this, one needs to
set up a 512-byte area of memory to save the SSE state to, using fxsave, and
then one needs to inspect bytes 28 through 31 for the MXCSR_MASK value. If bit 6
is set, DAZ is supported, otherwise, it isn't.
*/

static inline unsigned long long
read_mxcsr( void )
{
    unsigned long long mxcsr;
    asm( "stmxcsr %0" : "=m"( mxcsr ) );
    return mxcsr;
}

static inline void
write_mxcsr( unsigned long long val )
{
    asm( "ldmxcsr %0" ::"m"( val ) );
}

/* Set FZ and DAZ bits, disabling underflows and denorms
 * This fixes long execution times caused by 0.0 inputs to filter modules */
void
fz_daz( void )
{
    // unsigned long long mxcsr = read_mxcsr();
    // printk("mxcsr=%p\n", mxcsr);
    // mxcsr |= 1 | 1 << 15;
    // write_mxcsr(mxcsr);
    write_mxcsr( read_mxcsr( ) | 1 | 1 << 15 );
}

#endif
