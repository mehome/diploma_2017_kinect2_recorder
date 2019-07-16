#ifndef __VIDEO_IO__UTILS_H
#define __VIDEO_IO__UTILS_H

#include <VideoIO/Defs.h>

namespace video_io
{

// Поддерживаемые форматы пикселей cv::Mat или пустая строка.
VIDEO_IO_API char const *pixelFormat(int elemType);

// Время (s) с начала выполнения программы.
VIDEO_IO_API double getTime();

VIDEO_IO_API void sleep(double s);

}

#endif
