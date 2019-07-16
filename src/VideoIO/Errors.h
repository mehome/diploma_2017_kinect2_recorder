#ifndef __VIDEO_IO__ERRORS_H
#define __VIDEO_IO__ERRORS_H

#include <VideoIO/Defs.h>
#include <assert.h>
#include <new> /* std::bad_alloc */
#include <stdexcept>

#include <VideoIO/AnnoyingWarningsOff.h>

namespace video_io
{

enum
{
    ERR_OPEN_FILE        = 1,
    ERR_FIND_STREAM_INFO = 2,
    ERR_OPEN_CODEC       = 3,
    ERR_RW_FRAME         = 4,
    ERR_ENC_DEC_VIDEO    = 5,
    ERR_CONVERT_IMAGE    = 6,
    ERR_GUESS_FORMAT     = 7,
    ERR_FIND_CODEC       = 8,
    ERR_FIND_PIX_FMT     = 9,
    ERR_IMAGE_TYPE       = 10,
    ERR_IMAGE_SIZE       = 11,
    ERR_BAD_PARAM        = 12,
    ERR_SET_OPTIONS      = 13,
    ERR_SET_METADATA     = 14,
    ERR_WRITE_HEADER     = 15,
    ERR_WRITE_TRAILER    = 16
};

class VIDEO_IO_API Error:
        public std::runtime_error
{
public:
    explicit Error(int code) throw():
        std::runtime_error("VideoIO error"), _code(code)
    {
    }

    Error(int code, std::string const &what) throw():
        std::runtime_error(what), _code(code)
    {
    }

    virtual ~Error() throw()
    {
    }

    int code() const throw()
    {
        return _code;
    }

private:
    int _code;
};

#define DERIVED_VIDEO_IO_ERROR(type_name, default_what) \
    class VIDEO_IO_API type_name: \
            public video_io::Error \
    { \
    public: \
        explicit type_name(int code) throw(): \
            video_io::Error(code, default_what) {} \
        type_name(int code, std::string const &what) throw(): \
            video_io::Error(code, what) {} \
        virtual ~type_name() throw() {} \
    };

}

#include <VideoIO/AnnoyingWarningsOn.h>

#endif
