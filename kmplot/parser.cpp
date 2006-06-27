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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

// standard c(++) includes
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//KDE includes
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>

// local includes
#include "parser.h"
#include "settings.h"
#include "xparser.h"
//Added by qt3to4:
#include <QList>

#include <assert.h>

#include "parseradaptor.h"

double Parser::m_radiansPerAngleUnit = 0;

/// List of predefined functions.
Parser::Mfkt Parser::mfkttab[ FANZ ]=
{
	{"tanh", ltanh},	// Tangens hyperbolicus
	{"tan", ltan}, 		// Tangens
	{"sqrt", sqrt},		// Square root
	{"sqr", sqr}, 		// Square
	{"sinh", lsinh}, 	// Sinus hyperbolicus
	{"sin", lsin}, 		// Sinus
	{"sign", sign},		// Signum
	{"H", heaviside},	// Heaviside step function
	{"sech", sech},		// Secans hyperbolicus
	{"sec", sec},		// Secans
	{"log", llog}, 	        // Logarithm base 10
	{"ln", ln}, 		// Logarithm base e
	{"exp", exp}, 		// Exponential function base e
	{"coth", coth},		// Co-Tangens hyperbolicus
	{"cot", cot},		// Co-Tangens = 1/tan
	{"cosh", lcosh}, 	// Cosinus hyperbolicus
	{"cosech", cosech},	// Co-Secans hyperbolicus
	{"cosec", cosec},	// Co-Secans
	{"cos", lcos}, 		// Cosinus
	{"artanh", artanh}, 	// Area-tangens hyperbolicus = inverse of tanh
	{"arsinh", arsinh}, 	// Area-sinus hyperbolicus = inverse of sinh
	{"arsech", arsech},		// Area-secans hyperbolicus = invers of sech
	{"arctanh", artanh},	// The same as artanh
	{"arcsinh", arsinh},	// The same as arsinh
	{"arccosh", arcosh},	// The same as arcosh
	{"arctan", arctan},		// Arcus tangens = inverse of tan
	{"arcsin", arcsin}, 	// Arcus sinus = inverse of sin
	{"arcsec", arcsec},		// Arcus secans = inverse of sec
	{"arcoth", arcoth},		// Area-co-tangens hyperbolicus = inverse of coth
	{"arcosh", arcosh}, 	// Area-cosinus hyperbolicus = inverse of cosh
	{"arcosech", arcosech},	// Area-co-secans hyperbolicus = inverse of cosech
	{"arccot", arccot},		// Arcus co-tangens = inverse of cotan
	{"arccosec", arccosec},	// Arcus co-secans = inverse of cosec
	{"arccos", arccos}, 	// Arcus cosinus = inverse of cos
	{"abs", fabs},			// Absolute value
	{"floor", floor},		// round down to nearest integer
	{"ceil", ceil},			// round up to nearest integer
	{"round", round},		// round to nearest integer
	{"P_0", legendre0},		// lengedre polynomial (n=0)
	{"P_1", legendre1},		// lengedre polynomial (n=1)
	{"P_2", legendre2},		// lengedre polynomial (n=2)
	{"P_3", legendre3},		// lengedre polynomial (n=3)
	{"P_4", legendre4},		// lengedre polynomial (n=4)
	{"P_5", legendre5},		// lengedre polynomial (n=5)
	{"P_6", legendre6},		// lengedre polynomial (n=6)
};



//BEGIN class Parser
Parser::Parser()
	: m_sanitizer( this )
{
	m_evalPos = 0;
	evalflg = 0;
	m_nextFunctionID = 0;
	m_constants = new Constants( this );
	
	m_ownEquation = new Equation( Equation::Cartesian, 0 );
	m_currentEquation = m_ownEquation;

    new ParserAdaptor(this);
    QDBus::sessionBus().registerObject("/parser", this);
}


Parser::~Parser()
{
	kDebug() << "Exiting......" << endl;
	foreach ( Function * function, m_ufkt )
		delete function;
	delete m_ownEquation;
	
	delete m_constants;
}

void Parser::setAngleMode( AngleMode mode )
{
	switch ( mode )
	{
		case Radians:
			m_radiansPerAngleUnit = 1.0;
			break;
			
		case Degrees:
			m_radiansPerAngleUnit = M_PI/180;	
			break;
	}
}


double Parser::radiansPerAngleUnit()
{
        return m_radiansPerAngleUnit;
}


uint Parser::getNewId()
{
	uint i = m_nextFunctionID;
	while (1)
	{
		if ( !m_ufkt.contains( i ) )
		{
			m_nextFunctionID = i+1;
			return i;
		}
		++i;
	}
}

double Parser::eval( QString str, unsigned evalPosOffset, bool fixExpression )
{
// 	kDebug() << k_funcinfo << "str=\""<<str<<"\"\n";
	m_currentEquation = m_ownEquation;
	m_currentEquation->setFstr( str, true );
	
	stack=new double [STACKSIZE];
	stkptr=stack;
	evalflg=1;
	
	if ( fixExpression )
		m_sanitizer.fixExpression( & str, evalPosOffset );
	
	for ( int i = evalPosOffset; i < str.length(); i++ )
	{
		if ( constants()->have( str[i] ) )
		{
			m_error = UserDefinedConstantInExpression;
			m_errorPosition = m_sanitizer.realPos( i );
			delete []stack;
			return 0;
		}
	}
	
	m_eval = str;
	m_evalPos = evalPosOffset;
	m_error = ParseSuccess;
	heir1();
	if( !evalRemaining().isEmpty() && m_error==ParseSuccess)
		m_error=SyntaxError;
	evalflg=0;
	double const erg=*stkptr;
	delete [] stack;
	if ( m_error == ParseSuccess )
	{
		m_errorPosition = -1;
		return erg;
	}
	else
	{
		m_errorPosition = m_sanitizer.realPos( m_evalPos );
		return 0.;
	}
}


double Parser::fkt(uint const id, uint eq, double x )
{
	if ( !m_ufkt.contains( id ) || (eq >= 2) )
	{
		m_error = NoSuchFunction;
		return 0;
	}
	
	return fkt( m_ufkt[id]->eq[eq], x );
}


double Parser::fkt( Equation * eq, double x )
{
	Function * function = eq->parent();
	
	double var[3] = {0};
	
	switch ( function->type() )
	{
		case Function::Cartesian:
		case Function::Parametric:
		case Function::Polar:
		{
			var[0] = x;
			var[1] = function->k;
			break;
		}
			
		case Function::Implicit:
		{
			// Can only calculate when one of x, y is fixed
			assert( function->m_implicitMode != Function::UnfixedXY );
			if ( function->m_implicitMode == Function::FixedX )
			{
				var[0] = function->x;
				var[1] = x;
			}
			else
			{
				// fixed y
				var[0] = x;
				var[1] = function->y;
			}
			var[2] = function->k;
			break;
		}
	}
	
	return fkt( eq, var );
}


double Parser::fkt( Equation * eq, double x[3] )
{
	double *pDouble;
	double (**pFunction)(double);
	double *stack, *stkptr;
	uint *pUint;
	eq->mptr=eq->mem;
	stack=stkptr= new double [STACKSIZE];

	while(1)
	{
		switch(*eq->mptr++)
		{
			case KONST:
				pDouble=(double*)eq->mptr;
				*stkptr=*pDouble++;
				eq->mptr=(unsigned char*)pDouble;
				break;
				
			case VAR:
			{
				pUint = (uint*)eq->mptr;
				uint var = *pUint++;
				assert( var < 3 );
				*stkptr = x[var];
				eq->mptr = (unsigned char*)pUint;
				break;
			}
				
			case PUSH:
				++stkptr;
				break;
			case PLUS:
				stkptr[-1]+=*stkptr;
				--stkptr;
				break;
			case MINUS:
				stkptr[-1]-=*stkptr;
				--stkptr;
				break;
			case MULT:
				stkptr[-1]*=*stkptr;
				--stkptr;
				break;
			case DIV:
				if(*stkptr==0.)*(--stkptr)=HUGE_VAL;
				else
				{
					stkptr[-1]/=*stkptr;
					--stkptr;
				}
				break;
			case POW:
				stkptr[-1]=pow(*(stkptr-1), *stkptr);
				--stkptr;
				break;
			case NEG:
				*stkptr=-*stkptr;
				break;
			case SQRT:
				*stkptr = sqrt(*stkptr);
				break;
			case FKT:
				pFunction=(double(**)(double))eq->mptr;
				*stkptr=(*pFunction++)(*stkptr);
				eq->mptr=(unsigned char*)pFunction;
				break;
				
			case UFKT:
			{
				pUint=(uint*)eq->mptr;
				uint id = *pUint++;
				uint id_eq = *pUint++;
				
				// The number of arguments being passed to the function
				int args = *pUint++;
				assert( 1 <= args && args <= 3 );
				
// 				kDebug() << "Got function! id="<<id<<" id_eq="<<id_eq<<" args="<<args<<endl;
				
				if ( m_ufkt.contains( id ) )
				{
					double vars[3] = {0};
					for ( int i=0; i<args; ++i )
						vars[i] = *(stkptr-args+1+i);
					
					stkptr[1-args] = fkt( m_ufkt[id]->eq[id_eq], vars );
					stkptr -= args-1;
				}
				
				eq->mptr=(unsigned char*)pUint;
				break;
			}
			
			case ENDE:
				double const erg=*stkptr;
				delete [] stack;
				return erg;
		}
	}
}


int Parser::addFunction( QString str1, QString str2, Function::Type type )
{
// 	kDebug() << k_funcinfo << "str1="<<str1<<" str2="<<str2<<endl;
	
	QString str[2] = { str1, str2 };
	
	Function * temp = new Function( type );
	
	for ( unsigned i = 0; i < 2; ++i )
	{
		if ( str[i].isEmpty() || !temp->eq[i] )
			continue;
		
		if ( !temp->eq[i]->setFstr( str[i] ) )
		{
			kDebug() << "could not set fstr to \""<<str[i]<<"\"! error:"<<errorString()<<"\n";
			delete temp;
			return -1;
		}
	
		if ( fnameToID( temp->eq[i]->name() ) != -1 )
		{
			kDebug() << "function name reused.\n";
			m_error = FunctionNameReused;
			delete temp;
			return -1;
		}
	}
	
	temp->id = getNewId();
	m_ufkt[ temp->id ] = temp;
	
	emit functionAdded( temp->id );
	kDebug() << k_funcinfo << "all ok\n";
	return temp->id; //return the unique ID-number for the function
}


bool Parser::isFstrValid( QString str )
{
	stkptr = stack = 0;
	m_error = ParseSuccess;
	m_errorPosition = 0;

	const int p1=str.indexOf('(');
	int p2=str.indexOf(',');
	const int p3=str.indexOf(")=");
	
	m_sanitizer.fixExpression( & str, p1+4 );
        
	if(p1==-1 || p3==-1 || p1>p3)
	{
		/// \todo find the position of the capital and set into m_errorPosition
		m_error = InvalidFunctionVariable;
		kDebug() << k_funcinfo << "InvalidFunctionVariable: p1="<<p1<<" p2="<<p2<<" p3="<<p3<<" str="<<str<<endl;
		return false;
	}
	if ( p3+2 == str.length()) //empty function
	{
		/// \todo find the position of the capital and set into m_errorPosition
		m_error = EmptyFunction;
		return false;
	}
	if(p2==-1 || p2>p3)
		p2=p3;
	
	if (str.mid(p1+1, p2-p1-1) == "e")
	{
		/// \todo find the position of the capital and set into m_errorPosition
		m_error = InvalidFunctionVariable;
		return false;
	}
	
	QString fname = str.left(p1);
	
	if ( fname != fname.toLower() ) //isn't allowed to contain capital letters
	{
		m_error = CapitalInFunctionName;
		/// \todo find the position of the capital and set into m_errorPosition
		return false;
	}
	
// 	m_currentEquation = m_ownEquation;
// 	m_currentEquation->setFstr( str, true );
	(double) eval( str, p3+2, false );
	return (m_error == ParseSuccess);
}


void Parser::initEquation( Equation * eq )
{
	m_error = ParseSuccess;
	m_currentEquation = eq;
	mem = mptr = eq->mem;
	
	m_eval = eq->fstr();
	m_sanitizer.fixExpression( & m_eval, m_eval.indexOf('(')+4 );
	m_evalPos = m_eval.indexOf( '=' ) + 1;
	heir1();
	if ( !evalRemaining().isEmpty() && m_error == ParseSuccess )
		m_error = SyntaxError;
	addtoken(ENDE);
	m_errorPosition = -1;
}


bool Parser::removeFunction( Function * item )
{
	kDebug() << "Deleting id:" << item->id << endl;
	
	/// \todo FIX dependencies (also, test code with implicit function "x^2=y^2")
#if 0
	if (!item->dep.isEmpty())
	{
		KMessageBox::sorry(0,i18n("This function is depending on an other function"));
		return false;
	}
	
	kDebug() << "Looking for dependencies....\n";
	foreach ( Function * it1, m_ufkt )
	{
		if (it1==item)
			continue;
		for(QList<int>::iterator it2=it1->dep.begin(); it2!=it1->dep.end(); ++it2)
			if ( (uint)*it2 == item->id )
				it2 = it1->dep.erase(it2);
	}
#endif
	
	uint const id = item->id;
	
// 	kDebug() << "Removing from internal lists...\n";
	m_ufkt.remove(id);
	
// 	kDebug() << "Checking equations\n";
	for ( unsigned i = 0; i < 2; ++i )
	{
		if ( item->eq[i] && (item->eq[i] == m_currentEquation) )
			m_currentEquation = m_ownEquation;
	}
	
// 	kDebug() << "Actually deleting the function\n";
	delete item;
	
	emit functionRemoved( id );
	return true;
}

bool Parser::removeFunction(uint id)
{
	return m_ufkt.contains( id ) && removeFunction( m_ufkt[id] );
}

uint Parser::countFunctions()
{
	return m_ufkt.count();
}

void Parser::heir1()
{
	QChar c;
	heir2();
	if(m_error!=ParseSuccess)
		return ;

	while(1)
	{
		if ( m_eval.length() <= m_evalPos )
			return;
		
		c = m_eval[m_evalPos];
		switch ( c.unicode() )
		{
			default:
				return ;

			case ' ':
				++m_evalPos;
				continue;
			case '+':
			case '-':
				++m_evalPos;
				addtoken(PUSH);
				heir2();
				if(m_error!=ParseSuccess)
					return;
		}
		switch ( c.unicode() )
		{
			case '+':
				addtoken(PLUS);
				break;
			case '-':
				addtoken(MINUS);
		}
	}
}


void Parser::heir2()
{
	if ( match("-") )
	{
		heir2();
		if(m_error!=ParseSuccess)
			return;
		addtoken(NEG);
	}
	else if ( match( QChar(0x221a) ) ) // square root symbol
	{
		heir2();
		if(m_error!=ParseSuccess)
			return;
		addtoken(SQRT);
	}
	else
		heir3();
}


void Parser::heir3()
{
	QChar c;
	heir4();
	if(m_error!=ParseSuccess)
		return;
	while(1)
	{
		if ( m_eval.length() <= m_evalPos )
			return;
		
		c = m_eval[m_evalPos];
		switch ( c.unicode() )
		{
			default:
				return;
			case ' ':
				++m_evalPos;
				continue;
			case '*':
			case '/':
				++m_evalPos;
				addtoken(PUSH);
				heir4();
				if(m_error!=ParseSuccess)
					return ;
		}
		switch ( c.unicode() )
		{
			case '*':
				addtoken(MULT);
				break;
			case '/':
				addtoken(DIV);
		}
	}
}


void Parser::heir4()
{
	primary();
	if(m_error!=ParseSuccess)
		return;
	while(match("^"))
	{
		addtoken(PUSH);
		primary();
		if(m_error!=ParseSuccess)
			return;
		addtoken(POW);
	}
}


void Parser::primary()
{
	if ( match("(") || match(",") )
	{
		heir1();
		if ( !match(")") && !match(",") )
			m_error=MissingBracket;
		return;
	}
	int i;
	for(i=0; i<FANZ; ++i)
	{
		if(match(mfkttab[i].mfstr))
		{
			primary();
			addtoken(FKT);
			addfptr(mfkttab[i].mfadr);
			return;
		}
	}
	foreach ( Function * it, m_ufkt )
	{
		if ( evalRemaining() == "pi" ||
				   evalRemaining() == "e" ||
				   evalRemaining() == QChar(0x221E) )
			continue;

		for ( unsigned i = 0; i < 2; ++i )
		{
			if ( it->eq[i] && match(it->eq[i]->name()) )
			{
				if (it->eq[i] == m_currentEquation)
				{
					m_error=RecursiveFunctionCall;
					return;
				}
				
				int argCount = 0;
				bool argLeft = true;
				do
				{
					argCount++;
					primary();
					
					argLeft = m_eval.at(m_evalPos-1) == ',';
					if (argLeft)
					{
						addtoken(PUSH);
						m_evalPos--;
					}
				}
				while ( m_error == ParseSuccess && argLeft && !evalRemaining().isEmpty() );
				
				addtoken(UFKT);
				addfptr( it->id, i, argCount );
				if ( m_currentEquation->parent() )
					it->dep.append(m_currentEquation->parent()->id);
				return;
			}
		}
	}
        
	// A constant
	if ( (m_eval.length()>m_evalPos) && constants()->have( m_eval[m_evalPos] ) )
	{
		QVector<Constant> constants = m_constants->all();
		foreach ( Constant c, constants )
		{
			QChar tmp = c.constant;
			if ( match( tmp ) )
			{
				addtoken(KONST);
				addwert(c.value);
				return;
			}
		}
		m_error = NoSuchConstant;
		return;
	}
	
	if ( match("pi") || match( QChar(960) ) )
	{
		addtoken(KONST);
		addwert(M_PI);
		return;
	}
	
	if(match("e"))
	{
		addtoken(KONST);
		addwert(M_E);
		return;
	}
	
	if( match( QChar(0x221E) ) )
	{
		addtoken(KONST);
		addwert( INFINITY );
		return;
	}
	
	QStringList variables = m_currentEquation->parameters();
	uint at = 0;
	foreach ( QString var, variables )
	{
		if ( match( var ) )
		{
			addtoken( VAR );
			adduint( at );
			return;
		}
		at++;
	}

	QByteArray remaining = evalRemaining().toLatin1();
	char * lptr = remaining.data();
	char * p = 0;
	double const w = strtod(lptr, &p);
	if( lptr != p )
	{
		m_evalPos += p-lptr;
		addtoken(KONST);
		addwert(w);
	}
	else
		m_error = SyntaxError;
}


bool Parser::match( const QString & lit )
{
	if ( lit.isEmpty() )
		return false;
	
	while( (m_eval.length() > m_evalPos) && (m_eval[m_evalPos] == ' ') )
		++m_evalPos;
	
	if ( lit != evalRemaining().left( lit.length() ) )
		return false;
	
	m_evalPos += lit.length();
	return true;
}


void Parser::addtoken( Token token )
{
	if(stkptr>=stack+STACKSIZE-1)
	{
		m_error = StackOverflow;
		return;
	}

	if(evalflg==0)
	{
		if(mptr>=&mem[MEMSIZE-10])
			m_error = MemoryOverflow;
		else
			*mptr++=token;
        
		switch(token)
		{
			case PUSH:
				++stkptr;
				break;
			case PLUS:
			case MINUS:
			case MULT:
			case DIV:
			case POW:
				--stkptr;
				break;
			default:
				break;
		}
	}
	else switch(token)
	{
		case PUSH:
			++stkptr;
			break;
		case PLUS:
			stkptr[-1]+=*stkptr;
			--stkptr;
			break;

		case MINUS:
			stkptr[-1]-=*stkptr;
			--stkptr;
			break;
		case MULT:
			stkptr[-1]*=*stkptr;
			--stkptr;
			break;
		case DIV:
			if(*stkptr==0.)
				*(--stkptr)=HUGE_VAL;
			else
			{
				stkptr[-1]/=*stkptr;
				--stkptr;
			}
			break;
                
		case POW:
			stkptr[-1]=pow(*(stkptr-1), *stkptr);
			--stkptr;
			break;
		case NEG:
			*stkptr=-*stkptr;
			break;
		case SQRT:
			*stkptr = sqrt(*stkptr);
			break;
		default:
			break;
	}
}


void Parser::addwert(double x)
{
	double *pd=(double*)mptr;

	if(evalflg==0)
	{
		if(mptr>=&mem[MEMSIZE-10])
			m_error = MemoryOverflow;
		else
		{
			*pd++=x;
			mptr=(unsigned char*)pd;
		}
	}
	else
		*stkptr=x;
}


void Parser::adduint(uint x)
{
	uint *p=(uint*)mptr;

	if(evalflg==0)
	{
		if(mptr>=&mem[MEMSIZE-10])
			m_error = MemoryOverflow;
		else
		{
			*p++=x;
			mptr=(unsigned char*)p;
		}
	}
}


void Parser::addfptr(double(*fadr)(double))
{
        double (**pf)(double)=(double(**)(double))mptr;
        if( evalflg==0 )
        {
        if( mptr>=&mem[MEMSIZE-10] )
			m_error = MemoryOverflow;
        else
                {
                        *pf++=fadr;
                        mptr=(unsigned char*)pf;
                }
        }
        else
		{
// 			kDebug() << k_funcinfo << "*stkptr="<<*stkptr<<endl;
			*stkptr=(*fadr)(*stkptr);
		}
}


void Parser::addfptr( uint id, uint eq_id, uint args )
{
	uint *p=(uint*)mptr;
	if(evalflg==0)
	{
		if(mptr>=&mem[MEMSIZE-10])
			m_error=MemoryOverflow;
		else
		{
			*p++=id;
			*p++=eq_id;
			*p++=args;
			mptr=(unsigned char*)p;
		}
	}
	else
	{
		Function * function = functionWithID( id );
		if ( function )
		{
			/// \todo take into account args
			*stkptr = fkt( function->eq[eq_id], *stkptr );
		}
	}
}


int Parser::fnameToID(const QString &name)
{
	foreach ( Function * it, m_ufkt )
	{
		for ( unsigned i = 0; i < 2; ++i )
		{
			if ( it->eq[i] && (name == it->eq[i]->name()) )
				return it->id;
		}
	}
	return -1;     // Name not found
}


QString Parser::errorString() const
{
	switch(m_error)
	{
		case ParseSuccess:
			return QString();
			
		case SyntaxError:
			return i18n("Parser error at position %1:\n"
					"Syntax error", m_errorPosition+1);
			
		case MissingBracket:
			return i18n("Parser error at position %1:\n"
					"Missing parenthesis", m_errorPosition+1);
			
		case UnknownFunction:
			return i18n("Parser error at position %1:\n"
					"Function name unknown", m_errorPosition+1);
			
		case InvalidFunctionVariable:
			return i18n("Parser error at position %1:\n"
					"Void function variable", m_errorPosition+1);
			
		case TooManyFunctions:
			return i18n("Parser error at position %1:\n"
					"Too many functions", m_errorPosition+1);
			
		case MemoryOverflow:
			return i18n("Parser error at position %1:\n"
					"Token-memory overflow", m_errorPosition+1);
			
		case StackOverflow:
			return i18n("Parser error at position %1:\n"
					"Stack overflow", m_errorPosition+1);
			
		case FunctionNameReused:
			return i18n("Parser error at position %1:\n"
					"Name of function not free.", m_errorPosition+1);
			
		case RecursiveFunctionCall:
			return i18n("Parser error at position %1:\n"
					"recursive function not allowed.", m_errorPosition+1);
			
		case NoSuchConstant:
			return i18n("Could not find a defined constant at position %1.", m_errorPosition+1);
			
		case EmptyFunction:
			return i18n("Empty function");
			
		case CapitalInFunctionName:
			return i18n("The function name is not allowed to contain capital letters.");
			
		case NoSuchFunction:
			return i18n("Function could not be found.");
			
		case UserDefinedConstantInExpression:
			return i18n("The expression must not contain user-defined constants.");
	}
	
	return QString();
}


Parser::Error Parser::parserError(bool showMessageBox)
{
	if (!showMessageBox)
		return m_error;
	
	QString message( errorString() );
	if ( !message.isEmpty() )
		KMessageBox::sorry(0, message, "KmPlot");
	return m_error;
}


QString Parser::evalRemaining() const
{
	QString current( m_eval );
	return current.right( qMax( 0, current.length() - m_evalPos ) );
}


Function * Parser::functionWithID( int id ) const
{
	return m_ufkt.contains( id ) ? m_ufkt[id] : 0;
}


// static
QString Parser::number( double value )
{
	QString str = QString::number( value, 'g', 6 );
	str.replace( 'e', "*10^" );
// 	kDebug() << "returning str="<<str<<endl;
	return str;
}
//END class Parser



double ln(double x)
{
        return log(x);
}

double llog(double x)
{
        return log10(x);
}

double sign(double x)
{
        if(x<0.)
                return -1.;
        else
                if(x>0.)
                        return 1.;
        return 0.;
}

double heaviside( double x )
{
	if ( x < 0.0 )
		return 0.0;
	else if ( x > 0.0 )
		return 1.0;
	else
		return 0.5;
}

double sqr(double x)
{
        return x*x;
}

double arsinh(double x)
{
        return log(x+sqrt(x*x+1));
}


double arcosh(double x)
{
        return log(x+sqrt(x*x-1));
}


double artanh(double x)
{
        return log((1+x)/(1-x))/2;
}

// sec, cosec, cot and their inverses

double sec(double x)
{
        return (1 / cos(x*Parser::radiansPerAngleUnit()));
}

double cosec(double x)
{
        return (1 / sin(x*Parser::radiansPerAngleUnit()));
}

double cot(double x)
{
        return (1 / tan(x*Parser::radiansPerAngleUnit()));
}

double arcsec(double x)
{
        if ( !Parser::radiansPerAngleUnit() )
                return ( 1/acos(x)* 180/M_PI );
        else
                return acos(1/x);
}

double arccosec(double x)
{
        return asin(1/x)* 1/Parser::radiansPerAngleUnit();
}

double arccot(double x)
{
        return atan(1/x)* 1/Parser::radiansPerAngleUnit();
}

// sech, cosech, coth and their inverses


double sech(double x)
{
        return (1 / cosh(x*Parser::radiansPerAngleUnit()));
}

double cosech(double x)
{
        return (1 / sinh(x*Parser::radiansPerAngleUnit()));
}

double coth(double x)
{
        return (1 / tanh(x*Parser::radiansPerAngleUnit()));
}

double arsech(double x)
{
        return arcosh(1/x)* 1/Parser::radiansPerAngleUnit();
}

double arcosech(double x)
{
        return arsinh(1/x)* 1/Parser::radiansPerAngleUnit();
}

double arcoth(double x)
{   return artanh(1/x)* 1/Parser::radiansPerAngleUnit();
}

//basic trigonometry functions

double lcos(double x)
{
        return cos(x*Parser::radiansPerAngleUnit());
}
double lsin(double x)
{
        return sin(x*Parser::radiansPerAngleUnit());
}
double ltan(double x)
{
        return tan(x*Parser::radiansPerAngleUnit());
}

double lcosh(double x)
{
        return cosh(x*Parser::radiansPerAngleUnit());
}
double lsinh(double x)
{
        return sinh(x*Parser::radiansPerAngleUnit());
}
double ltanh(double x)
{
        return tanh(x*Parser::radiansPerAngleUnit());
}

double arccos(double x)
{
        return acos(x) * 1/Parser::radiansPerAngleUnit();
}
double arcsin(double x)
{
        return asin(x)* 1/Parser::radiansPerAngleUnit();
}

double arctan(double x)
{
        return atan(x)* 1/Parser::radiansPerAngleUnit();
}

double legendre0( double )
{
	return 1.0;
}

double legendre1( double x )
{
	return x;
}

double legendre2( double x )
{
	return (3*x*x-1)/2;
}

double legendre3( double x )
{
	return (5*x*x*x - 3*x)/2;
}

double legendre4( double x )
{
	return (35*x*x*x*x - 30*x*x +3)/8;
}

double legendre5( double x )
{
	return (63*x*x*x*x*x - 70*x*x*x + 15*x)/8;
}

double legendre6( double x )
{
	return (231*x*x*x*x*x*x - 315*x*x*x*x + 105*x*x - 5)/16;
}


//BEGIN class Constants
Constants::Constants( Parser * parser )
{
	m_parser = parser;
}


QVector< Constant >::iterator Constants::find( QChar name )
{
	QVector<Constant>::Iterator it;
	for ( it = m_constants.begin(); it != m_constants.end(); ++it )
	{
		if ( it->constant == name )
			break;
	}
	return it;
}


bool Constants::have( QChar name ) const
{
	for ( QVector<Constant>::ConstIterator it = m_constants.begin(); it != m_constants.end(); ++it )
	{
		if ( it->constant == name )
			return true;
	}
	return false;
}


void Constants::remove( QChar name )
{
// 	kDebug() << k_funcinfo << "removing " << name << endl;
	
	QVector<Constant>::iterator c = find( name );
	if ( c != m_constants.end() )
		m_constants.erase( c );
}


void Constants::add( Constant c )
{
// 	kDebug() << k_funcinfo << "adding " << c.constant << endl;
	
	remove( c.constant );
	m_constants.append( c );
}


bool Constants::isValidName( QChar name )
{
	// special cases: disallow heaviside step function, pi symbol
	if ( name == 'H' || name == QChar(960) )
		return false;
	
	switch ( name.category() )
	{
		case QChar::Letter_Uppercase:
// 		case QChar::Symbol_Math:
			return true;
			
		case QChar::Letter_Lowercase:
			// don't allow lower case letters of the Roman alphabet
			return ( (name.unicode() < 'a') || (name.unicode() > 'z') );
			
		default:
			return false;
	}
}


QChar Constants::generateUniqueName()
{
	for ( char c = 'A'; c <= 'Z'; ++c )
	{
		if ( !have( c ) )
			return c;
	}
	
	kWarning() << k_funcinfo << "Could not find a unique constant.\n";
	return 'C';
}


void Constants::load()
{
// 	KSimpleConfig conf ("kcalcrc");
// 	conf.setGroup("UserConstants");
// 	QString tmp;
// 	
// 	for( int i=0; ;i++)
// 	{
// 		tmp.setNum(i);
// 		QString tmp_constant = conf.readEntry("nameConstant"+tmp, QString(" "));
// 		QString tmp_value = conf.readEntry("valueConstant"+tmp, QString(" "));
// // 		kDebug() << "konstant: " << tmp_constant << endl;
// // 		kDebug() << "value: " << tmp_value << endl;
// // 		kDebug() << "**************" << endl;
// 		
// 		if ( tmp_constant == " " )
// 			return;
// 		
// 		if ( tmp_constant.isEmpty() )
// 			continue;
// 			
// 		double value = m_parser->eval(tmp_value);
// 		if ( m_parser->parserError(false) )
// 		{
// 			kWarning() << k_funcinfo << "Couldn't parse the value " << tmp_value << endl;
// 			continue;
// 		}
// 		
// 		QChar constant = tmp_constant[0].toUpper();
// 		
// 		if ( !isValidName( constant ) || have( constant ) )
// 			constant = generateUniqueName();
// 		
// 		add( Constant(constant, value) );
// 	}
}

void Constants::save()
{
// 	KSimpleConfig conf ("kcalcrc");
// 	conf.deleteGroup("Constants");
// 	
// 	// remove any previously saved constants
// 	conf.deleteGroup( "UserConstants", KConfigBase::Recursive );
// 	conf.deleteGroup( "UserConstants", 0 ); /// \todo remove this line when fix bug in kconfigbase
// 	
// 	
// 	conf.setGroup("UserConstants");
// 	QString tmp;
// 	
// 	int i = 0;
// 	foreach ( Constant c, m_constants )
// 	{
// 		tmp.setNum(i);
// 		conf.writeEntry("nameConstant"+tmp, QString( c.constant ) ) ;
// 		conf.writeEntry("valueConstant"+tmp, c.value);
// // 		kDebug() << "wrote constant="<<c.constant<<" value="<<c.value<<endl;
// 		
// 		i++;
// 	}
}
//END class Constants


//BEGIN class ExpressionSanitizer
ExpressionSanitizer::ExpressionSanitizer( Parser * parser )
	: m_parser( parser )
{
	m_str = 0l;
	m_decimalSymbol = KGlobal::locale()->decimalSymbol();
}


void ExpressionSanitizer::fixExpression( QString * str, int pos )
{
// 	kDebug() << k_funcinfo << "pos="<<pos<<" str="<<*str<<endl;
	
	m_str = str;
	
	m_map.resize( m_str->length() );
	for ( int i = 0; i < m_str->length(); ++i )
		m_map[i] = i;
	
	// hack for implicit functions: change e.g. "y = x + 2" to "y - (x+2)" so
	// that they can be evaluated via equality with zero.
	if ( str->count( '=' ) > 1 )
	{
		int equalsPos = str->lastIndexOf( '=' );
		replace( equalsPos, 1, "-(" );
		append( ')' );
	}
	
	stripWhiteSpace();
	
	m_map.insert( 0, 0 );
	m_map.insert( m_map.size(), m_map[ m_map.size()-1 ] );
	*str = ' ' + *str + ' ';
	
	// make sure all minus-like signs (including the actual unicode minus sign)
	// are represented by a dash (unicode 0x002d)
	QChar dashes[6] = { 0x2012, 0x2013, 0x2014, 0x2015, 0x2053, 0x2212 };
	for ( unsigned i = 0; i < 6; ++i )
		replace( dashes[i], '-' );
	
	// replace the proper unicode divide sign by the forward-slash
	replace( QChar( 0x2215 ), '/' );
	
	// replace the unicode middle-dot for multiplication by the star symbol
	replace( QChar( 0x2219 ), '*' );
	
	// various power symbols
	replace( QChar(0x00B2), "^2" );
	replace( QChar(0x00B3), "^3" );
	replace( QChar(0x2070), "^0" );
	replace( QChar(0x2074), "^4" );
	replace( QChar(0x2075), "^5" );
	replace( QChar(0x2076), "^6" );
	replace( QChar(0x2077), "^7" );
	replace( QChar(0x2078), "^8" );
	replace( QChar(0x2079), "^9" );
	
	// fractions
	replace( QChar(0x00BC), "(1/4)" );
	replace( QChar(0x00BD), "(1/2)" );
	replace( QChar(0x00BE), "(3/4)" );
	replace( QChar(0x2153), "(1/3)" );
	replace( QChar(0x2154), "(2/3)" );
	replace( QChar(0x2155), "(1/5)" );
	replace( QChar(0x2156), "(2/5)" );
	replace( QChar(0x2157), "(3/5)" );
	replace( QChar(0x2158), "(4/5)" );
	replace( QChar(0x2159), "(1/6)" );
	replace( QChar(0x215a), "(5/6)" );
	replace( QChar(0x215b), "(1/8)" );
	replace( QChar(0x215c), "(3/8)" );
	replace( QChar(0x215d), "(5/8)" );
	replace( QChar(0x215e), "(7/8)" );
	
	//insert '*' when it is needed
	QChar ch;
	bool function = false;
	for(int i=pos+1; i+1 <  str->length();i++)
	{
		ch = str->at(i);
		
		bool chIsFunctionLetter = false;
		chIsFunctionLetter |= ch.category()==QChar::Letter_Lowercase;
		chIsFunctionLetter |= (ch == 'H');
		chIsFunctionLetter |= (ch == 'P') && (str->at(i+1) == QChar('_'));
		chIsFunctionLetter |= (ch == '_' );
		chIsFunctionLetter |= (ch.isNumber() && (str->at(i-1) == QChar('_')));
		
		if ( str->at(i+1)=='(' && chIsFunctionLetter )
		{
			// Work backwards to build up the full function name
			QString str_function(ch);
			int n=i-1;
			while (n>0 && ((str->at(n).category() == QChar::Letter_Lowercase) || (str->at(n) == QChar('_')) || (str->at(n) == QChar('P')) && (str->at(n+1) == QChar('_'))) )
			{
				str_function.prepend(str->at(n));
				--n;
			}
				
			for ( int func = 0; func < FANZ; ++func )
			{
				if ( str_function == QString( m_parser->mfkttab[func].mfstr ) )
				{
					function = true;
					break;
				}
			}
				
			if ( !function )
			{
				// Not a predefined function, so search through the user defined functions (e.g. f(x), etc)
				// to see if it is one of those
				foreach ( Function * it, m_parser->m_ufkt )
				{
					for ( int j=i; j>0 && (str->at(j).isLetter() || str->at(j).isNumber() ) ; --j)
					{
						for ( uint k=0; k<2; ++k )
						{
							if ( it->eq[k] && (it->eq[k]->name() == str->mid(j,i-j+1)) )
								function = true;
						}
					}
				}
			}
		}
		else  if (function)
			function = false;
		
		bool chIsNumeric = ((ch.isNumber() && (str->at(i-1) != QChar('_'))) || m_parser->m_constants->have( ch ));
				
		if ( chIsNumeric && ( str->at(i-1).isLetter() || str->at(i-1) == ')' ) || (ch.isLetter() && str->at(i-1)==')') )
		{
			insert(i,'*');
// 			kDebug() << "inserted * before\n";
		}
		else if( (chIsNumeric || ch == ')') && ( str->at(i+1).isLetter() || str->at(i+1) == '(' ) || (ch.isLetter() && str->at(i+1)=='(' && !function ) )
		{
			insert(i+1,'*');
//  			kDebug() << "inserted * after, function="<<function<<" ch="<<ch<<"\n";
			i++;
		}
	}
	
	stripWhiteSpace();
	
	QString str_end = str->mid(pos);
	str_end = str_end.replace(m_decimalSymbol, "."); //replace the locale decimal symbol with a '.'
	str->truncate(pos);
	str->append(str_end);
// 	kDebug() << "str:" << *str << endl;
}


void ExpressionSanitizer::stripWhiteSpace()
{
	int i = 0;
	
	while ( i < m_str->length() )
	{
		if ( m_str->at(i).isSpace() )
		{
			m_str->remove( i, 1 );
			m_map.remove( i, 1 );
		}
		else
			i++;
	}
}


void ExpressionSanitizer::remove( const QString & str )
{
// 	kDebug() << "Before:\n";
// 	displayMap();
	
	int at = 0;
	
	do
	{
		at = m_str->indexOf( str, at );
		if ( at != -1 )
		{
			m_map.remove( at, str.length() );
			m_str->remove( at, str.length() );
		}
	}
	while ( at != -1 );
	
// 	kDebug() << "After:\n";
// 	displayMap();
}


void ExpressionSanitizer::remove( const QChar & str )
{
	remove( QString(str) );
}


void ExpressionSanitizer::replace( QChar before, QChar after )
{
	m_str->replace( before, after );
}


void ExpressionSanitizer::replace( QChar before, const QString & after )
{
	if ( after.isEmpty() )
	{
		remove( before );
		return;
	}
	
// 	kDebug() << "Before:\n";
// 	displayMap();
	
	int at = 0;
	
	do
	{
		at = m_str->indexOf( before, at );
		if ( at != -1 )
		{
			int to = m_map[ at ];
			for ( int i = at + 1; i < at + after.length(); ++i )
				m_map.insert( i, to );
			
			m_str->replace( at, 1, after );
			at += after.length() - 1;
		}
	}
	while ( at != -1 );
	
// 	kDebug() << "After:\n";
// 	displayMap();
}


void ExpressionSanitizer::replace( int pos, int len, const QString & after )
{
// 	kDebug() << "Before:\n";
// 	displayMap();
	
	int before = m_map[pos];
	m_map.remove( pos, len );
	m_map.insert( pos, after.length(), before );
	m_str->replace( pos, len, after );
	
// 	kDebug() << "After:\n";
// 	displayMap();
}


void ExpressionSanitizer::insert( int i, QChar ch )
{
// 	kDebug() << "Before:\n";
// 	displayMap();
	
	m_map.insert( i, m_map[i] );
	m_str->insert( i, ch );
	
// 	kDebug() << "After:\n";
// 	displayMap();
}


void ExpressionSanitizer::append( QChar str )
{
// 	kDebug() << "Before:\n";
// 	displayMap();
	
	m_map.insert( m_map.size(), m_map[ m_map.size() - 1 ] );
	m_str->append( str );
	
// 	kDebug() << "After:\n";
// 	displayMap();
}


int ExpressionSanitizer::realPos( int evalPos )
{
	if ( m_map.isEmpty() || (evalPos < 0) )
		return -1;
	
	if ( evalPos >= m_map.size() )
	{
		kWarning() << k_funcinfo << "evalPos="<<evalPos<<" is out of range.\n";
// 		return m_map[ m_map.size() - 1 ];
		return -1;
	}
	
	return m_map[evalPos];
}


void ExpressionSanitizer::displayMap( )
{
	QString out('\n');
	
	for ( int i = 0; i < m_map.size(); ++i )
		out += QString("%1").arg( m_map[i], 3 );
	out += '\n';
	
	for ( int i = 0; i < m_str->length(); ++i )
		out += "  " + (*m_str)[i];
	out += '\n';
	
	kDebug() << out;
}
//END class ExpressionSanitizer

#include "parser.moc"
