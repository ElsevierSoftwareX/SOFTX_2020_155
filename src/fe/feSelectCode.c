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
#elif defined(X00_CODE)
	#include "x00/x00.c"
#elif defined(LSC_CODE)
	#include "lsc/lsc.c"
#elif defined(LSP_CODE)
	#include "lsp/lsp.c"
#elif defined(SUP_CODE)
	#include "sup/sup.c"
#elif defined(SCX_CODE)
	#include "scx/scx.c"
#elif defined(SCY_CODE)
	#include "scy/scy.c"
#elif defined(SPY_CODE)
	#include "spy/spy.c"
#elif defined(SVC_CODE)
	#include "svc/svc.c"
#elif defined(SPX_CODE)
	#include "spx/spx.c"
#elif defined(TST_CODE)
	#include "tst/tst.c"
#elif defined(C1SPX_CODE)
	#include "c1spx/c1spx.c"
#elif defined(C1X00_CODE)
	#include "c1x00/c1x00.c"
#elif defined(C1LSC_CODE)
	#include "c1lsc/c1lsc.c"
#elif defined(C1LSP_CODE)
	#include "c1lsp/c1lsp.c"
#elif defined(C1SUP_CODE)
	#include "c1sup/c1sup.c"
#elif defined(C1SVC_CODE)
	#include "c1svc/c1svc.c"
#elif defined(C1SCX_CODE)
	#include "c1scx/c1scx.c"
#elif defined(C1SPY_CODE)
	#include "c1spy/c1spy.c"
#elif defined(C1SCY_CODE)
	#include "c1scy/c1scy.c"
#elif defined(C1VGL_CODE)
	#include "c1vgl/c1vgl.c"
#elif defined(C1X01_CODE)
	#include "c1x01/c1x01.c"
#elif defined(C1VGA_CODE)
	#include "c1vga/c1vga.c"
#elif defined(C1RF1_CODE)
	#include "c1rf1/c1rf1.c"
#elif defined(C1RF0_CODE)
	#include "c1rf0/c1rf0.c"
#elif defined(C1SUS_CODE)
	#include "c1sus/c1sus.c"
#elif defined(C1X02_CODE)
	#include "c1x02/c1x02.c"
#elif defined(C1TST_CODE)
	#include "c1tst/c1tst.c"
#else
	#error
#endif
