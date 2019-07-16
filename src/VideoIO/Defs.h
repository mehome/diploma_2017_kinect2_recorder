#ifndef __VIDEO_IO__DEFS_H
#define __VIDEO_IO__DEFS_H

#include <cstddef> /* std::ptrdiff_t, std::size_t */
#include <map>
#include <string>

namespace video_io
{

#if defined(_WIN32)
#   define VIDEO_IO_DECLSPEC_EXPORT __declspec(dllexport)
#   define VIDEO_IO_DECLSPEC_IMPORT __declspec()
#else
#   if __GNUC__ >= 4
#       define VIDEO_IO_DECLSPEC_EXPORT __attribute__ ((visibility("default")))
#       define VIDEO_IO_DECLSPEC_IMPORT __attribute__ ((visibility("default")))
#   else
#       define VIDEO_IO_DECLSPEC_EXPORT
#       define VIDEO_IO_DECLSPEC_IMPORT
#   endif
#endif

#if defined(VIDEO_IO_STATIC)
#   define VIDEO_IO_API
#else
#   if defined(VIDEO_IO_EXPORTS)
#       define VIDEO_IO_API VIDEO_IO_DECLSPEC_EXPORT
#   else
#       define VIDEO_IO_API VIDEO_IO_DECLSPEC_IMPORT
#   endif
#endif

typedef std::map<std::string, std::string> Metadata;

enum
{
    STS_EOF    = -1,
    STS_EAGAIN = -2
};

enum
{
    INTERP_FAST_BILINEAR = 1,
    INTERP_BILINEAR      = 2,
    INTERP_BICUBIC       = 3,
    INTERP_X             = 4,
    INTERP_POINT         = 5,
    INTERP_AREA          = 6,
    INTERP_BICUBLIN      = 7,
    INTERP_GAUSS         = 8,
    INTERP_SINC          = 9,
    INTERP_LANCZOS       = 10,
    INTERP_SPLINE        = 11
};

}

#include <VideoIO/Errors.h>

#endif
