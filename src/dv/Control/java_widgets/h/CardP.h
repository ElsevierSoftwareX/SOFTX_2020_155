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
** sccsid = "@(#)CardP.h  1.6" -- 97/04/24
*/
 
/*
** Card Layout Widget:
**
** A widget for laying out a stack of pages.
*/

#ifndef    _XdCardP_h
#define    _XdCardP_h

#include <Xm/RepType.h>
#include "LayoutP.h"

 
#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
** The Card Layout Widget Class and instance records
*/

typedef struct _XdCardConstraintPart 
{
	short     pageNumber ;
	XtPointer extension ;
} XdCardConstraintPart ;
 
typedef struct _XdCardConstraintRec 
{
	XmManagerConstraintPart manager ;
	XdCardConstraintPart    card ;
} XdCardConstraintRec, *XdCardConstraintPtr ;

typedef struct _XdCardClassPart
{
	XtPointer extension ;
} XdCardClassPart ;

typedef struct _XdCardClassRec
{
	CoreClassPart       core_class ;
	CompositeClassPart  composite_class ;
	ConstraintClassPart constraint_class ;
	XmManagerClassPart  manager_class ;
	XdLayoutClassPart   layout_class ;
	XdCardClassPart     card_class ;
} XdCardClassRec ;

extern XdCardClassRec XdcardClassRec ;

/* 
** The Card Layout instance record
*/

typedef struct _XdCardPart
{
	Dimension  hSpacing ;       /* Horizontal Spacing  */
	Dimension  vSpacing ;       /* Vertical   Spacing  */
	Widget     currentPage ;    /* Current Card Widget */
	int        currentPageNum ; /* Current Card Page   */
	XtPointer  extension ;         
} XdCardPart ;   

typedef struct _XdCardRec 
{
	CorePart       core ;
	CompositePart  composite ;
	ConstraintPart constraint ;
	XmManagerPart  manager ;
	XdLayoutPart   layout ;
	XdCardPart     card ;
} XdCardRec ;

#ifdef    __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif  /* _XdCardP_h */
/* DO NOT MODIFY BELOW HERE */
