#include "misc.h"


KApplication *ka;
KConfig *kc;

XParser ps(10, 200, 20);

	char mode,			// Diagrammodus
	     g_mode;		// Rastermodus

	int koordx,			// 0 => [-8|+8]
	    koordy,			// 1 => [-5|+5]
						// 2 => [0|+16]
						// 3 => [0|+10]
	    AchsenDicke,
	    GitterDicke,
	    TeilstrichDicke,
	    TeilstrichLaenge;

	double xmin,                   	// min. x-Wert
	       xmax,                   	// max. x-Wert
           ymin,                	// min. y-Wert
           ymax,                	// max. y-Wert
             sw,					// Schrittweite
            rsw,					// rel. Schrittweite
           tlgx,					// x-Achsenteilung
           tlgy,					// y-Achsenteilung
           drskalx,					// x-Ausdruckskalierung
           drskaly;					// y-Ausdruckskalierung

    	QString	datei,				// Dateiname
    		xminstr,				// String f�r xmind
            xmaxstr,				// String f�r xmaxd
    		yminstr,				// String f�r ymind
    		ymaxstr,				// String f�r ymaxd
    		tlgxstr,                // String f�r tlgx
    		tlgystr,                // String f�r tlgy
    		drskalxstr,             // String f�r drskalx
    		drskalystr;             // String f�r drskaly

	QRgb AchsenFarbe,
	     GitterFarbe;



void init()
{	int ix;
	
	mode=ACHSEN | PFEILE | EXTRAHMEN;
	koordx=koordy=0;
	rsw=1.;
	datei="";
	
	// Standardwerte f�r die Achsen
	
	kc->setGroup("Achsen");
	xminstr=kc->readEntry("xmin", "-8");
	xmin=ps.eval(xminstr); 
	xmaxstr=kc->readEntry("xmax", "8");
	xmax=ps.eval(xmaxstr);
	yminstr=kc->readEntry("ymin", "-8");
	ymin=ps.eval(yminstr);
	ymaxstr=kc->readEntry("ymax", "8");
	ymax=ps.eval(ymaxstr);

    tlgxstr=kc->readEntry("tlgx", "1");
	tlgx=ps.eval(tlgxstr);
    tlgystr=kc->readEntry("tlgy", "1");
	tlgy=ps.eval(tlgystr);

    drskalxstr=kc->readEntry("drskalx", "1");
	drskalx=ps.eval(drskalxstr);
    drskalystr=kc->readEntry("drskaly", "1");
	drskaly=ps.eval(drskalystr);

	AchsenDicke=kc->readNumEntry("Achsenst�rke", 5);
	TeilstrichDicke=kc->readNumEntry("Teilstrichst�rke", 3);
	TeilstrichLaenge=kc->readNumEntry("Teilstrichl�nge", 10);
	AchsenFarbe=kc->readColorEntry("Achsenfarbe", &QColor(0, 0, 0)).rgb();
	if(kc->readNumEntry("Beschriftung", 1)) mode|=BESCHRIFTUNG;
	
	// Standardwerte f�r das Gitter

	kc->setGroup("Gitter");
	GitterDicke=kc->readNumEntry("Gitterst�rke", 1);
	g_mode=kc->readNumEntry("Mode", 1);
	GitterFarbe=kc->readColorEntry("Gitterfarbe", &QColor(192, 192, 192)).rgb();

	// Standardwerte f�r die Graphen

	kc->setGroup("Graphen");
	ps.dicke0=kc->readNumEntry("Linienst�rke", 5);
	ps.fktext[0].farbe0=kc->readColorEntry("Farbe0", &QColor(255, 0, 0)).rgb();
	ps.fktext[1].farbe0=kc->readColorEntry("Farbe1", &QColor(0, 0, 255)).rgb();
	ps.fktext[2].farbe0=kc->readColorEntry("Farbe2", &QColor(0, 255, 0)).rgb();
	ps.fktext[3].farbe0=kc->readColorEntry("Farbe3", &QColor(255, 0, 255)).rgb();
	ps.fktext[4].farbe0=kc->readColorEntry("Farbe4", &QColor(255, 255, 0)).rgb();
	ps.fktext[5].farbe0=kc->readColorEntry("Farbe5", &QColor(0, 255, 255)).rgb();
	ps.fktext[6].farbe0=kc->readColorEntry("Farbe6", &QColor(0, 128, 0)).rgb();
	ps.fktext[7].farbe0=kc->readColorEntry("Farbe7", &QColor(0, 0, 128)).rgb();
	ps.fktext[8].farbe0=kc->readColorEntry("Farbe8", &QColor(0, 0, 0)).rgb();
	ps.fktext[9].farbe0=kc->readColorEntry("Farbe9", &QColor(0, 0, 0)).rgb();

	for(ix=0; ix<ps.ufanz; ++ix) ps.delfkt(ix);
}
