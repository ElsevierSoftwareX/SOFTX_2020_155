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
** sccsid = "@(#)Grid.h  1.6" -- 97/04/24
*/

/*
** Grid Layout Widget:
**
** A widget for laying out grids.
*/

#ifndef    _XdGrid_h
#define    _XdGrid_h

#include "XdResources.h"


#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 
** Grid Layout Widget class and record definitions  
*/

extern WidgetClass              xdGridWidgetClass ;

typedef struct _XdGridClassRec *XdGridWidgetClass ;
typedef struct _XdGridRec      *XdGridWidget ;

/*
** Subclass Definition 
*/

#ifndef   XdIsGrid
#define   XdIsGrid(w)     XtIsSubclass(w, xdGridWidgetClass)
#endif /* XdIsGrid */

/*
** Public Function Declarations
*/

#ifdef    _NO_PROTO
extern Widget XdCreateGridWidget() ;
#else  /* _NO_PROTO */
extern Widget XdCreateGridWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount) ;
#endif /* _NO_PROTO */

/*
** End Public Function Declarations
*/


#ifdef    __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif  /* _XdGrid_h */
/* DO NOT MODIFY BELOW HERE */
