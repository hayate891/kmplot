/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999, 2000, 2002  Klaus-Dieter M�ler <kd.moeller@t-online.de>
*               2006 David Saxton <david@bluehaze.org>
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "function.h"
#include "ksliderwindow.h"
#include "settings.h"
#include "View.h"
#include "xparser.h"

#include <kdebug.h>

#include <assert.h>
#include <cmath>


//BEGIN class Value
Value::Value( const QString & expression )
{
	m_value = 0.0;
	if ( expression.isEmpty() )
		m_expression = "0";
	else
		updateExpression( expression );
}


bool Value::updateExpression( const QString & expression )
{
	double newValue = XParser::self()->eval( expression );
	if ( XParser::self()->parserError( false ) )
		return false;
	
	m_value = newValue;
	m_expression = expression;
	return true;
}


bool Value::operator == ( const Value & other )
{
	return m_expression == other.expression();
}
//END class Value



//BEGIN class PlotAppearance
PlotAppearance::PlotAppearance( )
{
	lineWidth = 0.2;
	color = Qt::black;
	useGradient = false;
	color1 = QColor( 0xff, 0xff, 0x00 );
	color2 = QColor( 0xff, 0x00, 0x00 );
	visible = false;
	style = Qt::SolidLine;
	showExtrema = false;
}


bool PlotAppearance::operator !=( const PlotAppearance & other ) const
{
	return
			(lineWidth != other.lineWidth) ||
			(color != other.color) ||
			(useGradient != other.useGradient) ||
			(color1 != other.color1) ||
			(color2 != other.color2) ||
			(visible != other.visible) ||
			(style != other.style) ||
			(showExtrema != other.showExtrema);
}


QString PlotAppearance::penStyleToString( Qt::PenStyle style )
{
	switch ( style )
	{
		case Qt::NoPen:
			return "NoPen";
			
		case Qt::SolidLine:
			return "SolidLine";
			
		case Qt::DashLine:
			return "DashLine";
			
		case Qt::DotLine:
			return "DotLine";
			
		case Qt::DashDotLine:
			return "DashDotLine";
		
		case Qt::DashDotDotLine:
			return "DashDotDotLine";
			
		case Qt::MPenStyle:
		case Qt::CustomDashLine:
			kWarning() << "Unsupported pen style\n";
			break;
	}
	
	kWarning() << k_funcinfo << "Unknown style " << style << endl;
	return "SolidLine";
}


Qt::PenStyle PlotAppearance::stringToPenStyle( const QString & style )
{
	if ( style == "NoPen" )
		return Qt::NoPen;
			
	if ( style == "SolidLine" )
		return Qt::SolidLine;
			
	if ( style == "DashLine" )
		return Qt::DashLine;
			
	if ( style == "DotLine" )
		return Qt::DotLine;
			
	if ( style == "DashDotLine" )
		return Qt::DashDotLine;
		
	if ( style == "DashDotDotLine" )
		return Qt::DashDotDotLine;
	
	kWarning() << k_funcinfo << "Unknown style " << style << endl;
	return Qt::SolidLine;
}
//END class PlotAppearance



//BEGIN class Equation
Equation::Equation( Type type, Function * parent )
	: m_type( type ),
	  m_parent( parent )
{
	mem = new unsigned char [MEMSIZE];
	mptr = 0;
}


Equation::~ Equation()
{
	delete [] mem;
	mem = 0;
}


QString Equation::name( ) const
{
	if ( m_fstr.isEmpty() )
		return QString();
	
	int pos = m_fstr.indexOf( '(' );
	
	if ( pos == -1 )
		return QString();
	
	return m_fstr.left( pos );
}


QStringList Equation::parameters( ) const
{
	int p1 = m_fstr.indexOf( '(' );
	int p2 = m_fstr.indexOf( ')' );
	if ( (p1 == -1) || (p2 == -1) )
		return QStringList();
	
	QString parameters = m_fstr.mid( p1+1, p2-p1-1 );
	return parameters.split( ',' );
}


bool Equation::setFstr( const QString & fstr, bool force  )
{
// 	kDebug() << k_funcinfo << "fstr: "<<fstr<<endl;
	
	if ( force )
	{
		m_fstr = fstr;
		resetLastIntegralPoint();
		return true;
	}
	
	if ( !XParser::self()->isFstrValid( fstr ) )
	{
		XParser::self()->parserError( false );
		kDebug() << k_funcinfo << "invalid fstr\n";
		return false;
	}
	
	QString prevFstr = m_fstr;
	m_fstr = fstr;
	XParser::self()->initEquation( this );
	if ( XParser::self()->parserError( false ) != Parser::ParseSuccess )
	{
		m_fstr = prevFstr;
		XParser::self()->initEquation( this );
// 		kDebug() << k_funcinfo << "BAD\n";
		return false;
	}
	
	resetLastIntegralPoint();
	return true;
}


void Equation::resetLastIntegralPoint( )
{
	lastIntegralPoint = QPointF( m_startX.value(), m_startY.value() );
}


void Equation::setIntegralStart( const Value & x, const Value & y )
{
	m_startX = x;
	m_startY = y;
	
	resetLastIntegralPoint();
}


bool Equation::operator !=( const Equation & other )
{
	return (fstr() != other.fstr()) ||
			(integralInitialX() != other.integralInitialX()) ||
			(integralInitialY() != other.integralInitialY());
}


Equation & Equation::operator =( const Equation & other )
{
	setFstr( other.fstr() );
	setIntegralStart( other.integralInitialX(), other.integralInitialY() );
	
	return * this;
}
//END class Equation



//BEGIN class Function
Function::Function( Type type )
	: m_type( type )
{
	x = y = 0;
	m_implicitMode = UnfixedXY;
	
	eq[0] = eq[1] = 0;
	usecustomxmin = false;
	usecustomxmax = false;
	
	switch ( m_type )
	{
		case Cartesian:
			eq[0] = new Equation( Equation::Cartesian, this );
			dmin.updateExpression( QString("-")+QChar(960) );
			dmax.updateExpression( QChar(960) );
			break;
			
		case Polar:
			eq[0] = new Equation( Equation::Polar, this );
			dmin.updateExpression( QChar('0') );
			dmax.updateExpression( QString(QChar('2')) + QChar(960) );
			usecustomxmin = true;
			usecustomxmax = true;
			break;
			
		case Parametric:
			eq[0] = new Equation( Equation::ParametricX, this );
			eq[1] = new Equation( Equation::ParametricY, this );
			dmin.updateExpression( QString("-")+QChar(960) );
			dmax.updateExpression( QChar(960) );
			usecustomxmin = true;
			usecustomxmax = true;
			break;
			
		case Implicit:
			eq[0] = new Equation( Equation::Implicit, this );
			break;
	}
	
	id = 0;
	f0.visible = true;
	integral_use_precision = false;
	
	k = 0;
	integral_precision = 1.0;
}


Function::~Function()
{
	for ( unsigned i = 0; i < 2; ++i )
	{
		delete eq[i];
		eq[i] = 0;
	}
}


bool Function::copyFrom( const Function & function )
{
	bool changed = false;
	int i = 0;
#define COPY_AND_CHECK(s) \
	{ \
		if ( s != function.s ) \
		{ \
			s = function.s; \
			changed = true; \
		} \
	} \
	i++;
	
	COPY_AND_CHECK( f0 );						// 0
	COPY_AND_CHECK( f1 );						// 1
	COPY_AND_CHECK( f2 );						// 2
	COPY_AND_CHECK( integral );					// 3
	COPY_AND_CHECK( integral_use_precision );	// 4
	COPY_AND_CHECK( dmin );						// 5
	COPY_AND_CHECK( dmax );						// 6
	COPY_AND_CHECK( integral_precision );		// 7
	COPY_AND_CHECK( usecustomxmin );			// 8
	COPY_AND_CHECK( usecustomxmax );			// 9
	COPY_AND_CHECK( m_parameters );				// 10
	
	// handle equations separately
	for ( int i = 0; i < 2; ++i )
	{
		if ( !eq[i] || !function.eq[i] )
			continue;
		
		if ( *eq[i] != *function.eq[i] )
		{
			changed = true;
			*eq[i] = *function.eq[i];
		}
	}
	
// 	kDebug() << k_funcinfo << "changed="<<changed<<endl;
	return changed;
}


QString Function::prettyName( Function::PMode mode ) const
{
	if ( type() == Parametric )
		return eq[0]->fstr() + " ; " + eq[1]->fstr();
	
	switch ( mode )
	{
		case Function::Derivative0:
			return eq[0]->fstr();
			
		case Function::Derivative1:
			return eq[0]->name() + '\'';
			
		case Function::Derivative2:
			return eq[0]->name() + "\'\'";
			
		case Function::Integral:
			return eq[0]->name().toUpper();
	}
	
	kWarning() << k_funcinfo << "Unknown mode!\n";
	return "???";
}


PlotAppearance & Function::plotAppearance( PMode plot )
{
	// NOTE: This function is identical to the const one, so changes to this should be applied to both
	
	switch ( plot )
	{
		case Function::Derivative0:
			return f0;
		case Function::Derivative1:
			return f1;
			
		case Function::Derivative2:
			return f2;
			
		case Function::Integral:
			return integral;
	}
	
	kError() << k_funcinfo << "Unknown plot " << plot << endl;
	return f0;
}
PlotAppearance Function::plotAppearance( PMode plot ) const
{
	// NOTE: This function is identical to the none-const one, so changes to this should be applied to both
	
	switch ( plot )
	{
		case Function::Derivative0:
			return f0;
		case Function::Derivative1:
			return f1;
		case Function::Derivative2:
			return f2;
		case Function::Integral:
			return integral;
	}
	
	kError() << k_funcinfo << "Unknown plot " << plot << endl;
	return f0;
}


bool Function::allPlotsAreHidden( ) const
{
	return !f0.visible && !f1.visible && !f2.visible && !integral.visible;
}


QString Function::typeToString( Type type )
{
	switch ( type )
	{
		case Cartesian:
			return "cartesian";
			
		case Parametric:
			return "parametric";
			
		case Polar:
			return "polar";
			
		case Implicit:
			return "implicit";
	}
	
	kWarning() << "Unknown type " << type << endl;
	return "unknown";
}


Function::Type Function::stringToType( const QString & type )
{
	if ( type == "cartesian" )
		return Cartesian;
	
	if ( type == "parametric" )
		return Parametric;
	
	if ( type == "polar" )
		return Polar;
	
	if ( type == "implicit" )
		return Implicit;
	
	kWarning() << "Unknown type " << type << endl;
	return Cartesian;
}


QList< Plot > Function::allPlots( ) const
{
	QList< Plot > list;
	
	for ( PMode p = Derivative0; p <= Integral; p = PMode(p+1) )
	{
		int i = 0;
		
		if ( !plotAppearance( p ).visible )
			continue;
		
		Plot plot;
		plot.setFunctionID( id );
		plot.plotMode = p;
		plot.plotNumberCount = m_parameters.useList ? m_parameters.list.size() + (m_parameters.useSlider?1:0) : 1;
		
		bool usedParameter = false;
		
		// Don't use slider or list parameters if animating
		
		if ( !m_parameters.animating && m_parameters.useSlider )
		{
			Parameter param( Parameter::Slider );
			param.setSliderID( m_parameters.sliderID );
			plot.parameter = param;
			plot.plotNumber = i++;
			list << plot;
			usedParameter = true;
		}
		
		if ( !m_parameters.animating && m_parameters.useList )
		{
			int pos = 0;
			foreach ( Value v, m_parameters.list )
			{
				Parameter param( Parameter::List );
				param.setListPos( pos++ );
				plot.parameter = param;
				plot.plotNumber = i++;
				list << plot;
				usedParameter = true;
			}
		}
		
		if ( m_parameters.animating )
		{
			Parameter param( Parameter::Animated );
			plot.parameter = param;
		}
		
		if ( !usedParameter )
			list << plot;
	}
	
	return list;
}
//END class Function



//BEGIN class ParameterSettings
ParameterSettings::ParameterSettings()
{
	animating = false;
	useSlider = false;
	sliderID = 0;
	useList = false;
}


bool ParameterSettings::operator == ( const ParameterSettings & other ) const
{
	return ( useSlider == other.useSlider ) &&
			( sliderID == other.sliderID ) &&
			( useList == other.useList ) &&
			( list == other.list );
}
//END class ParameterSettings



//BEGIN class Parameter
Parameter::Parameter( Type type )
	: m_type( type )
{
	m_sliderID = -1;
	m_listPos = -1;
}


bool Parameter::operator == ( const Parameter & other ) const
{
	return ( type() == other.type() ) &&
			( listPos() == other.listPos() ) &&
			( sliderID() == other.sliderID() );
}
//END class Parameter



//BEGIN class Plot
Plot::Plot( )
{
	plotNumberCount = 1;
	plotNumber = 0;
	m_function = 0;
	m_functionID = -1;
	plotMode = Function::Derivative0;
}


bool Plot::operator ==( const Plot & other ) const
{
	return ( m_functionID == other.functionID() ) &&
			( plotMode == other.plotMode ) &&
			( parameter == other.parameter );
}


void Plot::setFunctionID( int id )
{
	m_functionID = id;
	updateCached();
}


void Plot::updateCached()
{
	m_function = XParser::self()->functionWithID( m_functionID );
}


void Plot::updateFunctionParameter() const
{
	if ( !m_function )
		return;
	
	double k = 0.0;
	
	switch ( parameter.type() )
	{
		case Parameter::Unknown:
			break;
			
		case Parameter::Slider:
		{
			KSliderWindow * sw = View::self()->m_sliderWindow;
			
			if ( !sw )
			{
				// Slider window isn't open. Ask View to open it
				View::self()->updateSliders();
				
				// It should now be open
				sw = View::self()->m_sliderWindow;
				assert( sw );
			}
			
			k = sw->value( parameter.sliderID() );
			break;
		}
			
		case Parameter::List:
		{
			if ( (parameter.listPos() >= 0) && (parameter.listPos() < m_function->m_parameters.list.size()) )
				k = m_function->m_parameters.list[ parameter.listPos() ].value();
			break;
		}
		
		case Parameter::Animated:
		{
			// Don't adjust the current function parameter
			return;
		}
	}
	
	m_function->setParameter( k );
}


void Plot::differentiate()
{
	switch ( plotMode )
	{
		case Function::Integral:
			plotMode = Function::Derivative0;
			break;

		case Function::Derivative0:
			plotMode = Function::Derivative1;
			break;

		case Function::Derivative1:
			plotMode = Function::Derivative2;
			break;

		case Function::Derivative2:
			kWarning() << k_funcinfo << "Can't handle this yet!\n";
			break;
	}
}


void Plot::integrate()
{
	switch ( plotMode )
	{
		case Function::Integral:
			kWarning() << k_funcinfo << "Can't handle this yet!\n";
			break;

		case Function::Derivative0:
			plotMode = Function::Integral;
			break;

		case Function::Derivative1:
			plotMode = Function::Derivative0;
			break;

		case Function::Derivative2:
			plotMode = Function::Derivative1;
			break;
	}
}


QColor Plot::color( ) const
{
	Function * f = function();
	assert(f); // Shouldn't call color without a function
	PlotAppearance appearance = f->plotAppearance( plotMode );
	
	if ( (plotNumberCount <= 1) || !appearance.useGradient )
		return appearance.color;
	
	// Is a gradient
	
	double x = plotNumber;
	double y = plotNumberCount - plotNumber - 1;
	
	double r = ((appearance.color1.redF() * x ) + (appearance.color2.redF() * y)) / (x+y);
	double g = ((appearance.color1.greenF() * x ) + (appearance.color2.greenF() * y)) / (x+y);
	double b = ((appearance.color1.blueF() * x ) + (appearance.color2.blueF() * y)) / (x+y);
	
	QColor color;
	color.setRedF( r );
	color.setGreenF( g );
	color.setBlueF( b );
	
	return color;
}
//END class Plot
