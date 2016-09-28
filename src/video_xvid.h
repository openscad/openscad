#pragma once

#include <xvid.h>

#include "video.h"

class XvidVideo : public AbstractVideo
{
private:
    bool init_ok;
    int width, height;

    FILE *f;
    unsigned char *buf;
    xvid_gbl_init_t      _gbl_init;
    xvid_enc_create_t    _enc_create;
    xvid_plugin_single_t _plugin_single;
    xvid_enc_plugin_t    _plugins[1];
    
public:
    XvidVideo(const int width, const int height);
    virtual ~XvidVideo();
    
    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
