#pragma once

#include <QString>
#include <QImage>
#include <QVector>
#include <QStringList>
#include <boost/shared_ptr.hpp>

class AbstractVideoExport
{
public:
    virtual ~AbstractVideoExport() { }

    virtual QString name() const = 0;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const = 0;

    virtual void open(const QString filename) = 0;
    virtual void close() = 0;
    virtual void exportFrame(const QImage frame, const double s, const double t) = 0;
};

class Video
{
private:
    QVector<boost::shared_ptr<AbstractVideoExport> > exporters;

public:
    Video();
    virtual ~Video();

    const QStringList getExporterNames();
    AbstractVideoExport * getExporter(unsigned int idx, unsigned int width, unsigned int height);
};