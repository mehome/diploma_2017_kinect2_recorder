#include <VideoIO/VideoWriter.h>
#include <VideoIO/UtilsInternal.h>
#include <algorithm>
#include <limits>
#include <vector>
// DEBUG
#include <iostream>

namespace video_io
{

VideoWriter::VideoStreamParams::VideoStreamParams():
    codecName("ffv1"),
    frameRate(0),
    findBestPixelFormat(false),
    width(0),
    height(0),
    aspectRatio(-1),
    bitRate(-1),
    bitRateTolerance(-1),
    gopSize(-1),
    maxBFrames(-1)
{
}

namespace {

class VideoWriterImpl
{
public:
    typedef VideoWriter::Error Error;
    typedef VideoWriter::VideoStreamParams VideoStreamParams;

    VideoWriterImpl():
        _nbThreads(-1),
        _formatContext(0),
        _dstFrame(0),
        _dstFrameBuf(0),
        _dstFrameBufSize(0),
        _failed(false)
    {
    }

    // Перехватывает исключение, генерируемое close().
    ~VideoWriterImpl()
    {
        try
        {
            close();
        }
        catch(...)
        {
        }
    }

    void open(std::string const &fileName, int nbThreads)
    {
        try
        {
            FFMpeg::init();
            close();

            int err = avformat_alloc_output_context2(&_formatContext, 0, 0, fileName.c_str());
            if (err == AVERROR(ENOMEM))
                throw std::bad_alloc();
            if (err < 0)
                throw Error(ERR_GUESS_FORMAT,
                            "could not deduce output format from the output file name \"" + fileName + "\"");

            // HACK to avoid MPEG-PS muxer to spit many underflow errors
            // (see ffmpeg's ffserver.c and ffmpeg_opt.c).
            _formatContext->max_delay = static_cast<int>(0.7 * AV_TIME_BASE);

            _nbThreads = nbThreads;

            // Open the output file, if needed.
            if (!(_formatContext->oformat->flags & AVFMT_NOFILE))
            {
                err = avio_open(&_formatContext->pb, fileName.c_str(), AVIO_FLAG_WRITE);
                if (err == AVERROR(ENOMEM))
                    throw std::bad_alloc();
                if (err < 0)
                    throw Error(ERR_OPEN_FILE,
                                "could not open output file \"" + fileName + "\"");
            }
        }
        catch (...)
        {
            _failed = true;
            throw;
        }
    }

    void setMetadata(Metadata const &metadata)
    {
        assert(_formatContext);

        for (Metadata::const_iterator it = metadata.begin(); it != metadata.end(); ++it)
        {
            int err = av_dict_set(&_formatContext->metadata, it->first.c_str(),
                                  it->second.c_str(), 0);
            if (err == AVERROR(ENOMEM))
                throw std::bad_alloc();
            if (err < 0)
                throw Error(ERR_SET_METADATA, "failed to set metadata");
        }
    }

    int addVideoStream(VideoStreamParams const &params)
    {
        assert(_formatContext);
        assert(_srcFrames.empty()); // Header еще не записан?

        int streamId;

        try
        {
            if (params.frameRate <= 0)
                throw Error(ERR_BAD_PARAM, "invalid frame rate");

            if (params.width <= 0 || (params.width & 1))
                throw Error(ERR_IMAGE_SIZE, "invalid image width");

            if (params.height <= 0 || (params.height & 1))
                throw Error(ERR_IMAGE_SIZE, "invalid image height");

            if (params.aspectRatio > 10)
                throw Error(ERR_BAD_PARAM, "invalid aspect ratio");

            // Поиск кодека.

            AVCodecID codecId = AV_CODEC_ID_NONE;
            AVCodec *codec = 0;

            if (params.codecName.empty())
                codecId = _formatContext->oformat->video_codec;

            if (params.codecName.size() == 4)
            {
                // Возможно задан RIFF codec tag.
                char const *s = params.codecName.c_str();
                unsigned codecTag = MKTAG(s[0], s[1], s[2], s[3]);
                AVCodecTag const *riffVideoTags = avformat_get_riff_video_tags();

                codecId = av_codec_get_id(&riffVideoTags, codecTag);
            }

            if (codecId == AV_CODEC_ID_NONE && params.codecName.size() > 0)
            {
                // Поиск по имени кодека.
                codec = avcodec_find_encoder_by_name(params.codecName.c_str());

                if (codec)
                    codecId = codec->id;
            }

            if (codecId == AV_CODEC_ID_NONE)
                throw Error(ERR_FIND_CODEC,
                            "could not find video encoder for the input codec name \"" + params.codecName + "\"");

            if (!codec)
                codec = avcodec_find_encoder(codecId);

            if (!codec || codec->type != AVMEDIA_TYPE_VIDEO)
                throw Error(ERR_FIND_CODEC,
                            "failed to find video encoder \"" + params.codecName + "\"");

            // Выбор формата пикселя.

            AVPixelFormat userPixFmt = AV_PIX_FMT_NONE;

            if (!params.pixelFormat.empty())
                userPixFmt = av_get_pix_fmt(params.pixelFormat.c_str());

            if (userPixFmt == AV_PIX_FMT_NONE)
                throw Error(ERR_FIND_PIX_FMT,
                            "invalid pixel format \"" + params.pixelFormat + "\"");

            AVPixelFormat pixFmt = userPixFmt;

            if (codec->pix_fmts) // Список поддерживаемых форматов пикселей не пуст?
            {
                if (params.findBestPixelFormat)
                {
                    AVPixelFormat *pixFmts = const_cast<AVPixelFormat *>(codec->pix_fmts);
                    pixFmt = avcodec_find_best_pix_fmt_of_list(pixFmts, userPixFmt,
                                                               /* has_alpha */ 0, 0);
                    if (pixFmt < 0)
                        throw Error(ERR_FIND_PIX_FMT,
                                    "failed to find best supported pixel format for the input pixel format \"" +
                                    params.pixelFormat + "\"");
                }
                else
                {
                    AVPixelFormat const *it;

                    for (it = codec->pix_fmts; *it != -1; ++it)
                    {


                        if (*it == userPixFmt)
                        {

                            break;
                        }
                    }

                    if (*it == -1)
                        throw Error(ERR_FIND_PIX_FMT,
                                    "the input pixel format \"" + params.pixelFormat +
                                    "\" is not supported by the encoder \"" +
                                    params.codecName + "\"");
                }
            }

            // Создание нового потока.

            AVStream *stream = avformat_new_stream(_formatContext, codec);
            if (!stream)
                throw std::bad_alloc();

            streamId = stream->id = _formatContext->nb_streams - 1;

            // Инициализация контекста кодека.

            AVCodecContext *codecCtx = stream->codec;
            AVRational qFrameRate = av_d2q(params.frameRate, std::numeric_limits<int>::max());

            if (!qFrameRate.num)
                qFrameRate.num = 1;

            codecCtx->thread_count = _nbThreads < 0 ? cv::getNumberOfCPUs() + 1 : _nbThreads;
            codecCtx->codec_id = codecId;
            codecCtx->time_base = av_inv_q(qFrameRate);
            codecCtx->pix_fmt = pixFmt;
            codecCtx->width = params.width;
            codecCtx->height = params.height;
            if (params.aspectRatio >= 0)
                codecCtx->sample_aspect_ratio = av_d2q(params.aspectRatio, 255);
            if (params.bitRate >= 0)
                codecCtx->bit_rate = params.bitRate;
            if (params.bitRateTolerance >= 0)
                codecCtx->bit_rate_tolerance = params.bitRateTolerance;
            if (params.gopSize >= 0)
                codecCtx->gop_size = params.gopSize;
            if (params.maxBFrames >= 0)
                codecCtx->max_b_frames = params.maxBFrames;

            if (codecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO ||
                    codecCtx->codec_id == AV_CODEC_ID_MSMPEG4V3)
            {
                // Needed to avoid using macroblocks in which some coeffs overflow.
                // This does not happen with normal video, it just happens here as the motion
                // of the chroma plane does not match the luma plane.
                codecCtx->mb_decision = FF_MB_DECISION_RD;
            }

            if (!params.options.empty())
            {
                int err = av_set_options_string(codecCtx, params.options.c_str(), "=", ":");
                if (err == AVERROR(ENOMEM))
                    throw std::bad_alloc();
                if (err < 0)
                    throw Error(ERR_SET_OPTIONS,
                                "failed to set options from the input options string \"" +
                                params.options + "\"");
            }

            // Some formats want stream headers to be separate.
            if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
                codecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

            // Open the codec.

            int err = avcodec_open2(codecCtx, codec, 0);
            if (err == AVERROR(ENOMEM))
                throw std::bad_alloc();
            if (err < 0)
                throw Error(ERR_OPEN_CODEC, "failed to open encoder");
        }
        catch (...)
        {
            _failed = true;
            throw;
        }

        return streamId;
    }

    int nbStreams() const
    {
        assert(_formatContext);

        return _formatContext->nb_streams;
    }

    int nbThreads(int id) const
    {
        return codecContext(id)->thread_count;
    }

    std::string codecDescription(int id) const
    {
        char buf[256] = {0};

        avcodec_string(buf, sizeof(buf), codecContext(id), true);

        return buf;
    }

    char const *codecName(int id) const
    {
        return avcodec_descriptor_get(codecContext(id)->codec_id)->name;
    }

    std::string fourCC(int id) const
    {
        AVCodecTag const *riffVideoTags = avformat_get_riff_video_tags();
        int codecTag = av_codec_get_tag(&riffVideoTags, codecContext(id)->codec_id);

        char buf[32] = {0};

        if (codecTag > 0)
            av_get_codec_tag_string(buf, sizeof(buf), codecTag);

        return buf;
    }

    double frameRate(int id) const
    {
        return 1.0 / av_q2d(codecContext(id)->time_base);
    }

    char const *pixelFormat(int id) const
    {
        return av_get_pix_fmt_name(codecContext(id)->pix_fmt);
    }

    int width(int id) const
    {
        return codecContext(id)->width;
    }

    int height(int id) const
    {
        return codecContext(id)->height;
    }

    double aspectRatio(int id) const
    {
        return av_q2d(codecContext(id)->sample_aspect_ratio);
    }

    int bitRate(int id) const
    {
        return codecContext(id)->bit_rate;
    }

    int bitRateTolerance(int id) const
    {
        return codecContext(id)->bit_rate_tolerance;
    }

    int gopSize(int id) const
    {
        return codecContext(id)->gop_size;
    }

    int maxBFrames(int id) const
    {
        return codecContext(id)->max_b_frames;
    }

    void write(cv::Mat &image, int id)
    {
        try
        {
            AVCodecContext *codecCtx = codecContext(id);
            int width = codecCtx->width;
            int height = codecCtx->height;

            if (width != image.size().width || height != image.size().height)
                throw Error(ERR_IMAGE_SIZE, "inconsistent image size");

            AVPixelFormat srcPixFmt = av_get_pix_fmt(video_io::pixelFormat(image.type()));
            AVPixelFormat dstPixFmt = codecCtx->pix_fmt;

            if (srcPixFmt == AV_PIX_FMT_NONE)
                throw Error(ERR_IMAGE_TYPE, "unsupported image type");

            if (_srcFrames.empty()) // Header еще не записан?
            {
                // Write the stream header, if any.
                int err = avformat_write_header(_formatContext, 0);
                if (err == AVERROR(ENOMEM))
                    throw std::bad_alloc();
                if (err < 0)
                    throw Error(ERR_WRITE_HEADER, "failed to write video file header");

                _srcFrames.assign(nbStreams(), static_cast<AVFrame *>(0));
            }

            if (!_srcFrames[id])
                if (!(_srcFrames[id] = av_frame_alloc()))
                    throw std::bad_alloc();

            if (!_srcFrames[id]->data[0])
            {
                if (avpicture_alloc(reinterpret_cast<AVPicture *>(_srcFrames[id]), srcPixFmt,
                                    width, height) < 0)
                    throw std::bad_alloc();

                _srcFrames[id]->format = srcPixFmt;
                _srcFrames[id]->width = width;
                _srcFrames[id]->height = height;
            }

            AVFrame *srcFrame = _srcFrames[id];

            if (image.data != srcFrame->data[0]) // Равны, когда выполняется flush кодеков.
                image.copyTo(cv::Mat(height, width, image.type(), srcFrame->data[0],
                             srcFrame->linesize[0]));

            AVFrame *frame = srcFrame;

            if (dstPixFmt != srcPixFmt)
            {
                try
                {
                    convertImage(srcFrame, &_dstFrame, dstPixFmt, width, height, &_dstFrameBuf,
                                 &_dstFrameBufSize, SWS_ACCURATE_RND | SWS_LANCZOS);
                }
                catch (video_io::Error &e)
                {
                    throw Error(e.code(), e.what());
                }

                frame = _dstFrame;
            }

            if (_inputFrameNumbers.empty())
                _inputFrameNumbers.assign(nbStreams(), 0);

            if (_outputFrameNumbers.empty())
                _outputFrameNumbers.assign(nbStreams(), 0);

            if (_timestamps.empty())
                _timestamps.assign(nbStreams(), 0);

            frame->pts = _timestamps[id];
            _inputFrameNumbers[id]++;
            _timestamps[id] += av_rescale_q(1, codecCtx->time_base, stream(id)->time_base);

            AVPacket pkt;
            int err = 0;

            av_init_packet(&pkt);

            if (_formatContext->oformat->flags & AVFMT_RAWPICTURE &&
                    codecCtx->codec->id == AV_CODEC_ID_RAWVIDEO)
            {
                // Raw pictures are written as AVPicture structure to avoid any copies.
                pkt.flags |= AV_PKT_FLAG_KEY;
                pkt.data = reinterpret_cast<uint8_t *>(frame);
                pkt.size = sizeof(AVPicture);
                pkt.stream_index = stream(id)->index;
                err = av_interleaved_write_frame(_formatContext, &pkt);
                _outputFrameNumbers[id]++;
            }
            else
            {
                int gotPacket;

                pkt.data = 0;
                pkt.size = 0;
                err = avcodec_encode_video2(codecCtx, &pkt, frame, &gotPacket);

                if (err < 0)
                {
                    av_free_packet(&pkt);

                    if (err == AVERROR(ENOMEM))
                        throw std::bad_alloc();

                    throw Error(ERR_ENC_DEC_VIDEO, "failed to encode video frame packet");
                }

                if (gotPacket)
                {
                    if (codecCtx->coded_frame->key_frame)
                        pkt.flags |= AV_PKT_FLAG_KEY;

                    pkt.stream_index = stream(id)->index;
                    err = av_interleaved_write_frame(_formatContext, &pkt);
                    _outputFrameNumbers[id]++;
                }

                av_free_packet(&pkt);
            }

            if (err == AVERROR(ENOMEM))
                throw std::bad_alloc();
            if (err < 0)
                throw Error(ERR_RW_FRAME, "failed to write frame");
        }
        catch (...)
        {
            _failed = true;
            throw;
        }
    }

    long long frameNumber(int id) const
    {
        assert(id >= 0);
        assert(id < nbStreams());

        return _inputFrameNumbers[id];
    }

    double timestamp(int id) const
    {
        assert(id >= 0);
        assert(id < nbStreams());

        return _timestamps[id] * av_q2d(stream(id)->time_base);
    }

    void close()
    {
        try
        {
            flushEncoders();
        }
        catch (...)
        {
            clear();
            throw;
        }

        clear();
    }

private:
    AVStream *stream(int id) const
    {
        assert(_formatContext);
        assert(id >= 0);
        assert(id < nbStreams());

        return _formatContext->streams[id];
    }

    AVCodecContext *codecContext(int id) const
    {
        return stream(id)->codec;
    }

    void flushEncoders()
    {
        try
        {
            if (!_formatContext || _failed || _srcFrames.empty())
                return;

            assert(_srcFrames.size() == nbStreams());
            assert(_inputFrameNumbers.size() == _srcFrames.size());
            assert(_outputFrameNumbers.size() == _srcFrames.size());

            std::vector<long long> targetFrameNumbers(_inputFrameNumbers);

            while (true)
            {
                int i = 0;

                for (; i < nbStreams(); ++i)
                    if (_outputFrameNumbers[i] < targetFrameNumbers[i])
                    {
                        cv::Mat image(codecContext(i)->height, codecContext(i)->width,
                                      elemType(static_cast<AVPixelFormat>(_srcFrames[i]->format)),
                                      _srcFrames[i]->data[0], _srcFrames[i]->linesize[0]);

                        write(image, i);
                        break;
                    }

                if (i == nbStreams())
                    break;
            }

            // !_srcFrames.empty(), следовательно, заголовок записан. Write the trailer, if any.
            // The trailer must be written before you close the CodecContexts open when you
            // wrote the header; otherwise av_write_trailer() may try to use memory that was
            // freed on av_codec_close().

            int err = av_write_trailer(_formatContext);

            if (err == AVERROR(ENOMEM))
                throw std::bad_alloc();
            if (err < 0)
                throw Error(ERR_WRITE_TRAILER, "failed to write video file trailer");
        }
        catch (...)
        {
            _failed = true;
            throw;
        }
    }

    void clear()
    {
        if (!_formatContext)
            return;

      /*  if (err == AVERROR(ENOMEM))
            throw std::bad_alloc();
        if (err < 0)
            throw Error(ERR_SET_METADATA, "failed to set metadata");*/


        av_dict_free(&_formatContext->metadata);

        for (int i = 0; i < nbStreams(); ++i)
            avcodec_close(codecContext(i));

        if (_formatContext->oformat && !(_formatContext->oformat->flags & AVFMT_NOFILE))
            if (_formatContext->pb)
            {
                avio_close(_formatContext->pb);
                _formatContext->pb = 0;
            }

        avformat_free_context(_formatContext);
        _formatContext = 0;

        for (int i = 0; i < _srcFrames.size(); ++i)
            if (_srcFrames[i])
            {
                if (_srcFrames[i]->data[0])
                    av_free(_srcFrames[i]->data[0]);
                av_frame_free(&_srcFrames[i]);
            }

        _srcFrames.clear();

        if (_dstFrame)
        {
            av_frame_free(&_dstFrame);
            _dstFrame = 0;
        }

        if (_dstFrameBuf)
        {
            av_free(_dstFrameBuf);
            _dstFrameBuf = 0;
        }

        _dstFrameBufSize = 0;


        _inputFrameNumbers.clear();
        _outputFrameNumbers.clear();

        _failed = false;
    }

    int _nbThreads;
    AVFormatContext *_formatContext;
    std::vector<AVFrame *> _srcFrames;
    AVFrame *_dstFrame;
    unsigned char *_dstFrameBuf;
    std::ptrdiff_t _dstFrameBufSize;
    std::vector<long long> _inputFrameNumbers;
    std::vector<long long> _outputFrameNumbers;
    std::vector<int64_t> _timestamps;
    bool _failed;
};

VideoWriterImpl *videoWriterImpl(void *impl)
{
    return static_cast<VideoWriterImpl *>(impl);
}

}

VideoWriter::VideoWriter():
    _impl(new VideoWriterImpl)
{
}

VideoWriter::~VideoWriter()
{
    delete videoWriterImpl(_impl);
}

void VideoWriter::open(std::string const &fileName, int nbThreads)
{
    return videoWriterImpl(_impl)->open(fileName, nbThreads);
}

void VideoWriter::setMetadata(Metadata const &metadata)
{
    return videoWriterImpl(_impl)->setMetadata(metadata);
}

int VideoWriter::addVideoStream(VideoStreamParams const &params)
{
    return videoWriterImpl(_impl)->addVideoStream(params);
}

int VideoWriter::nbStreams() const
{
    return videoWriterImpl(_impl)->nbStreams();
}

int VideoWriter::nbThreads(int id) const
{
    return videoWriterImpl(_impl)->nbThreads(id);
}

std::string VideoWriter::codecDescription(int id) const
{
    return videoWriterImpl(_impl)->codecDescription(id);
}

char const *VideoWriter::codecName(int id) const
{
    return videoWriterImpl(_impl)->codecName(id);
}

std::string VideoWriter::fourCC(int id) const
{
    return videoWriterImpl(_impl)->fourCC(id);
}

double VideoWriter::frameRate(int id) const
{
    return videoWriterImpl(_impl)->frameRate(id);
}

char const *VideoWriter::pixelFormat(int id) const
{
    return videoWriterImpl(_impl)->pixelFormat(id);
}

int VideoWriter::width(int id) const
{
    return videoWriterImpl(_impl)->width(id);
}

int VideoWriter::height(int id) const
{
    return videoWriterImpl(_impl)->height(id);
}

double VideoWriter::aspectRatio(int id) const
{
    return videoWriterImpl(_impl)->aspectRatio(id);
}

int VideoWriter::bitRate(int id) const
{
    return videoWriterImpl(_impl)->bitRate(id);
}

int VideoWriter::bitRateTolerance(int id) const
{
    return videoWriterImpl(_impl)->bitRateTolerance(id);
}

int VideoWriter::gopSize(int id) const
{
    return videoWriterImpl(_impl)->gopSize(id);
}

int VideoWriter::maxBFrames(int id) const
{
    return videoWriterImpl(_impl)->maxBFrames(id);
}

void VideoWriter::write(cv::Mat &image, int id)
{
    return videoWriterImpl(_impl)->write(image, id);
}

long long VideoWriter::frameNumber(int id) const
{
    return videoWriterImpl(_impl)->frameNumber(id);
}

double VideoWriter::timestamp(int id) const
{
    return videoWriterImpl(_impl)->timestamp(id);
}

void VideoWriter::close()
{
    return videoWriterImpl(_impl)->close();
}


}
