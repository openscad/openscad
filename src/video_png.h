#pragma once

#include "video.h"

class PngVideoExport : public AbstractVideoExport
{
private:
    unsigned int width, height;
    QString filename;

public:
    PngVideoExport(const unsigned int width = 0, const unsigned int height = 0);
    virtual ~PngVideoExport();

    virtual QString name() const;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const;

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
