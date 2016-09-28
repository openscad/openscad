#pragma once

#include <QString>
#include <QImage>

class AbstractVideo
{
public:
    virtual ~AbstractVideo() { }

    virtual void open(const QString fileName) = 0;
    virtual void close() = 0;
    virtual void exportFrame(const QImage frame, const double s, const double t) = 0;
};
