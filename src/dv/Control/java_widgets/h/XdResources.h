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
** sccsid = "@(#)XdResources.h(1.6)" -- 97/04/24
*/

/*
** XdResources:
**
** Common resources for XdWidgets
*/

#ifndef    _XdResources_h
#define    _XdResources_h

/*
** PR 5355 rework: name clashes with other widgets caused all sorts of problems
** particularly if the widget has a resource of the same name but different size,
** and the widget is placed under constraint by one of the widgets in this set.
**
** Naming scheme reworked, with a path for backwards compatability
*/

#ifdef   XD_OLD_RESOURCE_NAMES
#define XtNxdBorderAlignment      "borderAlignment"
#define XtNxdCellAlignment	  "cellAlignment"
#define XtNxdCellFill             "cellFill"
#define XtNxdColumn               "column"
#define XtNxdColumnWeight         "columnWeight"
#define XtNxdColumns              "columns"
#define XtNxdColumnAlignment      "columnAlignment"
#define XtNxdCurrentPage          "currentPage"
#define XtNxdFillColumn           "fillColumn"
#define XtNxdFillRow              "fillRow"
#define XtNxdHorizontalAlignment  "horizontalAlignment"
#define XtNxdHorizontalSpacing    "horizontalSpacing"
#define XtNxdInsetBottom          "insetBottom"
#define XtNxdInsetLeft            "insetLeft"
#define XtNxdInsetRight           "insetRight"
#define XtNxdInsetTop             "insetTop"
#define XtNxdNumColumns           "numColumns"
#define XtNxdPadX                 "padX"
#define XtNxdPadY                 "padY"
#define XtNxdPageNumber           "pageNumber"
#define XtNxdRow                  "row"
#define XtNxdRowWeight            "rowWeight"
#define XtNxdRows                 "rows"
#define XtNxdRowAlignment         "rowAlignment"
#define XtNxdVerticalAlignment	  "verticalAlignment"
#define XtNxdVerticalSpacing      "verticalSpacing"

#define XtCXdBorderAlignment      "BorderAlignment"
#define XtCXdCellAlignment	  "CellAlignment"
#define XtCXdCellFill             "CellFill"
#define XtCXdColumn               "Column"
#define XtCXdColumnWeight         "ColumnWeight"
#define XtCXdColumns              "Columns"
#define XtCXdColumnAlignment      "ColumnAlignment"
#define XtCXdCurrentPage          "CurrentPage"
#define XtCXdFillColumn           "FillColumn"
#define XtCXdFillRow              "FillRow"
#define XtCXdHorizontalAlignment  "HorizontalAlignment"
#define XtCXdHorizontalSpacing    "HorizontalSpacing"
#define XtCXdInsetBottom          "InsetBottom"
#define XtCXdInsetLeft            "InsetLeft"
#define XtCXdInsetRight           "InsetRight"
#define XtCXdInsetTop             "InsetTop"
#define XtCXdNumColumns           "NumColumns"
#define XtCXdPadX                 "PadX"
#define XtCXdPadY                 "PadY"
#define XtCXdPageNumber           "PageNumber"
#define XtCXdRow                  "Row"
#define XtCXdRowWeight            "RowWeight"
#define XtCXdRows                 "Rows"
#define XtCXdRowAlignment         "RowAlignment"
#define XtCXdVerticalAlignment	  "VerticalAlignment"
#define XtCXdVerticalSpacing      "VerticalSpacing"

#define XtRXdAlignment            "Alignment"
#define XtRXdBorderAlignment      "BorderAlignment"
#define XtRXdCellAlignment	  "CellAlignment"
#define XtRXdCellFill             "CellFill"
#define XtRXdCellPosition         "CellPosition"
#else  /* XD_OLD_RESOURCE_NAMES */
#define XtNxdBorderAlignment      "xdBorderAlignment"
#define XtNxdCellAlignment	  "xdCellAlignment"
#define XtNxdCellFill             "xdCellFill"
#define XtNxdColumn               "xdColumn"
#define XtNxdColumnWeight         "xdColumnWeight"
#define XtNxdColumns              "xdColumns"
#define XtNxdColumnAlignment      "xdColumnAlignment"
#define XtNxdCurrentPage          "xdCurrentPage"
#define XtNxdFillColumn           "xdFillColumn"
#define XtNxdFillRow              "xdFillRow"
#define XtNxdHorizontalAlignment  "xdHorizontalAlignment"
#define XtNxdHorizontalSpacing    "xdHorizontalSpacing"
#define XtNxdInsetBottom          "xdInsetBottom"
#define XtNxdInsetLeft            "xdInsetLeft"
#define XtNxdInsetRight           "xdInsetRight"
#define XtNxdInsetTop             "xdInsetTop"
#define XtNxdNumColumns           "xdNumColumns"
#define XtNxdPadX                 "xdPadX"
#define XtNxdPadY                 "xdPadY"
#define XtNxdPageNumber           "xdPageNumber"
#define XtNxdRow                  "xdRow"
#define XtNxdRowWeight            "xdRowWeight"
#define XtNxdRows                 "xdRows"
#define XtNxdRowAlignment         "xdRowAlignment"
#define XtNxdVerticalAlignment	  "xdVerticalAlignment"
#define XtNxdVerticalSpacing      "xdVerticalSpacing"

#define XtCXdBorderAlignment      "XdBorderAlignment"
#define XtCXdCellAlignment	  "XdCellAlignment"
#define XtCXdCellFill             "XdCellFill"
#define XtCXdColumn               "XdColumn"
#define XtCXdColumnWeight         "XdColumnWeight"
#define XtCXdColumns              "XdColumns"
#define XtCXdColumnAlignment      "XdColumnAlignment"
#define XtCXdCurrentPage          "XdCurrentPage"
#define XtCXdFillColumn           "XdFillColumn"
#define XtCXdFillRow              "XdFillRow"
#define XtCXdHorizontalAlignment  "XdHorizontalAlignment"
#define XtCXdHorizontalSpacing    "XdHorizontalSpacing"
#define XtCXdInsetBottom          "XdInsetBottom"
#define XtCXdInsetLeft            "XdInsetLeft"
#define XtCXdInsetRight           "XdInsetRight"
#define XtCXdInsetTop             "XdInsetTop"
#define XtCXdNumColumns           "XdNumColumns"
#define XtCXdPadX                 "XdPadX"
#define XtCXdPadY                 "XdPadY"
#define XtCXdPageNumber           "XdPageNumber"
#define XtCXdRow                  "XdRow"
#define XtCXdRowWeight            "XdRowWeight"
#define XtCXdRows                 "XdRows"
#define XtCXdRowAlignment         "XdRowAlignment"
#define XtCXdVerticalAlignment	  "XdVerticalAlignment"
#define XtCXdVerticalSpacing      "XdVerticalSpacing"

#define XtRXdAlignment            "XdAlignment"
#define XtRXdBorderAlignment      "XdBorderAlignment"
#define XtRXdCellAlignment	  "XdCellAlignment"
#define XtRXdCellFill             "XdCellFill"
#define XtRXdCellPosition         "XdCellPosition"
#endif /* XD_OLD_RESOURCE_NAMES */


#define XtRXdColumn               XtRInt
#define XtRXdColumnWeight         XtRInt
#define XtRXdColumns              XtRInt
#define XtRXdColumnAlignment      XtRXdAlignment
#define XtRXdCurrentPage          XtRInt
#define XtRXdFillColumn           XtRBoolean
#define XtRXdFillRow              XtRBoolean
#define XtRXdHorizontalAlignment  XtRXdAlignment
#define XtRXdHorizontalSpacing    XtRDimension
#define XtRXdInsetBottom          XtRDimension
#define XtRXdInsetLeft            XtRDimension
#define XtRXdInsetRight           XtRDimension
#define XtRXdInsetTop             XtRDimension
#define XtRXdNumColumns           XtRShort
#define XtRXdPadX                 XtRDimension
#define XtRXdPadY                 XtRDimension
#define XtRXdPageNumber           XtRShort
#define XtRXdRow                  XtRInt
#define XtRXdRowWeight            XtRInt
#define XtRXdRows                 XtRInt
#define XtRXdRowAlignment         XtRXdAlignment
#define XtRXdVerticalAlignment	  XtRXdAlignment
#define XtRXdVerticalSpacing      XtRDimension


#ifdef    XD_BACKWARDS_COMPATABLE_RESOURCE_NAMES
#define XdNborderAlignment        XtNxdBborderAlignment
#define XdNcellAlignment          XtNxdCellAlignment
#define XdNcellFill               XtNxdCellFill
#define XdNcolumn                 XtNxdColumn
#define XdNcolumnWeight           XtNxdColumnWeight
#define XdNcolumns                XtNxdColumns
#define XdNcolumnAlignment        XtNxdColumnAlignment
#define XdNcurrentPage            XtNxdCurrentPage
#define XdNfillColumn             XtNxdFillColumn
#define XdNfillRow                XtNxdFillRow
#define XdNhorizontalAlignment    XtNxdHorizontalAlignment
#define XdNhorizontalSpacing      XtNxdHorizontalSpacing
#define XdNinsetBottom            XtNxdInsetBottom
#define XdNinsetLeft              XtNxdInsetLeft
#define XdNinsetRight             XtNxdInsetRight
#define XdNinsetTop               XtNxdInsetTop
#define XdNnumColumns             XtNxdNumColumns
#define XdNpadX                   XtNxdPadX
#define XdNpadY                   XtNxdPadY
#define XdNpageNumber             XtNxdPageNumber
#define XdNrow                    XtNxdRow
#define XdNrowWeight              XtNxdRowWeight
#define XdNrows                   XtNxdRows
#define XdNrowAlignment           XtNxdRowAlignment
#define XdNverticalAlignment      XtNxdVerticalAlignment
#define XdNverticalSpacing        XtNxdVerticalSpacing

#define XdCBorderAlignment        XtCXdBorderAlignment
#define XdCCellAlignment          XtCXdCellAlignment
#define XdCCellFill               XtCXdCellFill
#define XdCColumn                 XtCXdColumn
#define XdCColumnWeight           XtCXdColumnWeight
#define XdCColumns                XtCXdColumns
#define XdCColumnAlignment        XtCXdColumnAlignment
#define XdCCurrentPage            XtCXdCurrentPage
#define XdCFillColumn             XtCXdFillColumn
#define XdCFillRow                XtCXdFillRow
#define XdCHorizontalAlignment    XtCXdHorizontalAlignment
#define XdCHorizontalSpacing      XtCXdHorizontalSpacing
#define XdCInsetBottom            XtCXdInsetBottom
#define XdCInsetLeft              XtCXdInsetLeft
#define XdCInsetRight             XtCXdInsetRight
#define XdCInsetTop               XtCXdInsetTop
#define XdCNumColumns             XtCXdNumColumns
#define XdCPadX                   XtCXdPadX
#define XdCPadY                   XtCXdPadY
#define XdCPageNumber             XtCXdPageNumber
#define XdCRow                    XtCXdRow
#define XdCRowWeight              XtCXdRowWeight
#define XdCRows                   XtCXdRows
#define XdCRowAlignment           XtCXdRowAlignment
#define XdCVerticalAlignment      XtCXdVerticalAlignment
#define XdCVerticalSpacing        XtCXdVerticalSpacing

#define XdRBorderAlignment        XtRXdBorderAlignment
#define XdRCellAlignment          XtRXdCellAlignment
#define XdRCellFill               XtRXdCellFill
#define XdRColumn                 XtRXdColumn
#define XdRColumnWeight           XtRXdColumnWeight
#define XdRColumns                XtRXdColumns
#define XdRColumnAlignment        XtRXdColumnAlignment
#define XdRCurrentPage            XtRXdCurrentPage
#define XdRFillColumn             XtRXdFillColumn
#define XdRFillRow                XtRXdFillRow
#define XdRHorizontalAlignment    XtRXdHorizontalAlignment
#define XdRHorizontalSpacing      XtRXdHorizontalSpacing
#define XdRInsetBottom            XtRXdInsetBottom
#define XdRInsetLeft              XtRXdInsetLeft
#define XdRInsetRight             XtRXdInsetRight
#define XdRInsetTop               XtRXdInsetTop
#define XdRNumColumns             XtRXdNumColumns
#define XdRPadX                   XtRXdPadX
#define XdRPadY                   XtRXdPadY
#define XdRPageNumber             XtRXdPageNumber
#define XdRRow                    XtRXdRow
#define XdRRowWeight              XtRXdRowWeight
#define XdRRows                   XtRXdRows
#define XdRRowAlignment           XtRXdRowAlignment
#define XdRVerticalAlignment      XtRXdVerticalAlignment
#define XdRVerticalSpacing        XtRXdVerticalSpacing
#endif /* XD_BACKWARDS_COMPATABLE_RESOURCE_NAMES */

#define XdALIGN_CENTER          0
#define XdALIGN_BEGINNING       1
#define XdALIGN_END             2

#define XdCELL_ALIGN_CENTER     0
#define XdCELL_ALIGN_EAST       1
#define XdCELL_ALIGN_NORTH      2
#define XdCELL_ALIGN_NORTHEAST  3
#define XdCELL_ALIGN_NORTHWEST  4
#define XdCELL_ALIGN_SOUTH      5
#define XdCELL_ALIGN_SOUTHEAST  6
#define XdCELL_ALIGN_SOUTHWEST  7
#define XdCELL_ALIGN_WEST       8

#define XdCELL_FILL_NONE        0
#define XdCELL_FILL_HORIZONTAL  1
#define XdCELL_FILL_VERTICAL    2
#define XdCELL_FILL_BOTH        3

#define XdBORDER_NORTH          0
#define XdBORDER_CENTER         1
#define XdBORDER_SOUTH          2
#define XdBORDER_EAST           3
#define XdBORDER_WEST           4


#ifndef    ENUMERATIONS_UNSUPPORTED
typedef enum
{
	XdAlignmentCenter    = XdALIGN_CENTER,
	XdAlignmentBeginning = XdALIGN_BEGINNING,
	XdAlignmentEnd       = XdALIGN_END
} XdAlignment ;

typedef enum
{
	XdCellAlignmentCenter    = XdCELL_ALIGN_CENTER,
	XdCellAlignmentEast      = XdCELL_ALIGN_EAST,
	XdCellAlignmentNorth     = XdCELL_ALIGN_NORTH,
	XdCellAlignmentNorthEast = XdCELL_ALIGN_NORTHEAST,
	XdCellAlignmentNorthWest = XdCELL_ALIGN_NORTHWEST,
	XdCellAlignmentSouth     = XdCELL_ALIGN_SOUTH,
	XdCellAlignmentSouthEast = XdCELL_ALIGN_SOUTHEAST,
	XdCellAlignmentSouthWest = XdCELL_ALIGN_SOUTHWEST,
	XdCellAlignmentWest      = XdCELL_ALIGN_WEST
} XdCellAlignment ;

typedef enum
{
	XdCellFillNone       = XdCELL_FILL_NONE,
	XdCellFillHorizontal = XdCELL_FILL_HORIZONTAL,
	XdCellFillVertical   = XdCELL_FILL_VERTICAL,
	XdCellFillBoth       = XdCELL_FILL_BOTH
} XdCellFill ;

typedef enum
{
	XdBorderAlignmentNorth  = XdBORDER_NORTH,
	XdBorderAlignmentCenter = XdBORDER_CENTER,
	XdBorderAlignmentSouth  = XdBORDER_SOUTH,
	XdBorderAlignmentEast   = XdBORDER_EAST,
	XdBorderAlignmentWest   = XdBORDER_WEST
} XdBorderAlignment ;
#else  /* ENUMERATIONS_UNSUPPORTED */
#define XdAlignmentCenter          XdALIGN_CENTER
#define XdAlignmentBeginning       XdALIGN_BEGINNING
#define XdAlignmentEnd             XdALIGN_END

#define XdCellAlignmentCenter      XdCELL_ALIGN_CENTER
#define XdCellAlignmentEast        XdCELL_ALIGN_EAST
#define XdCellAlignmentNorth       XdCELL_ALIGN_NORTH
#define XdCellAlignmentNorthEast   XdCELL_ALIGN_NORTHEAST
#define XdCellAlignmentNorthWest   XdCELL_ALIGN_NORTHWEST
#define XdCellAlignmentSouth       XdCELL_ALIGN_SOUTH
#define XdCellAlignmentSouthEast   XdCELL_ALIGN_SOUTHEAST
#define XdCellAlignmentSouthWest   XdCELL_ALIGN_SOUTHWEST
#define XdCellAlignmentWest        XdCELL_ALIGN_WEST

#define XdCellFillNone             XdCELL_FILL_NONE
#define XdCellFillHorizontal       XdCELL_FILL_HORIZONTAL
#define XdCellFillVertical         XdCELL_FILL_VERTICAL
#define XdCellFillBoth             XdCELL_FILL_BOTH

#define XdBorderAlignmentNorth     XdBORDER_NORTH
#define XdBorderAlignmentCenter    XdBORDER_CENTER
#define XdBorderAlignmentSouth     XdBORDER_SOUTH
#define XdBorderAlignmentEast      XdBORDER_EAST
#define XdBorderAlignmentWest      XdBORDER_WEST
#endif /* ENUMERATIONS_UNSUPPORTED */

#endif  /* _XdResources_h */
