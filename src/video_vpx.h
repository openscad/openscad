#pragma once

#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

#include "video.h"
#include "EbmlWriter.h"

class VpxVideoExport : public AbstractVideoExport
{
private:
    bool init_ok;
    unsigned int width, height;
    int frame_cnt;
    EbmlGlobal ebml;
    vpx_codec_ctx_t      codec;
    vpx_codec_enc_cfg_t  cfg;
    vpx_image_t          raw;
    vpx_codec_err_t      res;

public:
    VpxVideoExport(const unsigned int width = 0, const unsigned int height = 0);
    virtual ~VpxVideoExport();

    virtual QString name() const;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const;

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
