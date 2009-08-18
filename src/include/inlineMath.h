/* CVS VERSION: $Id: inlineMath.h,v 1.4 2009/08/18 16:25:21 aivanov Exp $ */

#define __lrint_code \
  long int __lrintres;                                                        \
  __asm__ __volatile__                                                        \
    ("fistpl %0"                                                              \
     : "=m" (__lrintres) : "t" (__x) : "st");                                 \
  return __lrintres

inline int rintf (float __x) { __lrint_code; }
inline int rint (double __x) { __lrint_code; }

inline void sincos(double __x, double *__sinx, double *__cosx)
{
  register long double __cosr;
  register long double __sinr;
  __asm __volatile__
    ("fsincos\n\t"
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
     : "=t" (__cosr), "=u" (__sinr) : "0" (__x));
  *__sinx = __sinr;
  *__cosx = __cosr;

}

/* Fast Pentium FPU SQRT command */
inline double lsqrt (double __x) { register double __result; __asm __volatile__ ("fsqrt" : "=t" (__result) : "0" (__x)); return __result; }


/* Fast Pentium FPU 2^x command for -1<=x<=1*/
inline double l2xr (double __x) { register double __result; __asm __volatile__ ("f2xm1\n\t fld1\n\t fadd\n\t" : "=t" (__result) : "0" (__x)); return __result; }

/* Fast Pentium FPU round to nearest integer command */
inline double lrndint (double __x) { register double __result; __asm __volatile__ ("frndint" : "=t" (__result) : "0" (__x)); return __result; }

/* Fast Pentium FPU to multiply with log2(10) */
inline double lmullog210 (double __x) { register double __result; __asm __volatile__ ("fldl2t\n\t  fmul" : "=t" (__result) : "0" (__x)); return __result; }


/* Fast Pentium FPU log10(x) command */
inline double llog10 (double __x) { register double __result; __asm __volatile__ ("fldlg2\n\t fxch %%st(1)\n\t fyl2x": "=t" (__result) : "0" (__x)); return __result; }

/* Fast Pentium absolute value */
inline double lfabs (double __x) { register double __result; __asm __volatile__ ("fabs" : "=t" (__result) : "0" (__x)); return __result; }

