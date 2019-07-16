#include <VideoIO/UtilsInternal.h>
#include <opencv2/core/core.hpp>

extern "C" {
#   include <libavutil/time.h>
}

namespace video_io
{

VIDEO_IO_API char const *pixelFormat(int elemType)
{
    if (elemType == CV_8UC1)
        return av_get_pix_fmt_name(AV_PIX_FMT_GRAY8);

    if (elemType == CV_16UC1)
        return av_get_pix_fmt_name(AV_PIX_FMT_GRAY16LE);

    if (elemType == CV_8UC3)
        return av_get_pix_fmt_name(AV_PIX_FMT_BGR24);

    if (elemType == CV_16UC3)
        return av_get_pix_fmt_name(AV_PIX_FMT_BGR48);

    // MENKIN FIX
    //if (elemType == CV_8UC4)
        return av_get_pix_fmt_name(AV_PIX_FMT_YUV420P);

    //return "";
}

VIDEO_IO_API double getTime()
{
    return 1.0e-6 * av_gettime();
}

VIDEO_IO_API void sleep(double s)
{
    if (s > 0)
    {
        int64_t nbUsleeps = static_cast<int64_t>((1.0e6 / 999999) * s);

        for (int64_t n = 0; n < nbUsleeps; ++n)
            av_usleep(999999);

        unsigned us = static_cast<unsigned>(1.0e6 * s - 999999.0 * nbUsleeps + 0.5);
        av_usleep(us > 999999 ? 999999 : us);
    }
}

int elemType(AVPixelFormat pixFmt)
{
    if (pixFmt == AV_PIX_FMT_GRAY8)
        return CV_8UC1;

    if (pixFmt == AV_PIX_FMT_GRAY16)
        return CV_16UC1;

    if (pixFmt == AV_PIX_FMT_BGR24)
        return CV_8UC3;

    if (pixFmt == AV_PIX_FMT_BGR48)
        return CV_16UC3;

    return 0;
}

int interpToSWSFlag(int interp)
{
    int ret = 0;

    switch (interp)
    {
    case INTERP_FAST_BILINEAR: ret = SWS_FAST_BILINEAR; break;
    case INTERP_BILINEAR     : ret = SWS_BILINEAR;      break;
    case INTERP_BICUBIC      : ret = SWS_BICUBIC;       break;
    case INTERP_X            : ret = SWS_X;             break;
    case INTERP_POINT        : ret = SWS_POINT;         break;
    case INTERP_AREA         : ret = SWS_AREA;          break;
    case INTERP_BICUBLIN     : ret = SWS_BICUBLIN;      break;
    case INTERP_GAUSS        : ret = SWS_GAUSS;         break;
    case INTERP_SINC         : ret = SWS_SINC;          break;
    case INTERP_LANCZOS      : ret = SWS_LANCZOS;       break;
    case INTERP_SPLINE       : ret = SWS_SPLINE;        break;
    }

    return ret;
}

AVFrame *convertImage(AVFrame *src, AVFrame **dst, AVPixelFormat dstPixFmt, int dstWidth,
                      int dstHeight, unsigned char **dstBuf, std::ptrdiff_t *dstBufSize,
                      int flags)
{
    assert(src);
    assert(dst);
    assert(dstBuf);
    assert(dstBufSize);

    AVPixelFormat srcPixFmt = static_cast<AVPixelFormat>(src->format);

    if (srcPixFmt < 0)
        throw Error(ERR_CONVERT_IMAGE, "invalid source pixel format");

    if (src->width <= 0 || (src->width & 1) || src->height <= 0 || (src->height & 1))
        throw Error(ERR_CONVERT_IMAGE, "invalid source image size");

    if (dstPixFmt < 0)
        throw Error(ERR_CONVERT_IMAGE, "invalid destination pixel format");

    if (dstWidth <= 0 || dstHeight <= 0)
        throw Error(ERR_CONVERT_IMAGE, "invalid destination image size");

    if (srcPixFmt == dstPixFmt && src->width == dstWidth && src->height == dstHeight)
        return src;

    if (!*dst)
        if (!(*dst = av_frame_alloc()))
            throw std::bad_alloc();

    std::ptrdiff_t bufSize = avpicture_get_size(dstPixFmt, dstWidth, dstHeight);
    if (bufSize < 0)
        throw Error(ERR_CONVERT_IMAGE, "failed to compute the destination buffer size");

    if (!*dstBuf || *dstBufSize < bufSize)
    {
        if (!(*dstBuf = static_cast<unsigned char *>(av_realloc(*dstBuf, bufSize))))
            throw std::bad_alloc();

        *dstBufSize = bufSize;
    }

    avpicture_fill(reinterpret_cast<AVPicture *>(*dst), *dstBuf, dstPixFmt, dstWidth, dstHeight);


    SwsContext  *convertCtx = sws_getCachedContext(0, src->width, src->height, srcPixFmt,
                                                  dstWidth, dstHeight, dstPixFmt, flags, 0, 0, 0);

    if (!convertCtx)
        throw Error(ERR_CONVERT_IMAGE, "failed to initialize the image conversion context");

    int err = sws_scale(convertCtx, src->data, src->linesize, 0, src->height,
                        (*dst)->data, (*dst)->linesize);

    sws_freeContext(convertCtx);
    if (err < 0)
        throw Error(ERR_CONVERT_IMAGE, "image conversion failed");

    return *dst;
}

}
