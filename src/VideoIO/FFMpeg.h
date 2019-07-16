#ifndef __VIDEO_IO__FFMPEG_H
#define __VIDEO_IO__FFMPEG_H

#include <VideoIO/Defs.h>
#include <opencv2/core/core.hpp>

#include <VideoIO/AnnoyingWarningsOff.h>

namespace video_io
{

class VIDEO_IO_API FFMpeg
{
public:
    static void init();

    // Если клиентский код инициализирует ffmpeg самостоятельно, то до использования
    // VideoIO необходимо вызвать FFMpeg::setInitialized().
    static void setInitialized(bool flag = true);

private:
    FFMpeg();

    ~FFMpeg();

    static FFMpeg &instance();

    static volatile int _initialized;
    static cv::Mutex _mutex;
};

}

#include <VideoIO/AnnoyingWarningsOn.h>

#endif
