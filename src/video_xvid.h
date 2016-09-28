#pragma once

#include <xvid.h>

#include "video.h"

class XvidVideoExport : public AbstractVideoExport
{
private:
    bool init_ok;
    unsigned int width, height;

    FILE *f;
    unsigned char *buf;
    xvid_gbl_init_t      _gbl_init;
    xvid_enc_create_t    _enc_create;
    xvid_plugin_single_t _plugin_single;
    xvid_enc_plugin_t    _plugins[1];
    
public:
    XvidVideoExport(const unsigned int width = 0, const unsigned int height = 0);
    virtual ~XvidVideoExport();
    
    virtual QString name() const;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const;

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
