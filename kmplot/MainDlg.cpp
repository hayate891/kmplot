/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999  Klaus-Dieter Möller
*               2000, 2002 kdmoeller@foni.net
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

// Qt includes
#include <QMainWindow>
#include <QPixmap>
#include <qslider.h>
#include <QTimer>

// KDE includes
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kio/netaccess.h>
#include <kinstance.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <ktoolbar.h>
#include <ktoolinvocation.h>

// local includes
#include "functioneditor.h"
#include "kprinterdlg.h"
#include "kconstanteditor.h"
#include "MainDlg.h"
#include "MainDlg.moc"
#include "settings.h"
#include "settingspagecolor.h"
#include "settingspagefonts.h"
#include "ksliderwindow.h"

class XParser;
class KmPlotIO;

bool MainDlg::oldfileversion;

MainDlg::MainDlg(QWidget *parentWidget, const char *, QObject *parent ) :  DCOPObject( "MainDlg" ), KParts::ReadOnlyPart( parent ), m_recentFiles( 0 ), m_modified(false), m_parent(parentWidget)
{
	// we need an instance
	setInstance( KmPlotPartFactory::instance() );

	kDebug() << "parentWidget->objectName():" << parentWidget->objectName() << endl;
	if ( QString(parentWidget->objectName()).startsWith("KmPlot") )
	{
		setXMLFile("kmplot_part.rc");
		m_readonly = false;
	}
	else
	{
		setXMLFile("kmplot_part_readonly.rc");
		m_readonly = true;
		new BrowserExtension(this); // better integration with Konqueror
	}
	
	coordsDialog = 0;
	m_popupmenu = new KMenu( parentWidget );
	m_newPlotMenu = new KMenu( parentWidget );
	view = new View( m_readonly, m_modified, m_popupmenu, parentWidget, actionCollection(), this );
	connect( view, SIGNAL( setStatusBarText(const QString &)), this, SLOT( setReadOnlyStatusBarText(const QString &) ) );
	
	if ( !m_readonly )
	{
		m_functionEditor = new FunctionEditor( view, m_newPlotMenu, parentWidget );
		static_cast<QMainWindow*>(parentWidget)->addDockWidget( Qt::LeftDockWidgetArea, m_functionEditor );
	}
	
	setWidget( view );
	view->setFocusPolicy(Qt::ClickFocus);
	minmaxdlg = new KMinMax(view, m_parent);
	view->setMinMaxDlg(minmaxdlg);
	setupActions();
	view->parser()->constants()->load();
	kmplotio = new KmPlotIO(view->parser());
	m_config = KGlobal::config();
	m_recentFiles->loadEntries( m_config );
	
	
	//BEGIN undo/redo stuff
	m_currentState = kmplotio->currentState();
	m_saveCurrentStateTimer = new QTimer( this );
	m_saveCurrentStateTimer->setSingleShot( true );
	connect( m_saveCurrentStateTimer, SIGNAL(timeout()), this, SLOT(saveCurrentState()) );
	//END undo/redo stuff
	

	// Let's create a Configure Diloag
	m_settingsDialog = new KConfigDialog( parentWidget, "settings", Settings::self() );
	m_settingsDialog->setHelp("general-config");

	// create and add the page(s)
	m_generalSettings = new SettingsPageGeneral( view );
	m_colorSettings = new SettingsPageColor( view );
	m_fontsSettings = new SettingsPageFonts( view );
	m_constantsSettings = new KConstantEditor( view, 0 );
	m_constantsSettings->setObjectName( "constantsSettings" );
	
	m_settingsDialog->addPage( m_generalSettings, i18n("General"), "package_settings", i18n("General Settings") );
	m_settingsDialog->addPage( m_colorSettings, i18n("Colors"), "colorize", i18n("Colors") );
	m_settingsDialog->addPage( m_fontsSettings, i18n("Fonts"), "font", i18n("Fonts") );
	m_settingsDialog->addPage( m_constantsSettings, i18n("Constants"), "editconstants", i18n("Constants") );
	// User edited the configuration - update your local copies of the
	// configuration data
	connect( m_settingsDialog, SIGNAL( settingsChanged( const QString &) ), this, SLOT(updateSettings() ) );
}

MainDlg::~MainDlg()
{
	m_recentFiles->saveEntries( m_config );
	view->parser()->constants()->save();
	delete kmplotio;
}

void MainDlg::setupActions()
{
	// standard actions
	m_recentFiles = KStdAction::openRecent( this, SLOT( slotOpenRecent( const KUrl& ) ), actionCollection(),"file_openrecent");
	KStdAction::print( this, SLOT( slotPrint() ), actionCollection(),"file_print" );
	KStdAction::save( this, SLOT( slotSave() ), actionCollection() );
	KStdAction::saveAs( this, SLOT( slotSaveas() ), actionCollection() );
	connect( kapp, SIGNAL( lastWindowClosed() ), kapp, SLOT( quit() ) );

	KAction *prefs  = KStdAction::preferences( this, SLOT( slotSettings() ), actionCollection());
	prefs->setText( i18n( "Configure KmPlot..." ) );
	KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
	KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());


	// KmPlot specific actions
	
	//BEGIN file menu
	KAction * exportAction = new KAction( i18n( "E&xport..." ), actionCollection(), "export" );
	connect( exportAction, SIGNAL(triggered(bool)), this, SLOT( slotExport() ) );
	//END file menu

	
	//BEGIN edit menu
	m_undoAction = KStdAction::undo( this, SLOT(undo()), actionCollection() );
	m_undoAction->setEnabled( false );
	
	m_redoAction = KStdAction::redo( this, SLOT(redo()), actionCollection() );
	m_redoAction->setEnabled( false );
	
	KAction * editAxes = new KAction( i18n( "&Coordinate System..." ), actionCollection(), "editaxes" );
	editAxes->setIcon( KIcon("coords.png") );
	connect( editAxes, SIGNAL(triggered(bool)), this, SLOT( editAxes() ) );
	
	KAction * editScaling = new KAction( i18n( "&Scaling..." ), actionCollection(), "editscaling" );
	editScaling->setIcon( KIcon("scaling") );
	connect( editScaling, SIGNAL(triggered(bool)), this, SLOT( editScaling() ) );
	//END edit menu
	
	
	//BEGIN view menu
	KAction * zoomIn = new KAction( i18n("Zoom &In"), actionCollection(), "zoom_in" );
	zoomIn->setShortcut( "CTRL+1" );
	zoomIn->setIcon( KIcon("viewmag+") );
	connect( zoomIn, SIGNAL(triggered(bool)), view, SLOT(mnuZoomIn_clicked()) );
	
	KAction * zoomOut = new KAction( i18n("Zoom &Out"), actionCollection(),"zoom_out" );
	zoomOut->setShortcut( "CTRL+2" );
	zoomOut->setIcon( KIcon("viewmag-") );
	connect( zoomOut, SIGNAL(triggered(bool)), view, SLOT( mnuZoomOut_clicked() ) );
	
	KAction * zoomTrig = new KAction( i18n("&Fit Widget to Trigonometric Functions"), actionCollection(), "zoom_trig" );
	connect( zoomTrig, SIGNAL(triggered(bool)), view, SLOT( mnuTrig_clicked() ) );
	
	KAction * coordI = new KAction( i18n( "Coordinate System I" ), actionCollection(), "coord_i" );
	coordI->setIcon( KIcon("ksys1.png") );
	connect( coordI, SIGNAL(triggered(bool)), this, SLOT( slotCoord1() ) );
	
	KAction * coordII = new KAction( i18n( "Coordinate System II" ), actionCollection(), "coord_ii" );
	coordII->setIcon( KIcon("ksys2.png") );
	connect( coordII, SIGNAL(triggered(bool)), this, SLOT( slotCoord2() ) );
	
	KAction * coordIII = new KAction( i18n( "Coordinate System III" ), actionCollection(), "coord_iii" );
	coordIII->setIcon( KIcon("ksys3.png") );
	connect( coordIII, SIGNAL(triggered(bool)), this, SLOT( slotCoord3() ) );
	//END view menu
	
	
	//BEGIN tools menu
	KAction *mnuYValue =  new KAction( i18n( "&Get y-Value..." ), actionCollection(), "yvalue" );
	mnuYValue->setIcon( KIcon("") );
	connect( mnuYValue, SIGNAL(triggered(bool)), this, SLOT( getYValue() ) );
	
	KAction *mnuMinValue = new KAction( i18n( "&Search for Minimum Value..." ), actionCollection(), "minimumvalue" );
	mnuMinValue->setIcon( KIcon("minimum") );
	connect( mnuMinValue, SIGNAL(triggered(bool)), this, SLOT( findMinimumValue() ) );
	
	KAction *mnuMaxValue = new KAction( i18n( "&Search for Maximum Value..." ), actionCollection(), "maximumvalue" );
	mnuMaxValue->setIcon( KIcon("maximum") );
	connect( mnuMaxValue, SIGNAL(triggered(bool)), this, SLOT( findMaximumValue() ) );
	
	KAction *mnuArea = new KAction( i18n( "&Area Under Graph..." ), actionCollection(), "grapharea" );
	mnuArea->setIcon( KIcon("") );
	connect( mnuArea, SIGNAL(triggered(bool)),this, SLOT( graphArea() )  );
	//END tools menu

	
	//BEGIN help menu
	KAction * namesAction = new KAction( i18n( "Predefined &Math Functions" ), actionCollection(), "names" );
	namesAction->setIcon( KIcon("functionhelp") );
	connect( namesAction, SIGNAL(triggered(bool)), this, SLOT( slotNames() ) );
	//END help menu
	
	
	//BEGIN new plots menu
	KAction * newFunction = new KAction( i18n( "Cartesian Plot" ), actionCollection(), "newcartesian" );
	newFunction->setIcon( KIcon("newfunction") );
	connect( newFunction, SIGNAL(triggered(bool)), m_functionEditor, SLOT( createCartesian() ) );
	newFunction->plug( m_newPlotMenu );
        
	KAction * newParametric = new KAction( i18n( "Parametric Plot" ), actionCollection(), "newparametric" );
	newParametric->setIcon( KIcon("newparametric") );
	connect( newParametric, SIGNAL(triggered(bool)), m_functionEditor, SLOT( createParametric() ) );
	newParametric->plug( m_newPlotMenu );
        
	KAction * newPolar = new KAction( i18n( "Polar Plot" ), actionCollection(), "newpolar" );
	newPolar->setIcon( KIcon("newpolar") );
	connect( newPolar, SIGNAL(triggered(bool)), m_functionEditor, SLOT( createPolar() ) );
	newPolar->plug( m_newPlotMenu );
	//END new plots menu
	
	

	view->m_menuSliderAction = new KToggleAction( i18n( "Show Sliders" ), actionCollection(), "options_configure_show_sliders" );
	connect( view->m_menuSliderAction, SIGNAL(triggered(bool)), this, SLOT( toggleShowSliders() ) );
	

	// Popup menu
	KAction *mnuHide = new KAction(i18n("&Hide"), actionCollection(),"mnuhide" );
	connect( mnuHide, SIGNAL(triggered(bool)), view, SLOT( mnuHide_clicked() ) );
	mnuHide->plug(m_popupmenu);
	
	KAction *mnuRemove = new KAction(i18n("&Remove"), actionCollection(),"mnuremove"  );
	mnuRemove->setIcon( KIcon("editdelete") );
	connect( mnuRemove, SIGNAL(triggered(bool)), view, SLOT( mnuRemove_clicked() ) );
	mnuRemove->plug(m_popupmenu);
	
	KAction *mnuEdit = new KAction(i18n("&Edit"), actionCollection(),"mnuedit"  );
	mnuEdit->setIcon( KIcon("editplots") );
	connect(mnuEdit , SIGNAL(triggered(bool)), view, SLOT( mnuEdit_clicked() ) );
	mnuEdit->plug(m_popupmenu);
	
	m_popupmenu->addSeparator();
	
	KAction *mnuCopy = new KAction(i18n("&Copy"), actionCollection(),"mnucopy"  );
	mnuCopy->setIcon( KIcon("") );
	connect( mnuCopy, SIGNAL(triggered(bool)), view, SLOT( mnuCopy_clicked() ) );
	mnuCopy->plug(m_popupmenu);
	
	KAction *mnuMove = new KAction(i18n("&Move"),actionCollection(),"mnumove"  );
	mnuMove->setIcon( KIcon("") );
	connect( mnuMove, SIGNAL(triggered(bool)), view, SLOT( mnuMove_clicked() ) );
	mnuMove->plug(m_popupmenu);
	
	m_popupmenu->addSeparator();
	mnuYValue->plug(m_popupmenu);
	mnuMinValue->plug(m_popupmenu);
	mnuMaxValue->plug(m_popupmenu);
	mnuArea->plug(m_popupmenu);
}


void MainDlg::undo()
{
	kDebug() << k_funcinfo << endl;
	
	if ( m_undoStack.isEmpty() )
		return;
	
	m_redoStack.push( m_currentState );
	m_currentState = m_undoStack.pop();
	
	kmplotio->restore( m_currentState );
	view->drawPlot();
	
	m_undoAction->setEnabled( !m_undoStack.isEmpty() );
	m_redoAction->setEnabled( true );
}


void MainDlg::redo()
{
	kDebug() << k_funcinfo << endl;
	
	if ( m_redoStack.isEmpty() )
		return;
	
	m_undoStack.push( m_currentState );
	m_currentState = m_redoStack.pop();
	
	kmplotio->restore( m_currentState );
	view->drawPlot();
	
	m_undoAction->setEnabled( true );
	m_redoAction->setEnabled( !m_redoStack.isEmpty() );
}


void MainDlg::requestSaveCurrentState()
{
	m_saveCurrentStateTimer->start( 0 );
}
void MainDlg::saveCurrentState( )
{
	kDebug() << k_funcinfo << endl;
	
	m_redoStack.clear();
	m_undoStack.push( m_currentState );
	m_currentState = kmplotio->currentState();
	
	// limit stack size to 100 items
	while ( m_undoStack.count() > 100 )
		m_undoStack.pop_front();
	
	m_undoAction->setEnabled( true );
	m_redoAction->setEnabled( false );
}


bool MainDlg::checkModified()
{
	if( m_modified )
	{
		int saveit = KMessageBox::warningYesNoCancel( m_parent, i18n( "The plot has been modified.\n"
		             "Do you want to save it?" ), QString(), KStdGuiItem::save(), KStdGuiItem::discard() );
		switch( saveit )
		{
			case KMessageBox::Yes:
				slotSave();
				if ( m_modified) // the user didn't saved the file
					return false;
				break;
			case KMessageBox::Cancel:
				return false;
		}
	}
	return true;
}


void MainDlg::slotSave()
{
	if ( !m_modified || m_readonly) //don't save if no changes are made or readonly is enabled
		return;
	if ( m_url.isEmpty() )            // if there is no file name set yet
		slotSaveas();
	else
	{
		if ( !m_modified) //don't save if no changes are made
			return;

		if ( oldfileversion)
		{
			if ( KMessageBox::warningContinueCancel( m_parent, i18n( "This file is saved with an old file format; if you save it, you cannot open the file with older versions of Kmplot. Are you sure you want to continue?" ), QString(), i18n("Save New Format") ) == KMessageBox::Cancel)
				return;
		}
		kmplotio->save( m_url );
		kDebug() << "saved" << endl;
		m_modified = false;
	}

}

void MainDlg::slotSaveas()
{
	if (m_readonly)
		return;
	const KUrl url = KFileDialog::getSaveURL( QDir::currentPath(), i18n( "*.fkt|KmPlot Files (*.fkt)\n*|All Files" ), m_parent, i18n( "Save As" ) );

	if ( !url.isEmpty() )
	{
		// check if file exists and overwriting is ok.
		if( !KIO::NetAccess::exists( url,false,m_parent ) || KMessageBox::warningContinueCancel( m_parent, i18n( "A file named \"%1\" already exists. Are you sure you want to continue and overwrite this file?" ).arg( url.url()), i18n( "Overwrite File?" ), KGuiItem( i18n( "&Overwrite" ) ) ) == KMessageBox::Continue )
		{
			if ( !kmplotio->save( url ) )
				KMessageBox::error(m_parent, i18n("The file could not be saved") );
			else
			{
				m_url = url;
				m_recentFiles->addUrl( url );
				setWindowCaption( m_url.prettyURL(0) );
				m_modified = false;
			}
			return;
		}
	}
}

void MainDlg::slotExport()
{
	KUrl const url = KFileDialog::getSaveURL(QDir::currentPath(),
	                 i18n("*.svg|Scalable Vector Graphics (*.svg)\n"
	                      "*.bmp|Bitmap 180dpi (*.bmp)\n"
	                      "*.png|Bitmap 180dpi (*.png)"), m_parent, i18n("Export") );
	if(!url.isEmpty())
	{
		// check if file exists and overwriting is ok.
		if( KIO::NetAccess::exists(url,false,m_parent ) && KMessageBox::warningContinueCancel( m_parent, i18n( "A file named \"%1\" already exists. Are you sure you want to continue and overwrite this file?" ).arg(url.url() ), i18n( "Overwrite File?" ), KGuiItem( i18n( "&Overwrite" ) ) ) != KMessageBox::Continue ) return;

		if( url.fileName().right(4).toLower()==".svg")
		{
			QPicture pic;
			view->draw(&pic, 2);
			if (url.isLocalFile() )
				pic.save( url.prettyURL(0), "SVG");
			else
			{
				KTempFile tmp;
				pic.save( tmp.name(), "SVG");
				if ( !KIO::NetAccess::upload(tmp.name(), url, 0) )
					KMessageBox::error(m_parent, i18n("The URL could not be saved.") );
				tmp.unlink();
			}
		}

		else if( url.fileName().right(4).toLower()==".bmp")
		{
			QPixmap pic(100, 100);
			view->draw(&pic, 3);
			if (url.isLocalFile() )
				pic.save(  url.prettyURL(0), "BMP");
			else
			{
				KTempFile tmp;
				pic.save( tmp.name(), "BMP");
				if ( !KIO::NetAccess::upload(tmp.name(), url, 0) )
					KMessageBox::error(m_parent, i18n("The URL could not be saved.") );
				tmp.unlink();
			}
		}

		else if( url.fileName().right(4).toLower()==".png")
		{
			QPixmap pic(100, 100);
			view->draw(&pic, 3);
			if (url.isLocalFile() )
				pic.save( url.prettyURL(0), "PNG");
			else
			{
				KTempFile tmp;
				pic.save( tmp.name(), "PNG");
				if ( !KIO::NetAccess::upload(tmp.name(), url, 0) )
					KMessageBox::error(m_parent, i18n("The URL could not be saved.") );
				tmp.unlink();
			}
		}
	}
}
bool MainDlg::openFile()
{
	view->init();
	if (m_url==m_currentfile || !kmplotio->load( m_url ) )
	{
		m_recentFiles->removeUrl(m_url ); //remove the file from the recent-opened-file-list
		m_url = "";
		return false;
	}
	m_currentfile = m_url;
	m_recentFiles->addUrl( m_url.prettyURL(0)  );
	setWindowCaption( m_url.prettyURL(0) );
	m_modified = false;
	view->updateSliders();
	view->drawPlot();
	return true;
}

void MainDlg::slotOpenRecent( const KUrl &url )
{
	if( isModified() || !m_url.isEmpty() ) // open the file in a new window
	{
		QByteArray data;
		QDataStream stream( &data,QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_3_1);
		stream << url;
		KApplication::kApplication()->dcopClient()->send(KApplication::kApplication()->dcopClient()->appId(), "KmPlotShell","openFileInNewWindow(KUrl)", data);
		return;
	}

	view->init();
	if ( !kmplotio->load( url ) ) //if the loading fails
	{
		m_recentFiles->removeUrl(url ); //remove the file from the recent-opened-file-list
		return;
	}
  m_url = m_currentfile = url;
	m_recentFiles->setCurrentItem(-1); //don't select the item in the open-recent menu
  setWindowCaption( m_url.prettyURL(0) );
  m_modified = false;
	view->updateSliders();
	view->drawPlot();
}

void MainDlg::slotPrint()
{
	KPrinter prt( QPrinter::PrinterResolution );
	prt.setResolution( 72 );
	KPrinterDlg* printdlg = new KPrinterDlg( m_parent );
	printdlg->setObjectName( "KmPlot page" );
	prt.addDialogPage( printdlg );
	if ( prt.setup( m_parent, i18n( "Print Plot" ) ) )
	{
		prt.setFullPage( true );
		view->draw(&prt, 1);
	}
}

void MainDlg::editAxes()
{
	// create a config dialog and add a axes page
	if ( !coordsDialog)
	{
		coordsDialog = new CoordsConfigDialog( view->parser(), m_parent);
		// User edited the configuration - update your local copies of the
		// configuration data
		connect( coordsDialog, SIGNAL( settingsChanged(const QString &) ), this, SLOT(updateSettings() ) );
	}
	coordsDialog->show();
}

void MainDlg::editScaling()
{
	// create a config dialog and add a scaling page
	KConfigDialog *scalingDialog = new KConfigDialog( m_parent, "scaling", Settings::self() );
	scalingDialog->setHelp("scaling-config");
	EditScaling *es = new EditScaling();
	es->setObjectName( "scalingSettings" );
	scalingDialog->addPage( es, i18n( "Scale" ), "scaling", i18n( "Edit Scaling" ) );
	// User edited the configuration - update your local copies of the
	// configuration data
	connect( scalingDialog, SIGNAL( settingsChanged(const QString &) ), this, SLOT(updateSettings() ) );
	scalingDialog->show();
}

void MainDlg::slotNames()
{
	KToolInvocation::invokeHelp( "func-predefined", "kmplot" );
}


void MainDlg::slotCoord1()
{
	Settings::setXRange( 0 );
	Settings::setYRange( 0 );
	m_modified = true;
	view->drawPlot();
}

void MainDlg::slotCoord2()
{
	Settings::setXRange( 2 );
	Settings::setYRange( 0 );
	m_modified = true;
	view->drawPlot();
}

void MainDlg::slotCoord3()
{
	Settings::setXRange( 2 );
	Settings::setYRange( 2 );
	m_modified = true;
	view->drawPlot();
}

void MainDlg::slotSettings()
{
	// An instance of your dialog has already been created and has been cached,
	// so we want to display the cached dialog instead of creating
	// another one
	KConfigDialog::showDialog( "settings" );
}

void MainDlg::updateSettings()
{
	view->getSettings();
	m_modified = true;
	view->drawPlot();
}


void MainDlg::getYValue()
{
	minmaxdlg->init(2);
	minmaxdlg->show();
}

void MainDlg::findMinimumValue()
{
	minmaxdlg->init(0);
	minmaxdlg->show();
}

void MainDlg::findMaximumValue()
{
	minmaxdlg->init(1);
	minmaxdlg->show();
}

void MainDlg::graphArea()
{
	minmaxdlg->init(3);
	minmaxdlg->show();
}

void MainDlg::toggleShowSliders()
{
	// create the slider if it not exists already
	if ( !view->m_sliderWindow )
	{
		view->m_sliderWindow = new KSliderWindow( view, actionCollection());
		connect( view->m_sliderWindow, SIGNAL( valueChanged() ), view, SLOT( drawPlot() ) );
		connect( view->m_sliderWindow, SIGNAL( windowClosed() ), view, SLOT( slidersWindowClosed() ) );
	}
	if ( !view->m_sliderWindow->isVisible() )
		view->m_sliderWindow->show();
	else
		view->m_sliderWindow->hide();
}

void MainDlg::setReadOnlyStatusBarText(const QString &text)
{
	setStatusBarText(text);
}

void MainDlg::optionsConfigureKeys()
{
	KApplication::kApplication()->dcopClient()->send(KApplication::kApplication()->dcopClient()->appId(), "KmPlotShell","optionsConfigureKeys()", QByteArray());
}

void MainDlg::optionsConfigureToolbars()
{
	KApplication::kApplication()->dcopClient()->send(KApplication::kApplication()->dcopClient()->appId(), "KmPlotShell","optionsConfigureToolbars()", QByteArray());
}

// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
#include <kaboutdata.h>
#include <klocale.h>
#include <ktoolinvocation.h>
#include <kglobal.h>

KInstance*  KmPlotPartFactory::s_instance = 0L;
KAboutData* KmPlotPartFactory::s_about = 0L;

KmPlotPartFactory::KmPlotPartFactory()
		: KParts::Factory()
{}

KmPlotPartFactory::~KmPlotPartFactory()
{
	delete s_instance;
	delete s_about;

	s_instance = 0L;
}

KParts::Part* KmPlotPartFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
        QObject *parent, const char *, const char *, const QStringList & )
{
	// Create an instance of our Part
	MainDlg* obj = new MainDlg( parentWidget, widgetName, parent );
	emit objectCreated( obj );
	return obj;
}

KInstance* KmPlotPartFactory::instance()
{
	if( !s_instance )
	{
		s_about = new KAboutData("kmplot",I18N_NOOP( "KmPlotPart" ), "1");
		s_instance = new KInstance(s_about);
	}
	return s_instance;
}

extern "C"
{
	KDE_EXPORT void* init_libkmplotpart()
	{
		return new KmPlotPartFactory;
	}
}


/// BrowserExtension class
BrowserExtension::BrowserExtension(MainDlg* parent)
		: KParts::BrowserExtension( parent )
{
	emit enableAction("print", true);
	setURLDropHandlingEnabled(true);
}

void BrowserExtension::print()
{
	static_cast<MainDlg*>(parent())->slotPrint();
}

