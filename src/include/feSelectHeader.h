#ifdef SEI_CODE
	#include "sei.h"	/* User code for HEPI control.		*/
#elif defined(SUS_CODE)
	#include "sus.h"	/* User code for quad control.		*/
#elif defined(PNM)
	#include "pnm.h"	/* User code for Ponderomotive control. */
#elif defined(PDE_CODE)
	#include "pde.h"	/* User code for Ponderomotive control. */
#elif defined(OMC_CODE)
	#include "omc.h"	/* User code for OMC control. */
#elif defined(LTB_CODE)
	#include "ltb.h"	/* User code for LTB control. */
#elif defined(DBB_CODE)
	#include "dbb.h"
#elif defined(DAS_CODE)
	#include "das.h"
#elif defined(TEST_CODE)
	#include "test.h"
#elif defined(SAS_CODE)
	#include "sas.h"
#elif defined(ASS_CODE)
	#include "ass.h"
#elif defined(PDX_CODE)
	#include "pdx.h"
#elif defined(TES_CODE)
	#include "tes.h"
#elif defined(MCE_CODE)
	#include "mce.h"
#elif defined(TPT_CODE)
	#include "tpt.h"
#elif defined(OMS_CODE)
	#include "oms.h"
#else
	#error
#endif
