kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kmplotpart_PART_SRCS 
   parser.cpp 
   xparser.cpp 
   diagr.cpp 
   View.cpp 
   MainDlg.cpp 
   FktDlg.cpp 
   kprinterdlg.cpp 
   kconstanteditor.cpp 
   kparametereditor.cpp 
   keditparametric.cpp 
   keditpolar.cpp 
   editfunction.cpp 
   kminmax.cpp 
   keditconstant.cpp 
   kmplotio.cpp 
   ksliderwindow.cpp 
   coordsconfigdialog.cpp )

kde4_automoc(${kmplotpart_PART_SRCS})

kde4_add_ui3_files(kmplotpart_PART_SRCS 
   FktDlgData.ui 
   settingspagecolor.ui 
   qparametereditor.ui 
   settingspagecoords.ui 
   settingspagefonts.ui 
   settingspagescaling.ui 
   settingspageprecision.ui 
   qeditparametric.ui 
   qeditpolar.ui 
   editfunctionpage.ui 
   editderivativespage.ui 
   editintegralpage.ui 
   qeditconstant.ui 
   qminmax.ui 
   qconstanteditor.ui 
   sliderwindow.ui )

kde4_add_dcop_skels(kmplotpart_PART_SRCS MainDlgIface.h parseriface.h Viewiface.h )

kde4_add_kcfg_files(kmplotpart_PART_SRCS settings.kcfgc )

kde4_add_plugin(kmplotpart WITH_PREFIX ${kmplotpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kmplotpart )

target_link_libraries(kmplotpart  ${QT_AND_KDECORE_LIBS} kdeui kdeprint )

install_targets(${LIB_INSTALL_DIR}/kde4 kmplotpart )

########### next target ###############                                                                                                                         set(kmplot_SRCS main.cpp kmplot.cpp kmplotprogress.cpp ${kmplotpart_PART_SRCS})

kde4_automoc(${kmplot_SRCS})

kde4_add_dcop_skels(kmplot_SRCS kmplotIface.h )

kde4_add_executable(kmplot ${kmplot_SRCS})

target_link_libraries(kmplot  ${QT_AND_KDECORE_LIBS} kparts )

install_targets(/bin kmplot ) 

########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kmplot.desktop )
install_files( ${KCFG_INSTALL_DIR} FILES kmplot.kcfg )
install_files( ${SERVICES_INSTALL_DIR} FILES kmplot_part.desktop )
install_files( ${MIME_INSTALL_DIR}/application FILES x-kmplot.desktop )
install_files( ${DATA_INSTALL_DIR}/kmplot FILES kmplot_part.rc kmplot_part_readonly.rc )
install_files( ${DATA_INSTALL_DIR}/kmplot FILES kmplot_shell.rc )

kde4_footer()



#original Makefile.am contents follow:

## set the include path for X, qt and KDE
#INCLUDES= $(all_includes)
#
## these are the headers for your project
#noinst_HEADERS = kmplot.h kmplotprogress.h MainDlg.h
#
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(EXTRACTRC) *.ui >> rc.cpp
#	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
#	if test -n "$$LIST"; then \
#	 $(XGETTEXT) $$LIST -o $(podir)/kmplot.pot; \
#	fi
## messages: rc.cpp
## $(XGETTEXT) *.cpp -o $(podir)/kmplot.pot
#
#
##########################################################################
## APPLICATION SECTION
##########################################################################
## this is the program that gets installed.  it's name is used for all
## of the other Makefile.am variables
#bin_PROGRAMS = kmplot
#
## the application source, library search path, and link libraries
#kmplot_SOURCES = main.cpp kmplot.cpp kmplotprogress.cpp kmplotIface.skel
#kmplot_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#kmplot_LDADD   = $(LIB_KPARTS)
#
## this is where the desktop file will go
#appmimedir = $(kde_mimedir)/application
#appmime_DATA = x-kmplot.desktop
#
## this is where the shell's XML-GUI resource file goes
#shellrcdir   = $(kde_datadir)/kmplot
#shellrc_DATA = kmplot_shell.rc
#
#
##########################################################################
## KPART SECTION
##########################################################################
#kde_module_LTLIBRARIES = libkmplotpart.la
#
## the Part's source, library search path, and link libraries
#libkmplotpart_la_SOURCES = FktDlgData.ui parser.cpp xparser.cpp  diagr.cpp View.cpp MainDlg.cpp FktDlg.cpp kprinterdlg.cpp settings.kcfgc settingspagecolor.ui kconstanteditor.cpp qparametereditor.ui kparametereditor.cpp settingspagecoords.ui settingspagefonts.ui settingspagescaling.ui settingspageprecision.ui qeditparametric.ui keditparametric.cpp qeditpolar.ui keditpolar.cpp editfunctionpage.ui editderivativespage.ui editintegralpage.ui editfunction.cpp qeditconstant.ui qminmax.ui kminmax.cpp qconstanteditor.ui keditconstant.cpp kmplotio.cpp sliderwindow.ui ksliderwindow.cpp MainDlgIface.skel parseriface.skel Viewiface.skel coordsconfigdialog.cpp
#libkmplotpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#libkmplotpart_la_LIBADD  = $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET) $(LIB_KDEPRINT)
#
## this is where the desktop file will go
#partdesktopdir   = $(kde_servicesdir)
#partdesktop_DATA = kmplot_part.desktop
#
## this is where the part's XML-GUI resource file goes
#partrcdir   = $(kde_datadir)/kmplot
#partrc_DATA = kmplot_part.rc kmplot_part_readonly.rc
#
#EXTRA_DIST = kmplot.desktop FktDlg.h MainDlg.cpp MainDlg.h View.h diagr.cpp diagr.h  xparser.h parser.h kprinterdlg.h keditparametric.h keditpolar.h ksliderwindow.h keditconstant.h kconstanteditor.h kparametereditor.h kmplotio.h kminmax.h coordsconfigdialog.h
#
#xdg_apps_DATA = kmplot.desktop
#kde_kcfg_DATA = kmplot.kcfg