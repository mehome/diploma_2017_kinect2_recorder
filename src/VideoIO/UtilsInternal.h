#ifndef __VIDEO_IO__UTILS_INTERNAL_H
#define __VIDEO_IO__UTILS_INTERNAL_H

#include <VideoIO/Utils.h>
#include <VideoIO/FFMpeg.h>

extern "C" {
#   include <libavcodec/avcodec.h>
#   include <libavformat/avformat.h>
#   include <libavutil/avutil.h>
#   include <libavutil/opt.h>
#   include <libavutil/pixdesc.h>
#   include <libswscale/swscale.h>
}

namespace video_io
{

int elemType(AVPixelFormat pixFmt);

int interpToSWSFlag(int interp);

// flags - флаги sws_getCachedContext().
AVFrame *convertImage(AVFrame *src, AVFrame **dst, AVPixelFormat dstPixFmt, int dstWidth,
                      int dstHeight, unsigned char **dstBuf, std::ptrdiff_t *dstBufSize,
                      int flags);

}

#endif
