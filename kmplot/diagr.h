/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999, 2000, 2002  Klaus-Dieter Möller <kd.moeller@t-online.de>
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
/** @file diagr.h 
 * @brief Contains the CDiagr class. */

#ifndef diagr_included
#define diagr_included

// standard includes
#include <math.h>
#include <stdio.h>

// Qt includes 
#include <qpainter.h>

//@{
/// Some abbreviations for horizontal and vertical lines.
#define Line drawLine
#define Lineh(x1, y, x2) drawLine( QPointF(x1, y), QPointF(x2, y) )
#define Linev(x, y1, y2) drawLine( QPointF(x, y1), QPointF(x, y2) )
//@}

class QTextDocument;
class QTextEdit;

/// Grid styles.
enum GridStyle
{
	GridNone,
	GridLines,
	GridCrosses,
	GridPolar,
};

class View;

/** @short This class manages the core drawing of the axes and the grid. */
class CDiagr
{
	public:
		/**
		 * \return a pointer to the singleton object of this class.
		 */
		static CDiagr * self();
		
		/// Nothing to do for the destructor.
		~CDiagr();
	
		/// Sets all members to current values.
		void Create( QPoint Ref,
					double lx, double ly,
					double xmin, double xmax,
					double ymin, double ymax );
		/// Sets the current values for the scaling factors
		void Skal( double ex, double ey );
		/// Draws all requested parts of the diagram (axes, labels, grid e.g.)
		void Plot( QPainter* pDC );
		/// Returns the rectangle around the core of the plot area.
		QRect plotArea() const { return m_plotArea; }
		/// Returns the rectangle for the frame around the plot. Extra frame is bigger.
		QRect frame() const { return m_frame; }
		/// Updates the settings from the user (e.g. borderThickness, etc)
		void updateSettings();

		/**
		 * How to behave in the *ToPixel functions.
		 */
		enum ClipBehaviour
		{
			ClipAll,		///< Clips any points going over the edge of the diagram
			ClipInfinite,	///< Clips only infinite and NaN points going over the edge
		};
		/**
		 * @{
		 * @name Transformations
		 * These functions convert real coordinates to pixel coordinates and vice
		 * versa.
		 */
		double xToPixel( double x, ClipBehaviour clipBehaviour = ClipAll );
		double yToPixel( double y, ClipBehaviour clipBehaviour = ClipAll );
		QPointF toPixel( const QPointF & real, ClipBehaviour clipBehaviour = ClipAll );
		double xToReal( double x );
		double yToReal( double y );
		QPointF toReal( const QPointF & pixel );
		///@}
	
	/** @name Style options
	 * These members hold the current options for line widths and colors
	 */
	//@{
	QColor frameColor;	///< color of the border frame
	QColor axesColor;		///< color of the axes
	QColor gridColor;		///< color of the grid

	double borderThickness;	///< current line width for the border frame in mm
	double axesLineWidth;		///< current line width for the axes in mm
	double gridLineWidth;		///< current line width for the grid in mm
	double ticWidth;			///< current line width for the tics in mm
	double ticLength;			///< current length of the tic lines in mm
	//@}
		bool xclipflg;	///< clipflg is set to 1 if the plot is out of the plot area.
		bool yclipflg;	///< clipflg is set to 1 if the plot is out of the plot area.

         
	protected:
		/// Draw the coordinate axes.
		void drawAxes(QPainter*);
		/// Draw the grid.
		void drawGrid( QPainter* );
		/// Write labels.
		void drawLabels(QPainter*);
		/// Current grid style.
		GridStyle m_gridMode;

	//@{
	/// Plot range edge.
	double xmin, xmax, ymin, ymax;
	//@}
	//@{
	/// Clip boundage.
	double xmd, ymd;
	//@}
	//@{
	/// Axes tic distance.
	double ex, ey;  
	//@}
	//@{
	///Position of the first tic.      
	double tsx, tsy;
	//@}
	//@{
	/// Screen coordinates of the coordinate system origin.
	double ox, oy;
	//@}
	//@{
	/// Transformation factors.
	/// @see Skal
	double skx, sky;
	//@}
	
		QRect m_plotArea;	///< plot area
		QRect m_frame;		///< frame around the plot
	
	private:
		static CDiagr * m_self;
		CDiagr();
		
		QTextEdit * m_textEdit; ///< Contains m_textDocument
		QTextDocument * m_textDocument; ///< Used for layout of axis labels
};

#endif // diagr_included
