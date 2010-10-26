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
** sccsid = "@(#)GridP.h  1.5" -- 97/04/24
*/

/*
** Grid Layout Widget:
**
** A widget for laying out grids.
*/

#ifndef    _XdGridP_h
#define    _XdGridP_h

#include <Xm/RepType.h>
#include "LayoutP.h"

#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 
** The Grid Layout Widget Class and instance records
*/

typedef struct _XdGridConstraintPart 
{
	XtPointer extension ;
} XdGridConstraintPart ;
 
typedef struct _XdGridConstraintRec 
{
	XmManagerConstraintPart manager ;
	XdGridConstraintPart    grid ;
} XdGridConstraintRec, *XdGridConstraintPtr ;

typedef struct _XdGridClassPart 
{
	XtPointer extension ;
} XdGridClassPart ;

typedef struct _XdGridClassRec
{
	CoreClassPart       core_class ;
	CompositeClassPart  composite_class ;
	ConstraintClassPart constraint_class ;
	XmManagerClassPart  manager_class ;
	XdLayoutClassPart   layout_class ;
	XdGridClassPart     grid_class ;
} XdGridClassRec ;

extern XdGridClassRec XdgridClassRec ;

/*
** The Grid Layout Widget instance record
*/

typedef struct _XdGridPart
{
	short     numColumns ; /* Column     Count   */
	Dimension hSpacing ;   /* Horizontal Spacing */
	Dimension vSpacing ;   /* Vertical   Spacing */
	XtPointer extension ;
} XdGridPart ;

typedef struct _XdGridRec 
{
	CorePart       core ;
	CompositePart  composite ;
	ConstraintPart constraint ;
	XmManagerPart  manager ;
	XdLayoutPart   layout ;
	XdGridPart     grid ;
} XdGridRec ;

#ifdef    __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif /* _XdGridP_h */
/* DO NOT EDIT BELOW HERE */
