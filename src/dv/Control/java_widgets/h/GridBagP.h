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
** sccsid = "@(#)GridBagP.h  1.7" -- 97/04/24
*/

/*
** GridBag Layout Widget:
**
** A widget for laying out more complex grids.
*/
 
#ifndef    _XdGridBagP_h
#define    _XdGridBagP_h

#include <Xm/RepType.h>
#include "LayoutP.h"

#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
** The GridBag Layout Widget Class and instance records
*/

typedef struct _XdGridBagConstraintPart 
{
	int           row ;
	int           col ;
	int           rows ;
	int           cols ;
	Dimension     padX ;
	Dimension     padY ;
	Dimension     insetL ;
	Dimension     insetR ;
	Dimension     insetT ;
	Dimension     insetB ;
	int           rowWeight ;
	int           colWeight ;
	unsigned char alignment ;
	unsigned char fill ;
} XdGridBagConstraintPart ;
 
typedef struct _XdGridBagConstraintRec 
{
	XmManagerConstraintPart manager ;
	XdGridBagConstraintPart gridBag ;
} XdGridBagConstraintRec, *XdGridBagConstraintPtr ;

typedef struct _XdGridBagClassPart 
{
	XtPointer extension ;
} XdGridBagClassPart ;

typedef struct _XdGridBagClassRec
{
	CoreClassPart       core_class ;
	CompositeClassPart  composite_class ;
	ConstraintClassPart constraint_class ;
	XmManagerClassPart  manager_class ;
	XdLayoutClassPart   layout_class ;
	XdGridBagClassPart  gridBag_class ;
} XdGridBagClassRec ;

extern XdGridBagClassRec XdgridBagClassRec ;

/* 
** The GridBag Layout instance record
*/

typedef struct _XdGridBagPart
{
	XtPointer extension ;
} XdGridBagPart ;

typedef struct _XdGridBagRec 
{
	CorePart       core ;
	CompositePart  composite ;
	ConstraintPart constraint ;
	XmManagerPart  manager ;
	XdLayoutPart   layout ;
	XdGridBagPart  gridBag ;
} XdGridBagRec ;

#ifdef    __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif /* _XdGridBagP_h */
/* DO NOT MODIFY BELOW HERE */
