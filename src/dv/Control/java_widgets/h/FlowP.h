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
** sccsid = "@(#)FlowP.h  1.5" -- 97/04/24
*/

/*
** Flow Layout Widget:
**
** A widget for laying out widgets in a text-like manner.
*/

#ifndef    _XdFlowP_h
#define    _XdFlowP_h

#include <Xm/RepType.h>
#include "LayoutP.h"

#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
** Flow Widget class and record definitions
*/

typedef struct _XdFlowConstraintPart 
{
	XtPointer extension ;
} XdFlowConstraintPart ;
 
typedef struct _XdFlowConstraintRec 
{
	XmManagerConstraintPart manager ;
	XdFlowConstraintPart    flow ;
} XdFlowConstraintRec, *XdFlowConstraintPtr ;

typedef struct _XdFlowClassPart 
{
	XtPointer extension ;
} XdFlowClassPart ;

typedef struct _XdFlowClassRec
{
	CoreClassPart       core_class ;
	CompositeClassPart  composite_class ;
	ConstraintClassPart constraint_class ;
	XmManagerClassPart  manager_class ;
	XdLayoutClassPart   layout_class ;
	XdFlowClassPart     flow_class ;
} XdFlowClassRec ;

extern XdFlowClassRec XdflowClassRec ;

/*
** The Flow Widget instance record
*/

typedef struct  _XdFlowPart
{
	unsigned char alignment ;  /* Flow     Alignment */
	Dimension     hSpacing ;   /* Horizontal Spacing */
	Dimension     vSpacing ;   /* Vertical   Spacing */
	XtPointer     extension ;
} XdFlowPart ;

typedef struct _XdFlowRec 
{
	CorePart       core ;
	CompositePart  composite ;
	ConstraintPart constraint ;
	XmManagerPart  manager ;
	XdLayoutPart   layout ;
	XdFlowPart     flow ;
} XdFlowRec ;

#ifdef    __cplusplus
} /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif  /* _XdFlowP_h */
/* DO NOT MODIFY BELOW HERE */
