/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 2004  Fredrik Edemar <f_edemar@linux.se>
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

#include "kparametereditor.h"

#include <kdebug.h>
#include <kfiledialog.h>
#include <KIcon>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <ktemporaryfile.h>
#include <kurl.h>
#include <qfile.h>
#include <qtextstream.h>
#include <QList>
#include <QListWidget>

#include <assert.h>

class ParameterValueList;

KParameterEditor::KParameterEditor( QList<Value> *l, QWidget *parent )
	: KDialog( parent ),
	  m_parameter(l)
{
	setCaption( i18n( "Parameter Editor" ) );
	setButtons( Ok | Cancel );

	m_mainWidget = new QParameterEditor( this );
	m_mainWidget->layout()->setMargin( 0 );
	setMainWidget( m_mainWidget );
	
	m_mainWidget->cmdNew->setIcon( KIcon("document-new" ) );
	m_mainWidget->cmdDelete->setIcon( KIcon("edit-delete" ) );
	m_mainWidget->moveUp->setIcon( KIcon("go-up") );
	m_mainWidget->moveDown->setIcon( KIcon("go-down") );
	m_mainWidget->cmdImport->setIcon( KIcon("document-open") );
	m_mainWidget->cmdExport->setIcon( KIcon("document-save") );
	
	m_mainWidget->list->setFocusPolicy( Qt::NoFocus );
	
	connect( m_mainWidget->value, SIGNAL(upPressed()), this, SLOT(prev()) );
	connect( m_mainWidget->value, SIGNAL(downPressed()), this, SLOT(next()) );
	
	foreach ( const Value &v, *m_parameter )
		m_mainWidget->list->addItem( v.expression() );
	
	connect( m_mainWidget->cmdNew, SIGNAL( clicked() ), this, SLOT( cmdNew_clicked() ));
	connect( m_mainWidget->cmdDelete, SIGNAL( clicked() ), this, SLOT( cmdDelete_clicked() ));
	connect( m_mainWidget->moveUp, SIGNAL(clicked()), this, SLOT(moveUp()) );
	connect( m_mainWidget->moveDown, SIGNAL(clicked()), this, SLOT(moveDown()) );
	connect( m_mainWidget->cmdImport, SIGNAL( clicked() ), this, SLOT( cmdImport_clicked() ));
	connect( m_mainWidget->cmdExport, SIGNAL( clicked() ), this, SLOT( cmdExport_clicked() ));
	connect( m_mainWidget->list, SIGNAL(currentItemChanged( QListWidgetItem *, QListWidgetItem * )), this, SLOT(selectedConstantChanged( QListWidgetItem * )) );
	
	connect( m_mainWidget->value, SIGNAL( textEdited( const QString & ) ), this, SLOT( saveCurrentValue() ) );
	connect( m_mainWidget->value, SIGNAL( textChanged( const QString & ) ), this, SLOT( checkValueValid() ) );
	connect( m_mainWidget->value, SIGNAL( returnPressed() ), m_mainWidget->cmdNew, SLOT( animateClick() ) );
	
	checkValueValid();
	
	m_mainWidget->value->setFocus();
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
		{
			Value value;
			if ( value.updateExpression( item_text ) )
				m_parameter->append( value );
		}
	}
	
	KDialog::accept();
}


void KParameterEditor::moveUp()
{
	int current = m_mainWidget->list->currentRow();
	
	if ( current == 0 )
		return;
	
	QListWidgetItem * item = m_mainWidget->list->takeItem( current-1 );
	m_mainWidget->list->insertItem( current, item );
}


void KParameterEditor::moveDown()
{
	int current = m_mainWidget->list->currentRow();
	
	if ( current == m_mainWidget->list->count() - 1 )
		return;
	
	QListWidgetItem * item = m_mainWidget->list->takeItem( current+1 );
	m_mainWidget->list->insertItem( current, item );
}


void KParameterEditor::cmdNew_clicked()
{
	QListWidgetItem * item = new QListWidgetItem( m_mainWidget->list );
	item->setText( "0" );
	m_mainWidget->list->setCurrentItem( item );
	m_mainWidget->value->setFocus();
	m_mainWidget->value->selectAll();
}


void KParameterEditor::prev()
{
	int current = m_mainWidget->list->currentRow();
	if ( current > 0 )
		m_mainWidget->list->setCurrentRow( current-1 );
}


void KParameterEditor::next()
{
	int current = m_mainWidget->list->currentRow();
	if ( current < m_mainWidget->list->count()-1 )
		m_mainWidget->list->setCurrentRow( current+1 );
	else
		cmdNew_clicked();
}


void KParameterEditor::selectedConstantChanged( QListWidgetItem * current )
{
	m_mainWidget->cmdDelete->setEnabled( current != 0 );
	m_mainWidget->value->setText( current ? current->text() : QString() );
	m_mainWidget->value->selectAll();
}


void KParameterEditor::cmdDelete_clicked()
{
	QListWidgetItem * item = m_mainWidget->list->currentItem();
	if ( !item )
		return;
	
	m_mainWidget->value->clear();
	m_mainWidget->list->takeItem( m_mainWidget->list->currentRow() );
	delete item;
	
	m_mainWidget->cmdDelete->setEnabled( m_mainWidget->list->currentItem() != 0 );
}


void KParameterEditor::saveCurrentValue()
{
	QListWidgetItem * current = m_mainWidget->list->currentItem();
	if ( !current )
		current = new QListWidgetItem( m_mainWidget->list );
	
	current->setText( m_mainWidget->value->text() );
	m_mainWidget->list->setCurrentItem( current );
}


bool KParameterEditor::checkValueValid()
{
	QString valueText = m_mainWidget->value->text();
	Parser::Error error;
	(double) XParser::self()->eval( valueText, & error );
	bool valid = (error == Parser::ParseSuccess);
	m_mainWidget->valueInvalidLabel->setVisible( !valueText.isEmpty() && !valid );
	return valid;
}


void KParameterEditor::cmdImport_clicked()
{
	KUrl url = KFileDialog::getOpenUrl( QString(),i18n("*.txt|Plain Text File "));
	if ( url.isEmpty() )
		return;
        
        if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, this) )
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
                file.setFileName(url.path() );
	
	if ( file.open(QIODevice::ReadOnly) )
	{
		QTextStream stream(&file);
		QString line;
		for( int i=1; !stream.atEnd();i++ )
		{
			line = stream.readLine();
			if (line.isEmpty())
				continue;
			
			Parser::Error error;
			XParser::self()->eval( line, & error );
			if ( error == Parser::ParseSuccess )
			{
				if ( !checkTwoOfIt(line) )
				{
					m_mainWidget->list->addItem(line);
				}
			}
			else if ( !verbose)
			{
				if ( KMessageBox::warningContinueCancel(this,i18n("Line %1 is not a valid parameter value and will therefore not be included. Do you want to continue?", i) ) == KMessageBox::Cancel)
				{
					file.close();
                                        KIO::NetAccess::removeTempFile( tmpfile );
					return;
				}
				else if (KMessageBox::warningYesNo(this,i18n("Would you like to be informed about other lines that cannot be read?"), QString(), KGuiItem(i18n("Get Informed")), KGuiItem(i18n("Ignore Information")) ) == KMessageBox::No)
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
	KUrl url = KFileDialog::getSaveUrl( QString(),i18n("*.txt|Plain Text File "));
        if ( url.isEmpty() )
                return;

        if( !KIO::NetAccess::exists( url, KIO::NetAccess::DestinationSide, this ) || KMessageBox::warningContinueCancel( this, i18n( "A file named \"%1\" already exists. Are you sure you want to continue and overwrite this file?", url.url()), i18n( "Overwrite File?" ), KGuiItem( i18n( "&Overwrite" ) ) ) == KMessageBox::Continue )
        {
                if ( !url.isLocalFile() )
                {
                        KTemporaryFile tmpfile;
                        
			if (tmpfile.open() )
			{
				QTextStream stream(&tmpfile);
				for ( int i = 0; i < m_mainWidget->list->count(); i++ )
				{
					QListWidgetItem * it = m_mainWidget->list->item( i );
					stream << it->text();
					if ( i < m_mainWidget->list->count()-1 )
						stream << endl; //only write a new line if there are more text
				}
				stream.flush();
			}
			else
				KMessageBox::sorry(0,i18n("An error appeared when saving this file"));
                        
                        if ( !KIO::NetAccess::upload(tmpfile.fileName(),url, this) )
                        {
				KMessageBox::sorry(0,i18n("An error appeared when saving this file"));
                                return;
                        }
                }
                else
                {
	                QFile file;
			kDebug() << "url.path()="<<url.path();
			file.setFileName(url.path());
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


bool KParameterEditor::checkTwoOfIt(const QString & text)
{
	return !m_mainWidget->list->findItems(text,Qt::MatchExactly).isEmpty();
}

#include "kparametereditor.moc"
