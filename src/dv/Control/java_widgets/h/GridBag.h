/* X-Designer
** (c) Copyright 1996
** Imperial Software Technology (IST),
** Berkshire House, 252 King's Road.,
** Reading,
** Berkshire,
** United Kingdom RG1 4HP.
**
** Telephone: +44 118 9587055
** Fax:       +44 118 9589005
** Email:     support@ist.co.uk
*/


/*
** sccsid = "@(#)GridBag.h  1.8" -- 97/04/24
*/

/*
** GridBag Layout Widget:
**
** A widget for laying out more complex grids.
*/

#ifndef    _XdGridBag_h
#define    _XdGridBag_h

#include "XdResources.h"


#define XdDEFAULT_POS     0
#define XdRELATIVE       -1
#define XdREMAINDER       0


#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
** GridBag Layout Widget class and record definitions
*/

extern WidgetClass                 xdGridBagWidgetClass ;

typedef struct _XdGridBagClassRec *XdGridBagWidgetClass ;
typedef struct _XdGridBagRec      *XdGridBagWidget ;

/*
** Subclass definition
*/

#ifndef   XdIsGridBag
#define   XdIsGridBag(w)     XtIsSubclass(w, xdGridBagWidgetClass)
#endif /* XdIsGridBag */

/*
** Public Function Declarations
*/

#ifdef    _NO_PROTO
extern Widget XdCreateGridBagWidget() ;
#else  /* _NO_PROTO */
extern Widget XdCreateGridBagWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount) ;
#endif /* _NO_PROTO */

/*
** End Public Function Declarations
*/

#ifdef    __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif /* _XdGridBag_h */
/* DO NOT MODIFY BELOW HERE */
