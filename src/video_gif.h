#pragma once

#include <gif_lib.h>

#include "video.h"

class GifVideo : public AbstractVideo
{
private:
    enum { MAX_FRAME_DELAY = 65535 };
    typedef enum { STATE_INIT, STATE_COLORMAP, STATE_OUTPUT, STATE_END } state_t;

    state_t state;
    unsigned int width, height;
    unsigned int frame_delay;

    ColorMapObject *cmap;
    QVector<QRgb> export_color_map;

    GifFileType *gif_handle;
    unsigned char *buf;

private:
    bool flush_buffer(unsigned char *buf, unsigned int delay);

public:
    GifVideo(const int width, const int height);
    virtual ~GifVideo();

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
