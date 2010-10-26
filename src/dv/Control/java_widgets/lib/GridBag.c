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

#if !defined(lint) && !defined(NOSCCS)
static char *sccsid = {"@(#)GridBag.c	1.14"} ; /* 98/09/18 */
#endif /* lint && NOSCCS */

/*
** GridBag Layout Widget:
**
** A widget for laying out more complex grids.
*/

#ifdef    __cplusplus
#undef    _NO_PROTO
#endif /* __cplusplus */

#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/CompositeP.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include "GridBagP.h"
#include "GridBag.h"
#include "XdResources.h"

#ifndef    MIN
#define    MIN(a, b) ((a) < (b) ? (a) : (b))
#define    MAX(a, b) ((a) > (b) ? (a) : (b))
#endif  /* MIN */

/*
** Static Widget Method Declarations
*/

#ifdef    _NO_PROTO
static void    ClassInitialize() ;
static void    Initialize() ;
static Boolean SetValues() ;
static Boolean ConstraintSetValues() ;
static void    GridBagLayout() ;
static void    GridBagPreferredSize() ;
#else  /* _NO_PROTO */
static void    ClassInitialize(void) ;
static void    Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args) ;
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args) ;
static Boolean ConstraintSetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args) ;
static void    GridBagLayout(Widget w) ;
static void    GridBagPreferredSize(Widget w, Dimension *width, Dimension *height) ;
#endif /* _NO_PROTO */

/*
** End Static Widget Method Declarations
*/
  
/*
** Translations Table
*/
/* NONE */
/*
** End Translations Table
*/
   
/*
** Actions Table
*/
/* NONE */
/*
** End Actions Table
*/
    
/*
** Resource Lists
*/

#define GridBagConstraint(w) ((XdGridBagConstraintPtr)(w)->core.constraints)

static String alignmentNames[] = 
{
	"center", "east", "north", "northeast", "northwest",
	"south", "southeast", "southwest", "west"
};

static String fillNames[] = 
{
	"none", "horizontal", "vertical", "both"
};

static XtResource constraintResources[] = {
    {   
	XtNxdCellAlignment,
        XtCXdCellAlignment,
        XtRXdCellAlignment,
        sizeof(unsigned char),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.alignment),
        XmRImmediate,
        (XtPointer) XdAlignmentCenter
    },
    {   
	XtNxdCellFill,
        XtCXdCellFill,
        XtRXdCellFill,
        sizeof(unsigned char),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.fill),
        XmRImmediate,
        (XtPointer) XdCellFillNone
    },
    {   
	XtNxdRow, /* Maps onto GridY */
        XtCXdRow,
        XtRXdRow,
        sizeof(int),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.row),
        XmRImmediate,
	/*
	** PR 6499: used to read XdDEFAULT_POS (0) where 
	** the Java documentation has GridY as XdRELATIVE (-1)
	*/

        (XtPointer) XdRELATIVE
    },
    {   
	XtNxdColumn, /* Maps onto GridX */
        XtCXdColumn,
        XtRXdColumn,
        sizeof(int),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.col),
        XmRImmediate,
        (XtPointer) XdRELATIVE
    },
    {   
	XtNxdRows, /* Maps onto GridWidth */
        XtCXdRows,
        XtRXdRows,
        sizeof(int),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.rows),
        XmRImmediate,
        (XtPointer)1
    },
    {   
	XtNxdColumns, /* Maps onto GridHeight */
        XtCXdColumns,
        XtRXdColumns,
        sizeof(int),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.cols),
        XtRImmediate,
        (XtPointer) 1
    },
    {   
	XtNxdPadX,
        XtCXdPadX,
        XtRXdPadX,
        sizeof(Dimension),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.padX),
        XtRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdPadY,
        XtCXdPadY,
        XtRXdPadY,
        sizeof(Dimension),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.padY),
        XtRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdInsetLeft,
        XtCXdInsetLeft,
        XtRXdInsetLeft,
        sizeof(Dimension),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.insetL),
        XtRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdInsetRight,
        XtCXdInsetRight,
        XtRXdInsetRight,
        sizeof(Dimension),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.insetR),
        XtRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdInsetTop,
        XtCXdInsetTop,
        XtRXdInsetTop,
        sizeof(Dimension),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.insetT),
        XtRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdInsetBottom,
        XtCXdInsetBottom,
        XtRXdInsetBottom,
        sizeof(Dimension),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.insetB),
        XtRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdRowWeight,
        XtCXdRowWeight,
        XtRXdRowWeight,
        sizeof(int),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.rowWeight),
        XtRPointer,
        (XtPointer) 0
    },
    {   
	XtNxdColumnWeight,
        XtCXdColumnWeight,
        XtRXdColumnWeight,
        sizeof(int),
        XtOffsetOf(XdGridBagConstraintRec, gridBag.colWeight),
        XtRPointer,
        (XtPointer) 0
    },
};

/*
** End Resource Lists
*/
  
/*
** Widget Class Record
*/
   
XdGridBagClassRec XdgridBagClassRec = {
	{ /* Core class */
		(WidgetClass)&xdLayoutClassRec,/* superclass           */
		"XdGridBag",		/* class_name           */
		sizeof(XdGridBagRec),	/* widget_size          */
		ClassInitialize,	/* class_initialize     */
		NULL,			/* class_part_initialize*/
		FALSE,			/* class_inited         */
		Initialize,		/* initialize           */
		NULL,			/* initialize_hook      */  
		XtInheritRealize,	/* realize              */
		NULL,			/* actions              */
		0,			/* num_actions          */
		NULL,			/* resources            */
		0,			/* num_resources        */
		NULLQUARK,		/* xrm_class            */
		TRUE,			/* compress_motion      */
		TRUE,			/* compress_exposure    */
		TRUE,			/* compress_enterleave  */
		FALSE,			/* visible_interest     */
		NULL,			/* destroy              */
		XtInheritResize,	/* resize               */
		XtInheritExpose,	/* expose               */
		SetValues,		/* set_values           */
		NULL,			/* set_values_hook      */
		XtInheritSetValuesAlmost,/* set_values_almost    */
		NULL,			/* get_values_hook      */
		NULL,			/* accept_focus         */
		XtVersion,		/* version              */
		NULL,			/* callback_offsets     */
		XtInheritTranslations,	/* tm_table             */
		XtInheritQueryGeometry,	/* query_geometry       */
		NULL,			/* display_accelerator  */
		NULL,			/* extension            */
	}, { /* Composite class */
		XtInheritGeometryManager,	/* geometry_handler     */
		XtInheritChangeManaged,		/* change_managed       */
		XtInheritInsertChild,	/* insert_child         */
		XtInheritDeleteChild,	/* delete_child         */
		NULL			/* extension            */
	}, { /* Constraint class */
		constraintResources,	/* resource list        */
		XtNumber(constraintResources),	/* num resources        */
		sizeof(XdGridBagConstraintRec),	/* constraint size      */
		NULL,			/* init proc            */
		NULL,			/* destroy proc         */
		ConstraintSetValues,	/* set values proc      */
		NULL			/* extension            */
	}, { /* Manager class */
		XtInheritTranslations,	/* default translations */
		NULL,			/* get resources        */
		0,			/* num get_resources    */
		NULL,			/* get cont resources   */
		0,			/* num cont get_resources*/
		NULL			/* extension */
	}, { /* Layout class */
		(XdLayoutProc)GridBagLayout,	/* layout */
		(XdPreferredSizeProc)GridBagPreferredSize,	/* preferred_size */
	}, {
		0			/* extension */
	}
};

/*
** End Widget Class Record
*/
  
/*
** Widget Class Pointer
*/

WidgetClass xdGridBagWidgetClass = (WidgetClass) &XdgridBagClassRec;

/*
** Class Initialisation Method - called once only the first time a widget of
**                               this type is instantiated
*/
  
#ifdef    _NO_PROTO
static void ClassInitialize()
#else  /* _NO_PROTO */
static void ClassInitialize(void)
#endif /* _NO_PROTO */
{
	/* Simply register the fill and alignment names */

	XmRepTypeId alignmentID = XmRepTypeRegister(XtRXdCellAlignment, alignmentNames, NULL, XtNumber(alignmentNames)) ;
	XmRepTypeId fillID      = XmRepTypeRegister(XtRXdCellFill, fillNames, NULL, XtNumber(fillNames)) ;
}

/*
** Widget Initialisation Method - called per-instance on widget create
*/
  
/* ARGSUSED */
#ifdef    _NO_PROTO
static void Initialize(request, new_widget, args, num_args)
	Widget    request ;
	Widget    new_widget ;
	ArgList   args ;
	Cardinal *num_args ;
#else  /* _NO_PROTO */
static void Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
	/* Do Nothing */
}

/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static int CountManaged(w)
	XdGridBagWidget w ;
#else  /* _NO_PROTO */
static int CountManaged(XdGridBagWidget w)
#endif /* _NO_PROTO */
{
	int i ;
	int numManaged = 0 ;

	for (i = 0 ; i < w->composite.num_children ; i++) {
		if (w->composite.children[i]->core.managed) {
			numManaged++ ;
		}
	}

	return numManaged;
}


/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static void CalcRowsAndCols(w, numRows, numCols)
	XdGridBagWidget w ;
	int            *numRows ;
	int            *numCols ;
#else  /* _NO_PROTO */
static void CalcRowsAndCols(XdGridBagWidget w, int *numRows, int *numCols)
#endif /* _NO_PROTO */
{
	int i;
	int lastRow = 0, lastCol = 0;

	*numRows = *numCols = 1;
	for (i = 0; i < w->composite.num_children; i++) {
		Widget child = w->composite.children[i];
		if (child->core.managed) {
			XdGridBagConstraintPtr c = GridBagConstraint(child);
			int n, rows, cols;

			n = c->gridBag.row;
			if (n == XdRELATIVE)
				n = lastRow;
			rows = c->gridBag.rows;
			if (rows == XdREMAINDER)
				rows = 1;

			n += rows;
			if (n > *numRows)
				*numRows = n;
			lastRow = n;

			n = c->gridBag.col;
			if (n == XdRELATIVE)
				n = lastCol;
			cols = c->gridBag.cols;
			if (cols == XdREMAINDER)
				cols = 1;
			n += cols;
			if (n > *numCols)
				*numCols = n;
			lastCol = n;
		}
	}
}

/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static int SumArray(array, start, size)
	int *array ;
	int  start ;
	int  size ;
#else  /* _NO_PROTO */
static int SumArray(int *array, int start, int size)
#endif /* _NO_PROTO */
{
	int i, total = 0;

	for (i = start; i < start+size; i++)
		total += array[i];
	return total;
}


/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static int CalcCellSizes(w, rNumRows, rRowHeights, rNumCols, rColWidths)
	XdGridBagWidget w ;
	int            *rNumRows ;
	int           **rRowHeights ;
	int            *rNumCols ;
	int           **rColWidths ;
#else  /* _NO_PROTO */
static int CalcCellSizes(XdGridBagWidget w, int *rNumRows, int **rRowHeights, int *rNumCols, int **rColWidths)
#endif /* _NO_PROTO */
{
	int i;
	int row, col;
	int lastRow = 0, lastCol = 0;
	Widget child;
	unsigned int numManaged;
	int numRows, numCols;
	int *colWidths;
	int *rowHeights;
	float *colWeights;
	float *rowWeights;
	float totalWeight;
	XtWidgetGeometry preferred;
	Dimension remainder, totalSize;

	numManaged = CountManaged(w);
	CalcRowsAndCols(w, &numRows, &numCols);

	colWidths = (int *)XtMalloc(numCols * sizeof(int));
	for (i = 0; i < numCols; i++)
		colWidths[i] = 0;
	rowHeights = (int *)XtMalloc(numRows * sizeof(int));
	for (i = 0; i < numRows; i++)
		rowHeights[i] = 0;
	colWeights = (float *)XtMalloc(numCols * sizeof(float));
	for (i = 0; i < numCols; i++)
		colWeights[i] = 0;
	rowWeights = (float *)XtMalloc(numRows * sizeof(float));
	for (i = 0; i < numRows; i++)
		rowWeights[i] = 0;

	for (i = 0; i < w->composite.num_children; i++) {
		child = w->composite.children[i];
		if (child->core.managed) {
			XdGridBagConstraintPtr c = GridBagConstraint(child);
			Dimension width, height;
			int j, rows, cols;
			float weight;

			row = c->gridBag.row;
			if (row == XdRELATIVE)
				row = lastRow;
			col = c->gridBag.col;
			if (col == XdRELATIVE)
				col = lastCol;
			
			rows = c->gridBag.rows;
			if (rows == XdREMAINDER)
				rows = numRows-row;
			cols = c->gridBag.cols;
			if (cols == XdREMAINDER)
				cols = numCols-col;


			XtQueryGeometry(child, NULL, &preferred);
			preferred.width += (2 * c->gridBag.padX) + c->gridBag.insetL + c->gridBag.insetR;
			preferred.height += (2 * c->gridBag.padY) + c->gridBag.insetT + c->gridBag.insetB;

			/* PR 5079 Patch                    */
			/* A.J.Fountain, IST, February 1997 */
			width  = ((cols > 0) ? (preferred.width  / (Dimension) cols) : preferred.width);
			height = ((rows > 0) ? (preferred.height / (Dimension) rows) : preferred.height);
			/* END PR 5079 Patch */


			for (j = 0; j < cols; j++)
				if ((Dimension) colWidths[j+col] < width)
					colWidths[j+col] = width;
			for (j = 0; j < rows; j++)
				if ((Dimension) rowHeights[j+row] < height)
					rowHeights[j+row] = height;

			weight = (float)c->gridBag.colWeight / 100.0;
			if (c->gridBag.colWeight > colWeights[col])
				colWeights[col] = weight;
			weight = (float)c->gridBag.rowWeight / 100.0;
			if (c->gridBag.rowWeight > rowWeights[row])
				rowWeights[row] = weight;

			/* PR 5079 Patch                    */
			/* A.J.Fountain, IST, February 1997 */
			if (c->gridBag.cols == XdREMAINDER) {
				lastCol = 0 ;
			}
			else {
				lastCol = col+cols;
			}
			/* END PR 5079 Patch */

			lastRow = row+rows;
		}
	}

	totalWeight = 0;
	totalSize = 0;
	for (col = 0; col < numCols; col++) {
		totalWeight += colWeights[col];
		totalSize += colWidths[col];
	}
	if (totalWeight != 0 && totalSize < w->core.width) {
		remainder = w->core.width - totalSize;
		for (col = 0; col < numCols; col++)
			colWidths[col] += ((float) remainder * colWeights[col]) / totalWeight;
	}

	totalWeight = 0;
	totalSize = 0;
	for (row = 0; row < numRows; row++) {
		totalWeight += rowWeights[row];
		totalSize += rowHeights[row];
	}
	if (totalWeight != 0 && totalSize < w->core.height) {
		remainder = w->core.height - totalSize;
		for (row = 0; row < numRows; row++)
			rowHeights[row] += ((float) remainder * rowWeights[row]) / totalWeight;
	}

	*rNumRows = numRows;
	*rRowHeights = rowHeights;
	*rNumCols = numCols;
	*rColWidths = colWidths;

	return numManaged;
}

/*
** SuperClass (Layout Widget) XdLayoutProc Method
*/

#ifdef    _NO_PROTO
static void GridBagLayout(widget)
	Widget widget ;
#else  /* _NO_PROTO */
static void GridBagLayout(Widget widget)
#endif /* _NO_PROTO */
{
	XdGridBagWidget w = (XdGridBagWidget) widget;
	int i;
	int numManaged, numRows, numCols;
	int maxWidth, maxHeight ;
	int *colWidths, *rowHeights;
	XRectangle widgetRect, cellRect;
	Dimension cellWidth, cellHeight;
	int lastRow = 0, lastCol = 0;

	numManaged = CalcCellSizes(w, &numRows, &rowHeights, &numCols, &colWidths);
	maxWidth = SumArray(colWidths, 0, numCols);
	maxHeight = SumArray(rowHeights, 0, numRows);

	if (numManaged) {
		/* PR 5079 Patch                    */
		/* A.J.Fountain, IST, February 1997 */
		cellWidth  = ((numCols > 0) ? (w->core.width  / (Dimension) numCols) : w->core.width) ;
		cellHeight = ((numRows > 0) ? (w->core.height / (Dimension) numRows) : w->core.height) ;
		/* END PR 5079 Patch */

		if (cellWidth && cellHeight) {
			for (i = 0; i < w->composite.num_children; i++) {
				Widget child = w->composite.children[i];
				if (child->core.managed) {
					XdGridBagConstraintPtr c = GridBagConstraint(child);
					int row, col, rows, cols;
					int alignment, fill;

					row = c->gridBag.row;
					if (row == XdRELATIVE)
						row = lastRow;
					col = c->gridBag.col;
					if (col == XdRELATIVE)
						col = lastCol;

					rows = c->gridBag.rows;
					if (rows == XdREMAINDER)
						rows = numRows-row;
					cols = c->gridBag.cols;
					if (cols == XdREMAINDER)
						cols = numCols-col;

					widgetRect.width = Width(child) + (2 * c->gridBag.padX);
					widgetRect.height = Height(child)+(2 * c->gridBag.padY);

					cellRect.x = SumArray(colWidths, 0, col) + c->gridBag.insetL ;
					cellRect.y = SumArray(rowHeights, 0, row)+ c->gridBag.insetT ;
					cellRect.width = SumArray(colWidths, col, cols) - c->gridBag.insetL - c->gridBag.insetR;
					cellRect.height = SumArray(rowHeights, row, rows) - c->gridBag.insetT - c->gridBag.insetB;
					alignment = GridBagConstraint(child)->gridBag.alignment;
					fill = GridBagConstraint(child)->gridBag.fill;

					XdAlignInCell(&widgetRect, &cellRect, alignment, fill);
					if (widgetRect.width <= 0)
						widgetRect.width = 1;
					if (widgetRect.height <= 0)
						widgetRect.height = 1;
					XtConfigureWidget(child, widgetRect.x, widgetRect.y, widgetRect.width, widgetRect.height, child->core.border_width);


					/* PR 5079 Patch                    */
					/* A.J.Fountain, IST, February 1997 */
					if ( c->gridBag.cols == XdREMAINDER ) {
						lastCol = 0 ;
					}
					else {
						lastCol = col + cols ;
					}
					/* END PR 5079 Patch */

					lastRow = row + rows;
				}
			}
		}
	}

	XtFree((char *) rowHeights) ;
	XtFree((char *) colWidths) ;
}

/*
** Constraint Change Method - if any values change, just recheck everything
*/

/* ARGSUSED */
#ifdef    _NO_PROTO
static Boolean ConstraintSetValues(cw, rw, nw, args, num_args)
	Widget    cw ;
	Widget    rw ;
	Widget    nw ;
	ArgList   args ;
	Cardinal *num_args ;
#else  /* _NO_PROTO */
static Boolean ConstraintSetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
	XdGridBagWidget        t = (XdGridBagWidget) XtParent(nw) ;
	XdGridBagConstraintPtr c = GridBagConstraint(nw) ;

	if (c->gridBag.row < -1) {
		c->gridBag.row = -1 ;
	}
	if (c->gridBag.col < -1) {
		c->gridBag.col = -1 ;
	}
	if (c->gridBag.rows < 0) {
		c->gridBag.rows = 0 ;
	}
	if (c->gridBag.cols < 0) {
		c->gridBag.cols = 0 ;
	}

	t->layout.needsLayout = True ;

	return XdCheckLayout((Widget)t, xdGridBagWidgetClass) ;
}

/*
** SetValues Method - handles any changes to widget instance resource set
*/
  
/* ARGSUSED */
#ifdef    _NO_PROTO
static Boolean SetValues(cw, rw, nw, args, num_args)
        Widget    cw ;
        Widget    rw ;
        Widget    nw ;
        ArgList   args ;
        Cardinal *num_args ;
#else  /* _NO_PROTO */
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
	/* No Resources */
	return False ;
}
 
/*
** SuperClass (Layout Widget) XdPreferredSizeProc Method
*/
 
#ifdef    _NO_PROTO
static void GridBagPreferredSize(widget, width, height)
        Widget         widget ;
        Dimension     *width ;
        Dimension     *height ;
#else  /* _NO_PROTO */
static void GridBagPreferredSize(Widget widget, Dimension *width, Dimension *height)
#endif /* _NO_PROTO */
{
	XdGridBagWidget w = (XdGridBagWidget) widget ;
	int numManaged, numRows, numCols ;
	int *colWidths, *rowHeights ;

	numManaged = CalcCellSizes(w, &numRows, &rowHeights, &numCols, &colWidths) ;
	*width     = SumArray(colWidths, 0, numCols) ;
	*height    = SumArray(rowHeights, 0, numRows) ;

	XtFree((char *) rowHeights) ;
	XtFree((char *) colWidths) ;
}

 
/*
** Convenience Function for creating a Border Layout Widget
*/
 
#ifdef    _NO_PROTO
Widget XdCreateGridBagWidget(parent, name, arglist, argcount)
        Widget   parent ;
        char    *name ;
        ArgList  arglist ;
        Cardinal argcount ;
#else  /* _NO_PROTO */
Widget XdCreateGridBagWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount)
#endif /* _NO_PROTO */
{
	return XtCreateWidget(name, xdGridBagWidgetClass, parent, arglist, argcount) ;
}
