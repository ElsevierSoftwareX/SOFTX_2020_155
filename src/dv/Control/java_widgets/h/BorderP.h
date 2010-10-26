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
** sccsid = "@(#)BorderP.h  1.5" -- 97/04/24
*/
 
/*
** Border Layout Widget:
**
** A widget for laying out main windows.
*/

#ifndef    _XdBorderP_h
#define    _XdBorderP_h

#include <Xm/RepType.h>
#include "LayoutP.h"


#ifdef     __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* 
** The Border Layout Widget Class and instance records
*/

typedef struct _XdBorderConstraintPart 
{
	short alignment ;
} XdBorderConstraintPart ;
 
typedef struct _XdBorderConstraintRec 
{
	XmManagerConstraintPart manager ;
	XdBorderConstraintPart  border ;
} XdBorderConstraintRec, *XdBorderConstraintPtr ;

typedef struct _XdBorderClassPart 
{
	XtPointer extension ;
} XdBorderClassPart ;

typedef struct _XdBorderClassRec
{
	CoreClassPart       core_class ;
	CompositeClassPart  composite_class ;
	ConstraintClassPart constraint_class ;
	XmManagerClassPart  manager_class ;
	XdLayoutClassPart   layout_class ;
	XdBorderClassPart   border_class ;
} XdBorderClassRec ;

extern XdBorderClassRec XdborderClassRec ;


/* 
** The Border Layout instance record  
*/

typedef struct _XdBorderPart
{
	Dimension hSpacing ;   /* Horizontal Spacing */
	Dimension vSpacing ;   /* Vertical   Spacing */
	XtPointer extension ;
} XdBorderPart;

typedef struct _XdBorderRec 
{
	CorePart       core ;
	CompositePart  composite ;
	ConstraintPart constraint ;
	XmManagerPart  manager ;
	XdLayoutPart   layout ;
	XdBorderPart   border ;
} XdBorderRec ;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif /* _XdBorderP_h */
/* DO NOT MODIFY BELOW HERE */
