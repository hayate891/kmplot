/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999  Klaus-Dieter Möller
*               2000, 2002 kd.moeller@t-online.de
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
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/
// KDE includes
#include <kmessagebox.h>
#include <kurl.h>

// local includes
#include "misc.h"
#include "settings.h"

KApplication *ka;

XParser ps( 10, 200, 20 );

int mode;     // Diagrammodus
// g_mode;  // grid style

int koordx,     // 0 => [-8|+8]
koordy,     // 1 => [-5|+5]
// 2 => [0|+16]
// 3 => [0|+10]
axesThickness,
gridThickness,
gradThickness,
gradLength;

double xmin,                      // min. x-Wert
xmax,                      // max. x-Wert
ymin,                   // min. y-Wert
ymax,                   // max. y-Wert
sw,       // Schrittweite
rsw,       // rel. Schrittweite
tlgx,       // x-Achsenteilung
tlgy,       // y-Achsenteilung
drskalx,       // x-Ausdruckskalierung
drskaly;     // y-Ausdruckskalierung

QString xminstr,      // String fr xmind
xmaxstr,      // String fr xmaxd
yminstr,      // String fr ymind
ymaxstr,      // String fr ymaxd
tlgxstr,                  // String fr tlgx
tlgystr,                  // String fr tlgy
drskalxstr,               // String fr drskalx
drskalystr;             // String fr drskaly

QString font_header, font_axes; // Font family names

bool printtable;		// header table printing option


void getSettings()
{
	mode = AXES | ARROWS | EXTFRAME;
	rsw = 1.;

	// axes settings
	
	koordx = Settings::xRange();
	koordy = Settings::yRange();
	xminstr = Settings::xMin();
	xmaxstr = Settings::xMax();
	yminstr = Settings::yMin();
	ymaxstr = Settings::yMax();
	
	if( xminstr.isEmpty() ) xminstr = "-2*pi";
	if( xmaxstr.isEmpty() ) xmaxstr = "2*pi";
	if( yminstr.isEmpty() ) yminstr = "-2*pi";
	if( ymaxstr.isEmpty() ) ymaxstr = "2*pi";

	if ( !coordToMinMax( koordx, xmin, xmax, xminstr, xmaxstr ) )
	{
		KMessageBox::error( 0, i18n( "Config file x-axis entry corrupt.\n"
		                             "Fall back to system defaults.\nCall Settings->Configure KmPlot..." ) );
		xminstr = "-2*pi";
		xmaxstr = "2*pi";
		koordx = 0;
		coordToMinMax( koordx, xmin, xmax, xminstr, xmaxstr );
	}
	if ( !coordToMinMax( koordy, ymin, ymax, yminstr, ymaxstr ) )
	{
		KMessageBox::error( 0, i18n( "Config file y-axis entry corrupt.\n"
		                             "Fall back to system defaults.\nCall Settings->Configure KmPlot..." ) );
		yminstr = "-2*pi";
		ymaxstr = "2*pi";
		koordy = 0;
		coordToMinMax( koordy, ymin, ymax, yminstr, ymaxstr );
	}
	
	const char* units[ 8 ] = { "10", "5", "2", "1", "0.5", "pi/2", "pi/3", "pi/4" };
	
	tlgxstr = units[ Settings::xScaling() ];
	tlgx = ps.eval( tlgxstr );
	tlgystr = units[ Settings::yScaling() ];
	tlgy = ps.eval( tlgystr );

	drskalxstr = units[ Settings::xPrinting() ];
	drskalx = ps.eval( drskalxstr );
	drskalystr = units[ Settings::yPrinting() ];
	drskaly = ps.eval( drskalystr );

	axesThickness = Settings::axesLineWidth();
	if ( Settings::showLabel() ) mode |= LABEL;
	gradThickness = Settings::ticWidth();
	gradLength = Settings::ticLength();

	// grid settings

	gridThickness = Settings::gridLineWidth();

	// font settings
	font_header = Settings::headerTableFont().family();
	font_axes = Settings::axesFont().family();

	// graph settings

	ps.dicke0 = Settings::gridLineWidth();
	ps.fktext[ 0 ].color = Settings::color0().rgb();
	ps.fktext[ 1 ].color = Settings::color1().rgb();
	ps.fktext[ 2 ].color = Settings::color2().rgb();
	ps.fktext[ 3 ].color = Settings::color3().rgb();
	ps.fktext[ 4 ].color = Settings::color4().rgb();
	ps.fktext[ 5 ].color = Settings::color5().rgb();
	ps.fktext[ 6 ].color = Settings::color6().rgb();
	ps.fktext[ 7 ].color = Settings::color7().rgb();
	ps.fktext[ 8 ].color = Settings::color8().rgb();
	ps.fktext[ 9 ].color = Settings::color9().rgb();
	printtable = true;
	
	// precision settings
	rsw = Settings::stepWidth();
}

void init()
{
	getSettings();

	for ( int ix = 0; ix < ps.ufanz; ++ix )
		ps.delfkt( ix );
}

/*
 * evaluates the predefined axes settings (kkordx/y)
 */
bool coordToMinMax( const int koord, double &min, double &max, const QString minStr, const QString maxStr )
{
	bool ok = true;
	switch ( koord )
	{
	case 0:
		min = -8.0;
		max = 8.0;
		break;
	case 1:
		min = -5.0;
		max = 5.0;
		break;
	case 2:
		min = 0.0;
		max = 16.0;
		break;
	case 3:
		min = 0.0;
		max = 10.0;
		break;
	case 4:
		min = ps.eval( minStr );
		ok = ps.err == 0;
		max = ps.eval( maxStr );
		ok &= ps.err == 0;
	}
	ok &= min < max;
	return ok;
}

