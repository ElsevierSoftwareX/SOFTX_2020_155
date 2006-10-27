#ifdef SEI_CODE
	#include "drv/seiwd.c"	/* User code for HEPI control.		*/
	#include "sei/sei.c"	/* User code for HEPI control.		*/
#elif defined(SUS_CODE)
	#include "sus/sus.c"	/* User code for quad control.		*/
#elif defined(PNM)
	#include "pnm/pnm.c"	/* User code for Ponderomotive control. */
#elif defined(PDE_CODE)
	#include "pde/pde.c"	/* User code for Ponderomotive control. */
#elif defined(OMC_CODE)
	#include "omc/omc.c"	/* User code for Ponderomotive control. */
	volatile float *lscRfmPtr = 0;
#elif defined(LTB_CODE)
	#include "ltb/ltb.c"	/* User code for Ponderomotive control. */
<<<<<<< feSelectCode.c
#elif defined(SEI_CODE)
	#include "sei/sei.c"
=======
#elif defined(DBB_CODE)
	#include "dbb/dbb.c"
>>>>>>> 1.2
#else
	#error
#endif
