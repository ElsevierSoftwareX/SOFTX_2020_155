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
** sccsid = "@(#)Flow.h  1.6" -- 97/04/24
*/
 
/*
** Flow Layout Widget:
**
** A widget for laying out widgets in a text-like manner.
*/

#ifndef    _XdFlow_h
#define    _XdFlow_h

#include "XdResources.h"

 
#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 
** Flow Layout Widget class and record definitions
*/

extern WidgetClass              xdFlowWidgetClass ;

typedef struct _XdFlowClassRec *XdFlowWidgetClass ;
typedef struct _XdFlowRec      *XdFlowWidget ;

/*
** Subclass definition 
*/

#ifndef   XdIsFlow
#define   XdIsFlow(w)     XtIsSubclass(w, xdFlowWidgetClass)
#endif /* XdIsFlow */

/*
** Public Function Declarations
*/

#ifdef    _NO_PROTO
extern Widget XdCreateFlowWidget() ;
#else  /* _NO_PROTO */
extern Widget XdCreateFlowWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount) ;
#endif /* _NO_PROTO */

/*
** End Public Function Declarations
*/

#ifdef    __cplusplus
} /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif  /* _XdFlow_h */
/* DO NOT MODIFY BELOW HERE */
