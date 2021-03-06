/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 2006 David Saxton <david@bluehaze.org>
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
*/

#ifndef _PARAMETERSWIDGET_H
#define _PARAMETERSWIDGET_H

#include <QGroupBox>
#include <QList>

#include "ui_parameterswidget.h"
#include "function.h"

class EquationEdit;

class ParametersWidget : public QGroupBox, public Ui_ParametersWidget
{
	Q_OBJECT
	public:
		ParametersWidget( QWidget * parent );
		
		/**
		 * Initializes the contents of the widgets to the settings in
		 * \p function.
		 */
		void init( const ParameterSettings & parameters );
		/**
		 * \return the current settings as specified in the widgets.
		 */
		ParameterSettings parameterSettings() const;
		/**
		 * The ParametersWidget can make sure that when the user wants to use
		 * a parameter (i.e. the Use List checkbox or Use Slider checkbox is
		 * checked), the function string has a parameter variable. Use this
		 * to add an EquationEdit for a function string that ParametersWidget
		 * will update when necessary.
		 */
		void associateEquationEdit( EquationEdit * edit );
		
	signals:
		/**
		 * Emitted when the user edits the list of parameters.
		 */
		void parameterListChanged();
		
	private slots:
		/**
		 * Called when the "Edit [parameter] List" button is clicked.
		 */
		void editParameterList();
		/**
		 * Called when one of the checkboxes is checked.
		 */
		void updateEquationEdits();
		
	protected:
		/**
		 * If we are currently editing a cartesian function, this will be set
		 * to its parameter list.
		 */
		QList<Value> m_parameters;
		/**
		 * The list of equation edits that may be updated. See sassociateEquationEdit.
		 */
		QList<EquationEdit*> m_equationEdits;
};

#endif
