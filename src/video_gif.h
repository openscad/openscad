#pragma once

#include <gif_lib.h>

#include "video.h"

class GifVideoExport : public AbstractVideoExport
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
    void collect_colormap(const QImage &frame);
    unsigned int find_hchange(const unsigned char *b1, const unsigned char *b2, const int start, const int end);
    unsigned int find_vchange(const unsigned char *b1, const unsigned char *b2, const unsigned int miny, const unsigned int maxy, const int start, const int end);
    bool flush_buffer(unsigned char *buf, unsigned int delay, unsigned int minx, unsigned int maxx, unsigned int miny, unsigned int maxy);

public:
    GifVideoExport(const unsigned int width = 0, unsigned const int height = 0);
    virtual ~GifVideoExport();

    virtual QString name() const;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const;

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
