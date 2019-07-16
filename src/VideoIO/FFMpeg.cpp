#include <VideoIO/FFMpeg.h>

extern "C" {
#   include <libavcodec/avcodec.h>
#   include <libavformat/avformat.h>
#   include <libavutil/avutil.h>
}

namespace video_io
{

namespace
{

int FFMpegLockManager(void **mutex, AVLockOp op)
{
    if (!mutex)
        return -1;

    try {
        switch (op)
        {
        case AV_LOCK_CREATE:
            *mutex = 0;
            *mutex = new cv::Mutex;
            break;
        case AV_LOCK_OBTAIN:
            reinterpret_cast<cv::Mutex *>(*mutex)->lock();
            break;
        case AV_LOCK_RELEASE:
            reinterpret_cast<cv::Mutex *>(*mutex)->unlock();
            break;
        case AV_LOCK_DESTROY:
            delete reinterpret_cast<cv::Mutex *>(*mutex);
            *mutex = 0;
            break;
        }
    }
    catch(...)
    {
        return -1;
    }

    return 0;
}

}

cv::Mutex FFMpeg::_mutex;
volatile int FFMpeg::_initialized = 0;

void FFMpeg::init()
{
    if (_initialized)
        return;

    cv::AutoLock lock(_mutex);

    if (!_initialized)
    {
        av_register_all();
        avformat_network_init();
        av_lockmgr_register(&FFMpegLockManager);
        av_log_set_level(AV_LOG_ERROR);

        instance()._initialized = 1;
    }
}

void FFMpeg::setInitialized(bool flag)
{
    assert(_initialized <= 0);

    _initialized = flag ? -1 : 0;
}

FFMpeg::FFMpeg()
{
}

FFMpeg::~FFMpeg()
{
    if (_initialized > 0)
    {
        av_lockmgr_register(0);
        avformat_network_deinit();
    }
}

FFMpeg &FFMpeg::instance()
{
    static FFMpeg theFFMpeg;

    return theFFMpeg;
}

}
