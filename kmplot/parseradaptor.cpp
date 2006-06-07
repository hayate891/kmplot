/*
 * This file was generated by dbusidl2cpp version 0.5
 * when processing input file org.kde.kmplot.Parser.xml
 *
 * dbusidl2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 */

#include "parseradaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class ParserAdaptor
 */

ParserAdaptor::ParserAdaptor(QObject *parent)
   : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

ParserAdaptor::~ParserAdaptor()
{
    // destructor
}

void ParserAdaptor::drawPlot()
{
    // handle method call org.kde.kmplot.Parser.drawPlot
    QMetaObject::invokeMethod(parent(), "drawPlot");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->drawPlot();
}

void ParserAdaptor::stopDrawing()
{
    // handle method call org.kde.kmplot.Parser.stopDrawing
    QMetaObject::invokeMethod(parent(), "stopDrawing");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->stopDrawing();
}


#include "parseradaptor.moc"
