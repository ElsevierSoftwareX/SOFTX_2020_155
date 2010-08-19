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
#elif defined(DBB_CODE)
	#include "dbb/dbb.c"
#elif defined(DAS_CODE)
	#include "das/das.c"
#elif defined(TEST_CODE)
	#include "test/test.c"
#elif defined(SAS_CODE)
	#include "sas/sas.c"
#elif defined(ASS_CODE)
	#include "ass/ass.c"
#elif defined(PDX_CODE)
	#include "pdx/pdx.c"
#elif defined(TES_CODE)
	#include "tes/tes.c"
#elif defined(MCE_CODE)
	#include "mce/mce.c"
#elif defined(TPT_CODE)
	#include "tpt/tpt.c"
#elif defined(OMS_CODE)
	#include "oms/oms.c"
#elif defined(SH1_CODE)
	#include "sh1/sh1.c"
#elif defined(ISI_CODE)
	#include "isi/isi.c"
#elif defined(OM1_CODE)
	#include "om1/om1.c"
#elif defined(OM2_CODE)
	#include "om2/om2.c"
#elif defined(ALX_CODE)
	#include "alx/alx.c"
#else
	#error
#endif
