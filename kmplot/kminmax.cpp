/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 2004  Fredrik Edemar
*                     f_edemar@linux.se
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

#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>

#include "kminmax.h"
#include "xparser.h"

KMinMax::KMinMax(QWidget *parent, const char *name)
 : QMinMax(parent, name)
{
}


KMinMax::KMinMax(View *v, QWidget *parent, const char *name)
	: QMinMax(parent, name)
{
	m_view=v;
	m_mode=-1;
	connect( cmdClose, SIGNAL( clicked() ), this, SLOT( close() ));
	connect( cmdFind, SIGNAL( clicked() ), this, SLOT( cmdFind_clicked() ));
}


void KMinMax::init(char m)
{
	if ( m_mode==m)
	{
		updateFunctions();
		return;
	}
	
	m_mode = m;
	if ( m_mode < 2) //find minimum point
	{
		max->setReadOnly(false);
		min->setText("");
		max->setText("");
		cmdFind->setText("&Find");
		if ( m_mode == 1) //find maximum point
			setCaption(i18n("Find maximum point"));
	}
	else if ( m_mode == 2) //get y-value
	{
		setCaption(i18n("Get y-value"));
		max->setReadOnly(true);
		min->setText("");
		max->setText("");
		cmdFind->setText("&Find");
		lblMin->setText(i18n("X:"));
		lblMax->setText(i18n("Y:"));	
	}
	else if ( m_mode == 3) //area under a graph
	{
		setCaption(i18n("Area under a graph"));
		lblMin->setText(i18n("Draw the area between the x-values"));
		lblMax->setText(i18n("and"));
		min->setText("");
		max->setText("");
		cmdFind->setText("&Draw");
	}	

	QString range;
	range.setNum(View::xmin);
	min->setText( range);
	range.setNum(View::xmax);
	max->setText(range);
	
	updateFunctions();
}

void KMinMax::updateFunctions()
{
	list->clear();
	int index;
	QString fname, fstr;
	for ( index = 0; index < m_view->parser()->ufanz; ++index )
	{
		if ( m_view->parser()->getfkt( index, fname, fstr ) == -1 ) continue;
		if( fname[0] != 'x'  && fname[0] != 'y' && fname[0] != 'r')
		{
			if ( m_view->parser()->fktext[ index ].f_mode )
				list->insertItem(m_view->parser()->fktext[ index ].extstr);
			if ( m_view->parser()->fktext[ index ].f1_mode ) //1st derivative
			{
				QString function (m_view->parser()->fktext[ index ].extstr);
				int i= function.find('(');
				function.insert(i,'\'');
				list->insertItem(function );
			}
			if ( m_view->parser()->fktext[ index ].f2_mode )//2nd derivative
			{
				QString function (m_view->parser()->fktext[ index ].extstr);
				int i= function.find('(');
				function.insert(i,"\'\'");
				list->insertItem(function );
			}
			if ( m_view->parser()->fktext[ index ].anti_mode )//anti derivative
			{
				QString function (m_view->parser()->fktext[ index ].extstr);
				int i= function.find('(');
				QString tmp = function.left(i);
				tmp = tmp.upper();
				function.replace(0,i,tmp);
				list->insertItem(function );
			}
		}
	}
	selectItem();
}

void KMinMax::selectItem()
{
	if (  m_view->csmode < 0)
		return;
	kdDebug() << "cstype: " << (int)m_view->cstype << endl;
	QString function = m_view->parser()->fktext[ m_view->csmode ].extstr;
	if ( m_view->cstype == 2)
	{
		int i= function.find('(');
		function.remove(i-2,2);
	}
	else if ( m_view->cstype == 1)
	{
		int i= function.find('(');
		function.remove(i-1,1);
	}
	kdDebug() << "function: " << function << endl;
	QListBoxItem *item = list->findItem(function);
	list->setSelected(item,true);
}

KMinMax::~KMinMax()
{
}

void KMinMax::cmdFind_clicked()
{
	if ( list->currentItem() == -1)
	{
		KMessageBox::error(this, i18n("Please choose a function"));
		return;
	}
	double dmin, dmax;
	dmin = m_view->parser()->eval(min->text() );
	if ( m_view->parser()->errmsg()!=0 )
	{
		min->setFocus();
		min->selectAll();
		return;
	}
	if ( m_mode != 2)
	{
		dmax = m_view->parser()->eval(max->text() );
		if ( m_view->parser()->errmsg()!=0 )
		{
			max->setFocus();
			max->selectAll();
			return;
		}
	}
		
	int index;
	QString fname, fstr;
	QString function( list->currentText() );
	char p_mode = 0;
	if ( function.contains('\'') == 1)
	{
		p_mode = 1;
		int pos = function.find('\'');
		function.remove(pos,1);
	}
	else if ( function.contains('\'') == 2)
	{
		p_mode = 2;
		int pos = function.find('\'');
		function.remove(pos,2);
	}
	else if ( function.at(0).category() == QChar::Letter_Uppercase)
	{
		p_mode = 3;
		function.at(0) =  function.at(0).lower();
	}	
	
	bool stop=false;
	for ( index = 0; index < m_view->parser()->ufanz && !stop; ++index )
	{
		if ( m_view->parser()->getfkt( index, fname, fstr ) == -1 ) continue;
		if ( m_view->parser()->fktext[ index ].extstr == function)
			stop=true;
	}


	if ( m_mode == 0)
		m_view->findMinMaxValue(index-1,p_mode,true,dmin,dmax);
	else if ( m_mode == 1)
		m_view->findMinMaxValue(index-1,p_mode,false,dmin,dmax);
	else if ( m_mode == 2)
	{
		m_view->getYValue(index-1,p_mode,dmin,dmax);
		QString tmp;
		tmp.setNum(dmax);
		max->setText(tmp);
	}
	else if ( m_mode == 3)
	{
		m_view->areaUnderGraph(index-1,p_mode,dmin,dmax);
	}
	QDialog::accept();
}


#include "kminmax.moc"