#pragma once

#include "video.h"

class PngVideo : public AbstractVideo
{
private:
    int width, height;

public:
    PngVideo(const int width, const int height);
    virtual ~PngVideo();

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
