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

