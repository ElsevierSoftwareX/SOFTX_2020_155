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
** sccsid = "@(#)Layout.h  1.5" -- 97/04/24
*/

/*
** Layout:
**
** An abstract layout widget.
**
** Jeremy Huxtable 1995
*/

#ifndef    _XdLayout_h
#define    _XdLayout_h

#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
** Layout Widget class and record definitions
*/

extern WidgetClass                xdLayoutWidgetClass ;

typedef struct _XdLayoutClassRec *XdLayoutWidgetClass ;
typedef struct _XdLayoutRec       *XdLayoutWidget ;

/*
** Subclass Definition
*/

#ifndef   XdIsLayout
#define   XdIsLayout(w)     XtIsSubclass(w, xdLayoutWidgetClass)
#endif /* XdIsLayout */

/*
** Public Function Declarations
*/

/*
** End Public Function Declarations
*/
 
#ifdef    __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#endif /* _XdLayout_h */
/* DO NOT MODIFY BELOW HERE */
