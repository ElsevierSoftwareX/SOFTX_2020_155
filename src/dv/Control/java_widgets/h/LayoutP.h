/* X-Designer
** (c) 1992, 1993, 1994, 1995
** Imperial Software Technology (IST),
** Berkshire House, 252 King's Road.,
** Reading,
** Berkshire,
** United Kingdom RG1 4HP.
**
** Telephone: +44 1734 587055
** Fax:       +44 1734 589005
** Email:     support@ist.co.uk
*/


/*
** sccsid = "@(#)LayoutP.h  1.7" -- 97/04/24
*/

/*
** Layout:
**
** A an abstract layout widget.
**
** Jeremy Huxtable 1995
*/

#ifndef    _XdLayoutP_h
#define    _XdLayoutP_h

#include <Xm/ManagerP.h>

#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef    _NO_PROTO
typedef void (*XdLayoutProc)() ;
typedef void (*XdPreferredSizeProc)() ;
#else  /* _NO_PROTO */
typedef void (*XdLayoutProc)(Widget w) ;
typedef void (*XdPreferredSizeProc)(Widget w, Dimension *width, Dimension *height) ;
#endif /* _NO_PROTO */

/*
** The Layout Widget Class and instance records
*/

typedef struct _XdLayoutClassPart 
{
	XdLayoutProc        layout ;
	XdPreferredSizeProc preferred_size ;
} XdLayoutClassPart ;

typedef struct _XdLayoutClassRec
{
	CoreClassPart       core_class ;
	CompositeClassPart  composite_class ;
	ConstraintClassPart constraint_class ;
	XmManagerClassPart  manager_class ;
	XdLayoutClassPart   layout_class ;
} XdLayoutClassRec ;

extern XdLayoutClassRec xdLayoutClassRec ;

/*
** The Layout Widget instance record
*/

typedef struct _XdLayoutPart
{
	Boolean needsLayout ;
	Boolean needsRedisplay ;
} XdLayoutPart ;

typedef struct _XdLayoutRec 
{
	CorePart       core ;
	CompositePart  composite ;
	ConstraintPart constraint ;
	XmManagerPart  manager ;
	XdLayoutPart   layout ;
} XdLayoutRec ;

#define LayoutClass(w)	((XdLayoutClassRec *)XtClass(w))
#define XdCallLayout(w)	(*LayoutClass(w)->layout_class.layout)((Widget)w)

#define Width(w) ((Dimension) ((w)->core.width + 2*(w)->core.border_width))
#define Height(w) ((Dimension) ((w)->core.height + 2*(w)->core.border_width))

/*
** Private Function Declarations
*/

#ifdef    _NO_PROTO
extern Boolean XdCheckLayout() ;
extern void    XdAlignInCell() ;
#else  /* _NO_PROTO */
extern Boolean XdCheckLayout(Widget w, WidgetClass c) ;
extern void    XdAlignInCell(XRectangle *r, XRectangle *cell, int alignment, int fill) ;
#endif /* _NO_PROTO */

/*
** End Private Function Declarations
*/

#ifdef    __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif /* _XdLayoutP_h */
/* DO NOT MODIFY BELOW HERE */
