/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 2004  Fredrik Edemar
*                     f_edemar@linux.se
*               2006  David Saxton <david@bluehaze.org>
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


#include <kdebug.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <klistbox.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <ktempfile.h>
#include <kurl.h>
#include <qfile.h>
#include <qtextstream.h>
#include <QList>
#include <QListWidget>

#include "kparametereditor.h"

class ParameterValueList;

KParameterEditor::KParameterEditor(XParser *m, QList<ParameterValueItem> *l, QWidget *parent )
	: KDialog( parent, i18n("Parameter Editor"), Ok ),
	  m_parameter(l),
	  m_parser(m)
{
	m_mainWidget = new QParameterEditor( this );
	setMainWidget( m_mainWidget );
	
	for (  QList<ParameterValueItem>::Iterator it = m_parameter->begin(); it != m_parameter->end(); ++it )
		m_mainWidget->list->addItem( (*it).expression );
	m_mainWidget->list->sortItems();
	
	connect( m_mainWidget->cmdNew, SIGNAL( clicked() ), this, SLOT( cmdNew_clicked() ));
	connect( m_mainWidget->cmdEdit, SIGNAL( clicked() ), this, SLOT( cmdEdit_clicked() ));
	connect( m_mainWidget->cmdDelete, SIGNAL( clicked() ), this, SLOT( cmdDelete_clicked() ));
	connect( m_mainWidget->cmdImport, SIGNAL( clicked() ), this, SLOT( cmdImport_clicked() ));
	connect( m_mainWidget->cmdExport, SIGNAL( clicked() ), this, SLOT( cmdExport_clicked() ));
	connect( m_mainWidget->list, SIGNAL( itemDoubleClicked( QListWidgetItem * ) ), this, SLOT( varlist_doubleClicked( QListWidgetItem *) ));
	connect( m_mainWidget->list, SIGNAL( itemClicked ( QListWidgetItem * ) ), this, SLOT( varlist_clicked(QListWidgetItem *  ) ));
	
}

KParameterEditor::~KParameterEditor()
{
}

void KParameterEditor::accept()
{
	kDebug() << "saving\n";
	m_parameter->clear();
	QString item_text;
	for ( int i = 0; i < m_mainWidget->list->count(); i++ )
	{
		item_text = m_mainWidget->list->item(i)->text();
		if ( !item_text.isEmpty() )
			m_parameter->append( ParameterValueItem(item_text, m_parser->eval( item_text)) );
	}
	
	KDialog::accept();
}

void KParameterEditor::cmdNew_clicked()
{
	QString result="";
	while (1)
	{
		bool ok;
		result = KInputDialog::getText( i18n("Parameter Value"), i18n( "Enter a new parameter value:" ), result, &ok );
		if ( !ok)
			return;
		m_parser->eval( result );
		if ( m_parser->parserError(false) != 0 )
		{
			m_parser->parserError( true );
			continue;
		}
		if ( checkTwoOfIt(result) )
		{
			KMessageBox::sorry(0,i18n("The value %1 already exists and will therefore not be added.").arg(result));
			continue;
		}
		m_mainWidget->list->addItem(result);
		m_mainWidget->list->sortItems();
		break;
	}
}

void KParameterEditor::cmdEdit_clicked()
{
	QListWidgetItem * currentItem = m_mainWidget->list->currentItem();
	QString result = currentItem ? currentItem->text() : QString::null;
	
	while (1)
	{
		bool ok;
		result = KInputDialog::getText( i18n("Parameter Value"), i18n( "Enter a new parameter value:" ), result, &ok );
		if ( !ok)
			return;
		m_parser->eval(result);
		if ( m_parser->parserError(false) != 0)
		{
			m_parser->parserError( true );
			continue;
		}
		if ( checkTwoOfIt(result) )
		{
			currentItem = m_mainWidget->list->currentItem();
			QString currentText = currentItem ? currentItem->text() : QString::null;
			
			if( result != currentText )
				KMessageBox::sorry(0,i18n("The value %1 already exists.").arg(result));
			continue;
		}
		m_mainWidget->list->takeItem( m_mainWidget->list->currentRow() );
		m_mainWidget->list->addItem(result);
		m_mainWidget->list->sortItems();
		break;
	}
}

void KParameterEditor::cmdDelete_clicked()
{
	delete m_mainWidget->list->takeItem( m_mainWidget->list->currentRow() );
	m_mainWidget->list->sortItems();
}

void KParameterEditor::cmdImport_clicked()
{
	KUrl url = KFileDialog::getOpenURL( QString(),i18n("*.txt|Plain Text File "));
	if ( url.isEmpty() )
		return;
        
        if (!KIO::NetAccess::exists(url,true,this) )
        {
			KMessageBox::sorry(0,i18n("The file does not exist."));
                return;
        }
        
	bool verbose = false;
        QFile file;
        QString tmpfile;
        if ( !url.isLocalFile() )
        {
                if ( !KIO::NetAccess::download(url, tmpfile, this) )
                {
					KMessageBox::sorry(0,i18n("An error appeared when opening this file"));
                        return;
                }
                file.setFileName(tmpfile);
        }
        else
                file.setFileName(url.prettyURL(0) );
	
	if ( file.open(QIODevice::ReadOnly) )
	{
		QTextStream stream(&file);
		QString line;
		for( int i=1; !stream.atEnd();i++ )
		{
			line = stream.readLine();
			if (line.isEmpty())
				continue;
			m_parser->eval( line );
			if ( m_parser->parserError(false) == 0)
			{
				if ( !checkTwoOfIt(line) )
				{
					m_mainWidget->list->addItem(line);
					m_mainWidget->list->sortItems();
				}
			}
			else if ( !verbose)
			{
				if ( KMessageBox::warningContinueCancel(this,i18n("Line %1 is not a valid parameter value and will therefore not be included. Do you want to continue?").arg(i) ) == KMessageBox::Cancel)
				{
					file.close();
                                        KIO::NetAccess::removeTempFile( tmpfile );
					return;
				}
				else if (KMessageBox::warningYesNo(this,i18n("Would you like to be informed about other lines that cannot be read?"), QString(), i18n("Get Informed"), i18n("Ignore Information") ) == KMessageBox::No)
					verbose = true;
			}
		}
		file.close();
	}
	else
		KMessageBox::sorry(0,i18n("An error appeared when opening this file"));
        
        if ( !url.isLocalFile() )
                KIO::NetAccess::removeTempFile( tmpfile );
}

void KParameterEditor::cmdExport_clicked()
{
	if ( !m_mainWidget->list->count() )
                return;
        KUrl url = KFileDialog::getSaveURL( QString(),i18n("*.txt|Plain Text File "));
        if ( url.isEmpty() )
                return;

        if( !KIO::NetAccess::exists( url,false,this ) || KMessageBox::warningContinueCancel( this, i18n( "A file named \"%1\" already exists. Are you sure you want to continue and overwrite this file?" ).arg( url.url()), i18n( "Overwrite File?" ), KGuiItem( i18n( "&Overwrite" ) ) ) == KMessageBox::Continue )
        {
                QString tmpfile;
                QFile file;
                if ( !url.isLocalFile() )
                {
                        KTempFile tmpfile;
                        file.setFileName(tmpfile.name() );
                        
                        if (file.open( QIODevice::WriteOnly ) )
                        {
							QTextStream stream(&file);
							for ( int i = 0; i < m_mainWidget->list->count(); i++ )
							{
								QListWidgetItem * it = m_mainWidget->list->item( i );
								stream << it->text();
								if ( i < m_mainWidget->list->count()-1 )
									stream << endl; //only write a new line if there are more text
							}
							file.close();
                        }
						else
							KMessageBox::sorry(0,i18n("An error appeared when saving this file"));
                        
                        if ( !KIO::NetAccess::upload(tmpfile.name(),url, this) )
                        {
							KMessageBox::sorry(0,i18n("An error appeared when saving this file"));
                                tmpfile.unlink();
                                return;
                        }
                        tmpfile.unlink();
                }
                else
                {
                        file.setFileName(url.prettyURL(0));
                        if (file.open( QIODevice::WriteOnly ) )
                        {
							QTextStream stream(&file);
							for ( int i = 0; i < m_mainWidget->list->count(); i++ )
							{
								QListWidgetItem * it = m_mainWidget->list->item( i );
								stream << it->text();
								if ( i < m_mainWidget->list->count()-1 )
									stream << endl; //only write a new line if there are more text
							}
							file.close();
                        }
                        else
							KMessageBox::sorry(0,i18n("An error appeared when saving this file"));
                }
        }


}

void KParameterEditor::varlist_clicked( QListWidgetItem * item )
{
	if (item)
	{
		m_mainWidget->cmdEdit->setEnabled(true);
		m_mainWidget->cmdDelete->setEnabled(true);
	}
	else
	{
		m_mainWidget->cmdEdit->setEnabled(false);
		m_mainWidget->cmdDelete->setEnabled(false);		
	}
}


void KParameterEditor::varlist_doubleClicked( QListWidgetItem * )
{
	cmdEdit_clicked();
}

bool KParameterEditor::checkTwoOfIt(const QString & text)
{
	return !m_mainWidget->list->findItems(text,Qt::MatchExactly).isEmpty();
}

#include "kparametereditor.moc"
