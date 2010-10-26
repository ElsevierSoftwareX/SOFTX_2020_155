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
** sccsid = "@(#)Card.h  1.9" -- 97/04/24
*/
 
/*
** Card Layout Widget:
**
** A widget for laying out a stack of pages.
*/

#ifndef    _XdCard_h
#define    _XdCard_h

#include "XdResources.h"


#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
** Widget class and record definitions
*/

extern WidgetClass              xdCardWidgetClass ;

typedef struct _XdCardClassRec *XdCardWidgetClass ;
typedef struct _XdCardRec      *XdCardWidget ;

/*
** Subclass Definition
*/

#ifndef   XdIsCard
#define   XdIsCard(widget)    XtIsSubClass(widget, XdCardWidgetClass)
#endif /* XdIsCard */

/*
** Public Function Declarations
*/

#ifdef    _NO_PROTO
extern void   XdCardShowPage() ;
extern Widget XdCreateCardWidget() ;
#else  /* _NO_PROTO */
extern void   XdCardShowPage(Widget w, int page);
extern Widget XdCreateCardWidget(Widget parent, char *name, Arg *arglist, Cardinal argCount);
#endif /* _NO_PROTO */

/*
** End Public Function Declarations
*/

#ifdef    __cplusplus
} /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif /* _XdCard_h */
/* DO NOT MODIFY BELOW HERE */
