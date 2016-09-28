#include "video.h"

#include "video_png.h"

#ifdef ENABLE_VIDEO_GIF
#include "video_gif.h"
#endif

#ifdef ENABLE_VIDEO_VPX
#include "video_vpx.h"
#endif

#ifdef ENABLE_VIDEO_XVID
#include "video_xvid.h"
#endif

Video::Video()
{
	exporters << boost::shared_ptr<AbstractVideoExport>(new PngVideoExport());

#ifdef ENABLE_VIDEO_GIF
	exporters << boost::shared_ptr<AbstractVideoExport>(new GifVideoExport());
#endif

#ifdef ENABLE_VIDEO_VPX
	exporters << boost::shared_ptr<AbstractVideoExport>(new VpxVideoExport());
#endif

#ifdef ENABLE_VIDEO_XVID
	exporters << boost::shared_ptr<AbstractVideoExport>(new XvidVideoExport());
#endif
}

Video::~Video()
{
}

const QStringList Video::getExporterNames()
{
	QStringList names;
	for (int a = 0;a < exporters.size();a++) {
		QString name = exporters[a].get()->name();
		names << name;
	}
	return names;
}

AbstractVideoExport * Video::getExporter(unsigned int idx, unsigned int width, unsigned int height)
{
	AbstractVideoExport * exporterTemplate = exporters[idx].get();
	return exporterTemplate->create(width, height);
}
