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
** sccsid = "@(#)Border.h(1.8)" -- 97/04/24
*/

/*
** Border Layout Widget:
**
** A widget for laying out main windows.
*/


#ifndef    _XdBorder_h
#define    _XdBorder_h

#include "XdResources.h"


#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 
** Border Layout Widget class and record definitions
*/

extern WidgetClass                xdBorderWidgetClass ;

typedef struct _XdBorderClassRec *XdBorderWidgetClass ;
typedef struct _XdBorderRec      *XdBorderWidget ;


/* 
** Subclass definition 
*/

#ifndef   XdIsBorder
#define   XdIsBorder(w)     XtIsSubclass(w, xdBorderWidgetClass)
#endif /* XdIsBorder */


/*
** Public Function Declarations
*/

#ifdef    _NO_PROTO
extern Widget XdCreateBorderWidget() ;
#else  /* _NO_PROTO */
extern Widget XdCreateBorderWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount) ;
#endif /* _NO_PROTO */

/*
** End Public Function Declarations
*/


#ifdef     __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif  /* __cplusplus */

#endif /* _XdBorder_h */
/* DO NOT MODIFY BELOW HERE */
