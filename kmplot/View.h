/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999  Klaus-Dieter Möller
*               2000, 2002 kd.moeller@t-online.de
*                     2006 David Saxton <david@bluehaze.org>
*               
* This file is part of the KDE Project.
* KmPlot is part of the KDE-EDU Project.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
*/

#ifndef View_included
#define View_included
#undef	 GrayScale

// Qt includes
#include <qpixmap.h>
#include <QPointer>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QEvent>

// KDE includes
#include <dcopclient.h>
#include <kactionclasses.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kprinter.h>
#include <kprogressbar.h>
#include <kpushbutton.h>


// local includes
#include "diagr.h"
#include "Viewiface.h"
#include "xparser.h"

class KMinMax;
class KSliderWindow;
class MainDlg;
class QPaintEvent;
class QTime;
class XParser;


/**
 * For drawing the area of a (Cartesian) plot.
 */
 class IntegralDrawSettings
{
	public:
		IntegralDrawSettings();
		
		int functionID;
		Function::PMode pMode;
		double dmin, dmax;
		QString parameter; ///< parameter for the function
};


/**
 * @short This class contains the plots. 
 *
 * It is the central widget of MainDlg.
 * @see MainDlg, MainDlg::view
 */
class View : public QWidget, virtual public ViewIface
{
	Q_OBJECT
	public:
		/// Contructor sets up the parser, too.
		View( bool readOnly, bool & modified, KMenu * functionPopup, QWidget* parent, KActionCollection *ac );
		void setMinMaxDlg(KMinMax *);
		virtual ~View();
	
		/// There is only one view.
		static View * self() { return m_self; }

		enum PlotMedium
		{
			Screen,
			Printer,
			SVG,
			Pixmap,
		};
		/// Reimplemented to draw all stuff to the view.
		void draw( QPaintDevice *, PlotMedium medium );
		
		/// Getting all relevant settings using KConfig XT class Settings.
		void getSettings();
		/// Clears all functions in the parser and gets default settings.
		/// @see getSettings
		void init();
		enum ExtremaType { Minimum, Maximum };
		/**
		* Finding the minimum or maximum value.
		* \return The (x,y) coordinates of the extrema point.
		*/
		QPointF findMinMaxValue( Function * function, Function::PMode p_mode, ExtremaType type, double dmin, double dmax,const QString & parameter );
		/// get a y-value from a x-value
		double getYValue( Function * function, Function::PMode p_mode, double x, const QString & parameter );
		/**
		* Calculates the area between the given plot and the x-axis
		* (from x = \p dmin to x = \p dmax). The area will also be colored in.
		* \return the area.
		*/
		double areaUnderGraph( IntegralDrawSettings settings );
	/// the calculation was cancelled by the user
	bool isCalculationStopped();

	/// Slider controlling parameter values
	QPointer<KSliderWindow> m_sliderWindow;
	/// Menu actions for the sliders
	KToggleAction * m_menuSliderAction;
	void updateSliders(); /// show only needed sliders
	
	/// Convert a width in mm to a suitable QPen width for drawing
	double mmToPenWidth( double width_mm, bool antialias ) const;

		/** Current plot range endge. */
		double m_xmin;
		double m_xmax;

		/// trace mode stuff, must be accessible in KMinMax
		int m_currentFunctionID;
		int m_currentFunctionParameter;
		Function::PMode m_currentFunctionPlot;

public slots:
	/// Called when the user want to cancel the drawing
	void stopDrawing();
	/// A slider window has been closed
	void slidersWindowClosed();

	/// Called when the graph should be updated
	void drawPlot();
	///Slots for the three first items in popup menu
	void mnuHide_clicked();
	void mnuRemove_clicked();
	void mnuEdit_clicked();
	///Slots for the zoom menu
	void mnuZoomIn_clicked();
	void mnuZoomOut_clicked();
	void mnuTrig_clicked();

protected slots:
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *);
	/// Updating the cross hair.
	void mouseMoveEvent(QMouseEvent *);
	/// Toggles the trace mode if the cursor is near to a plot.
	void mousePressEvent(QMouseEvent *);
	/// when a key is pressed and the graph widget has focus
	void keyPressEvent(QKeyEvent * );
	/// called when a mouse key is released
	void mouseReleaseEvent ( QMouseEvent * e );
	/// Is needed to be reimplement so that the user can stop a preview-drawing
	bool event( QEvent * e );
	/// Restore the mouse cursor when a drawing is finished
	void updateCursor();
	/**
	 * Updates csxpos and csypos from the current mouse position.
	 * @return whether the crosshair is within the bounds of the diagram.
	 */
	bool updateCrosshairPosition();

signals:
	void setStatusBarText(const QString &);
	
protected:
	/// called when focus is lost
	virtual void focusOutEvent( QFocusEvent * );
	/// called when focus is gained
	virtual void focusInEvent( QFocusEvent * );

private:
	/// Print out table with additional information.
	/// Only for printing.
	void drawHeaderTable(QPainter *);
	/// Draw the function plots.
	void plotfkt(Function *ufkt, QPainter*);
	/// @return an appropriate pen for drawing the plot
	QPen penForPlot( Function * ufkt, Function::PMode, bool antialias ) const;
	/// Gets the greek pi symbol.
	void setpi(QString *);
	/// in trace mode checks, if the function is (near by) zero
	bool root(double *, Equation *);
	///return the inverted color
	void invertColor(QColor &, QColor &);
	/// Changes the text in the statusbar
	void setStatusBar(const QString &text, const int id);
	/// Functions for the progressbar
	bool stopProgressBar();
	void startProgressBar(int);
	void increaseProgressBar();
	/// @return whether the crosshairs should be shown for the current mouse position
	bool shouldShowCrosshairs() const;
	/**
	 * Zooms in by amount \p zoomFactor (which will zooming out if less than 1)
	 * from clicking at \p mousePos (in widget coordinates).
	 */
	void zoomIn( const QPoint & mousePos, double zoomFactor );
	/// zooms in from having drawn \p zoomRect (which is in widget coordinates)
	void zoomIn( const QRect & zoomRect );
	/// zooms out from havoutg drawn \p zoomRect (which is out widget coordoutates)
	void zoomOut( const QRect & zoomRect );
	/// translates the view by \p dx, \p dy (in widget coordinates)
	void translateView( int dx, int dy );
	/// animates zooming from the current zoom rect to the one given (in real coordinates)
	void animateZoom( const QRectF & newCoords );
	/**
	 * Finds the plot (if any) under the last mouse pos as recorded by
	 * updateCrosshairPosition(). This sets csmode, cstype, csparam. If no plot
	 * was found, then csmode is set to -1.
	 */
	void getPlotUnderMouse();
	/**
	 * Finds the closest point to \p pos to the given function.
	 * \return the parametization (angle or t) that gives the closest point.
	 */
	double getClosestPoint( const QPointF & pos, Function * function, Function::PMode mode );
	/**
	 * Calculates the pixel distance from \p pos to the display point of the
	 * given function at \p x.
	 */
	double pixelDistance( const QPointF & pos, Function * function, Function::PMode mode, double x );
	/**
	 * @return a string for displaying the x or y coordinate in the statusbar.
	 * \param delta is the amount by which the value varies over one pixel in
	 * the view. This is for choosing an appropriate number of decimals so that
	 * moving the cursor shows a nice change in the string.
	 */
	QString posToString( double x, double delta ) const;
	
		/**
		 * Convenience function for calculating the value of \p eq using the
		 * given \p mode
		 */
		double value( Equation * eq, Function::PMode mode, double x );
		/**
		 * \return the real position of the function (similar to calling
		 * value(), but returns both coordinates).
		 */
		QPointF realValue( Function * function, Function::PMode mode, double x );
		/**
		 * \return an appropriate xmin value for the given function
		 * plotting.
		 */
		double getXmin( Function * function );
		/**
		 * \return an appropriate xmax value for the given function for
		 * plotting.
		 */
		double getXmax( Function * function );
	
		/// for areadrawing
		IntegralDrawSettings m_integralDrawSettings;
		bool m_drawIntegral;
	
		double m_width, m_height;
		float m_scaler;
	
		QPointF m_crosshairPixelCoords;
		QPointF m_crosshairPosition;	///< in real coordinates
	
	/// The t- or x- (angle) coordinate of the traced curve
	double m_trace_x;
	
	/// trace mode stuff
	bool rootflg;

		/// @return whether cspos is in the range of the view or in the custom range for the given \p plot
		bool crosshairPositionValid( Function * plot ) const;
	
	CDiagr dgr;	///< Coordinate system
	QPoint ref;
	QRect area,
	PlotArea;
	QMatrix wm;

	double tlgx, tlgy, drskalx, drskaly;
	QString tlgxstr, tlgystr, drskalxstr, drskalystr;

	/** @name Plotrange
	 * There are 4 predefined plot ranges:
	 * @li 0: -8..8
	 * @li 1: -5..5
	 * @li 2: 0..16
	 * @li 3: 0..10
	 * @li 4: custom
	 */
	//@{
	///Convert axes range predefinition index to boundaries.
	void getMinMax(int koord, QString &mini, QString &maxi);
	/** Handle predefiend axes ranges.
	*
	* @p koord can have the values 0 to 4 which have the following meanings: 
	*
	* In the last case @p minstr and @p maxstr are evaluated.
	*/
	void coordToMinMax( const int koord, const QString &minStr, const QString &maxStr,
	                    double &min, double &max );
		//@{
		/** Current plot range endge. */
		double m_ymin;
		double m_ymax;
		//@}
		//@}

	void setScaling();
	/// represents the KPrinter option app-kmplot-printtable.
	/// @see KPrinterDlg
	bool m_printHeaderTable;
	/// if stop_calculating is true, the user has canceled drawing of an integral graph
	bool stop_calculating;
	/// the background color of the graph
	QColor backgroundcolor;
	/// the inverted background color used by the "Fadenkreuz"
	QColor inverted_backgroundcolor;
	/// pointer to KMinMax
	KMinMax *m_minmax;
	///buffer the current window so all functions don't need to be re-drawed
	QPixmap buffer;
	/// the popup menu
	KMenu *m_popupmenu;
	/// is set to true if an integral is calculated
	bool isDrawing;
	///status of the popup menu
	char m_popupmenushown; /// 0==no popup 1==popup 2==popup+trace mode before
	/// true == modifications not saved
	bool &m_modified;
	/// False if KmPlot is started as a program, otherwise true
	bool const m_readonly;
	
	enum ZoomMode
	{
		Normal,				///< no zooming
		AnimatingZoom,		///< animating a current zooming
		ZoomIn,				///< zoom in
		ZoomOut,			///< zoom out
		ZoomInDrawing,		///< drawing a rectangle for zooming in
		ZoomOutDrawing,		///< drawing a rectangle for zooming out
		AboutToTranslate,	///< user has clicked on an empty spot, but hasn't moved the mouse yet
		Translating,		///< dragging the view with the mouse
	};
		
	/// The current editing status
	ZoomMode m_zoomMode;
	/// for zoom-mode
	QPoint m_zoomRectangleStart;
	/// for animating zoom; contains the rectangle (in real coordinates) to draw
	QRectF m_animateZoomRect;
	/// for translating the view via dragging
	QPoint m_prevDragMousePos;
	/// timer that is started when the mouse is pressed
	QTime * m_mousePressTimer;

	DCOPClient *m_dcop_client;
	QString m_statusbartext1;
	QString m_statusbartext2;
	QString m_statusbartext3;
	QString m_statusbartext4;
	KActionCollection *m_ac;
	
	enum Cursor { CursorWait, CursorBlank, CursorArrow, CursorCross, CursorMagnify, CursorLessen, CursorMove };
	Cursor m_prevCursor;
	
	static View * m_self;
};

#endif // View_included
