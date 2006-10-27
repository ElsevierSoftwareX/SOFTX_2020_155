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
#else
	#error
#endif
