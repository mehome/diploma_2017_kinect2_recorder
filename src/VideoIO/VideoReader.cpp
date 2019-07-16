#include <VideoIO/VideoReader.h>
#include <VideoIO/UtilsInternal.h>
#include <algorithm>


namespace video_io
{

VideoReader::ReadParams::ReadParams():
    fixAspectRatio(false),
    aspectRatioTol(2.0),
    interp(INTERP_LANCZOS)
{
}

namespace {

class VideoReaderImpl
{
public:
    typedef VideoReader::Error Error;
    typedef VideoReader::ReadParams ReadParams;

    VideoReaderImpl():
        _formatContext(0),
        _nbStreams(0),
        _nbVideoStreams(0),
        _nbReadableVideoStreams(0),
        _frame(0),
        _eof(true),
        _lastStreamId(-1),
        _dstFrame(0),
        _dstFrameBuf(0),
        _dstFrameBufSize(0)
    {
    }

    ~VideoReaderImpl()
    {
        close();
    }


    void open(std::string const &fileName, int nbThreads)
    {
        FFMpeg::init();
        close();

        int err = avformat_open_input(&_formatContext, fileName.c_str(), 0, 0);
        if (err == AVERROR(ENOMEM))
            throw std::bad_alloc();
        if (err < 0)
            throw Error(ERR_OPEN_FILE,
                        "could not open input file \"" + fileName + "\"");

        err = avformat_find_stream_info(_formatContext, 0);
        if (err == AVERROR(ENOMEM))
            throw std::bad_alloc();
        if (err < 0)
            throw Error(ERR_FIND_STREAM_INFO,
                        "failed to find stream information in \"" + fileName + "\"");

        _nbStreams = _formatContext->nb_streams;
        _frameNumbers.assign(_nbStreams, 0);
        _timestamps.assign(_nbStreams, 0.0);

        for (int i = 0; i < _nbStreams; ++i)
        {
            AVCodecContext *codecCtx = codecContext(i);

            if (codecCtx->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                codecCtx->thread_count = nbThreads < 0 ? cv::getNumberOfCPUs() + 1 : nbThreads;

                AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);

                if (codec)
                {
                    err = avcodec_open2(codecCtx, codec, 0);
                    if (err == AVERROR(ENOMEM))
                        throw std::bad_alloc();
                    if (err < 0)
                        throw Error(ERR_OPEN_CODEC, "failed to open decoder");

                    _nbReadableVideoStreams++;
                }

                _nbVideoStreams++;
            }
        }

        if (_nbReadableVideoStreams > 0)
        {
            if (!(_frame = av_frame_alloc()))
                throw std::bad_alloc();

            _eof = false;
        }
    }

    bool getMetadata(Metadata &metadata) const
    {
        assert(_formatContext);

        metadata.clear();

        if (_formatContext->metadata)
        {
            AVDictionaryEntry *tag = 0;

            while ((tag = av_dict_get(_formatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
                metadata.insert(Metadata::value_type(tag->key, tag->value));
        }

        return !metadata.empty();
    }

    int nbStreams() const
    {
        return _nbStreams;
    }

    int nbThreads(int id) const
    {
        return codecContext(id)->thread_count;
    }

    std::string codecDescription(int id) const
    {
        char buf[256] = {0};

        avcodec_string(buf, sizeof(buf), codecContext(id), false);

        return buf;
    }

    char const *codecName(int id) const
    {
        // return avcodec_get_name(codecContext(id)->codec_id); // Может вернуть "unknown codec".
        AVCodecDescriptor const *desc = avcodec_descriptor_get(codecContext(id)->codec_id);

        if (desc && desc->name)
            return desc->name;

        return "";
    }

    std::string fourCC(int id) const
    {
        unsigned codecTag = 0;

        if (isVideoStream(id))
        {
            AVCodecTag const *riffVideoTags = avformat_get_riff_video_tags();
            codecTag = av_codec_get_tag(&riffVideoTags, codecContext(id)->codec_id);
        }

        if (codecTag <= 0)
            codecTag = codecContext(id)->codec_tag;

        char buf[32] = {0};

        if (codecTag > 0)
            av_get_codec_tag_string(buf, sizeof(buf), codecTag);

        return buf;
    }

    bool isVideoStream(int id) const
    {
        return codecContext(id)->codec_type == AVMEDIA_TYPE_VIDEO;
    }

    int nbVideoStreams() const
    {
        return _nbVideoStreams;
    }

    bool haveVideoDecoder(int id) const
    {
        return isVideoStream(id) && codecContext(id)->codec;
    }

    int nbReadableVideoStreams() const
    {
        return _nbReadableVideoStreams;
    }

    long long nbFrames(int id) const
    {
        int64_t nbf =  stream(id)->nb_frames;

        if (nbf == 0)
        {
            nbf = (int64_t)floor(duration(id) * frameRate(id) + 0.5);
        }
        return nbf;
    }

    double duration(int id) const
    {
        double eps_zero = 0.000025;

        double ret = _formatContext->duration / (double)AV_TIME_BASE;

        if(ret < eps_zero)
        {
            ret = stream(id)->duration * av_q2d(stream(id)->time_base);
        }

        return ret < 0 ? 0 : ret;
    }

    double frameRate(int id) const
    {
        if (!isVideoStream(id))
            return 0;

        double ret = av_q2d(stream(id)->r_frame_rate);

        if (ret > 0)
            return ret;

        ret = 1.0 / av_q2d(codecContext(id)->time_base);

        if (codecContext(id)->ticks_per_frame > 1)
            ret /= codecContext(id)->ticks_per_frame;

        return ret;
    }

    double avgFrameRate(int id) const
    {
        if (!isVideoStream(id))
            return 0;

        return av_q2d(stream(id)->avg_frame_rate);
    }

    char const *pixelFormat(int id) const
    {
        char const *ret = 0;

        if (isVideoStream(id))
        {
            AVPixelFormat pixFmt = codecContext(id)->pix_fmt;

            if (pixFmt != AV_PIX_FMT_NONE)
                ret = av_get_pix_fmt_name(pixFmt);
        }

        return ret ? ret : "";
    }

    int width(int id) const
    {
        return codecContext(id)->width;
    }

    int height(int id) const
    {
        return codecContext(id)->height;
    }

    int width(int id, ReadParams const &params) const
    {
        if (params.fixAspectRatio)
            if (!(params.aspectRatioTol >= 1))
                throw Error(ERR_BAD_PARAM, "invalid aspect ratio tolerance");

        int ret = codecContext(id)->width;
        double aspectRatio = this->aspectRatio(id);

        if (params.fixAspectRatio && aspectRatio * params.aspectRatioTol >= 1 &&
                aspectRatio <= params.aspectRatioTol)
        {
            AVRational r = codecContext(id)->sample_aspect_ratio;

            if (r.num < 0) r.num = -r.num;
            if (r.den < 0) r.den = -r.den;

            if (r.num && r.den && r.num != r.den)
                if (r.num > r.den)
                {
                    ret = static_cast<int64_t>(ret) * r.num / r.den;

                    if (ret & 1)
                        ++ret;
                }
        }

        return ret;
    }

    int height(int id, ReadParams const &params) const
    {
        if (params.fixAspectRatio)
            if (!(params.aspectRatioTol >= 1))
                throw Error(ERR_BAD_PARAM, "invalid aspect ratio tolerance");

        int ret = codecContext(id)->height;
        double aspectRatio = this->aspectRatio(id);

        if (params.fixAspectRatio && aspectRatio * params.aspectRatioTol >= 1 &&
                aspectRatio <= params.aspectRatioTol)
        {
            AVRational r = codecContext(id)->sample_aspect_ratio;

            if (r.num < 0) r.num = -r.num;
            if (r.den < 0) r.den = -r.den;

            if (r.num && r.den && r.num != r.den)
                if (r.num < r.den)
                {
                    ret = static_cast<int64_t>(ret) * r.den / r.num;

                    if (ret & 1)
                        ++ret;
                }
        }

        return ret;
    }

    double aspectRatio(int id) const
    {
        return isVideoStream(id) ? av_q2d(codecContext(id)->sample_aspect_ratio) : 0;
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

    int read(cv::Mat &image, std::vector<int> const &ids, ReadParams const &params)
    {
        if (params.fixAspectRatio)
        {
            if (!(params.aspectRatioTol >= 1))
                throw Error(ERR_BAD_PARAM, "invalid aspect ratio tolerance");
            if (!interpToSWSFlag(params.interp))
                throw Error(ERR_BAD_PARAM, "invalid interpolation type");
        }

        int i = readFrame(ids);

        if (i >= 0)
        {
            AVPixelFormat dstPixFmt = this->dstPixFmt(codecContext(i)->pix_fmt);
            int dstWidth = width(i, params);
            int dstHeight = height(i, params);

            try
            {
                AVFrame *picture = convertImage(_frame, &_dstFrame, dstPixFmt, dstWidth, dstHeight,
                                                &_dstFrameBuf, &_dstFrameBufSize,
                                                SWS_ACCURATE_RND | interpToSWSFlag(params.interp));

                image = cv::Mat(dstHeight, dstWidth, elemType(dstPixFmt), picture->data[0],
                        picture->linesize[0]);
            }
            catch (video_io::Error &e)
            {
                throw Error(e.code(), e.what());
            }
        }else{
            this->seekFrame(0);
        }

        return i;
    }

    int read(cv::Mat &image, int id, ReadParams const &params)
    {
        _ids.resize(1);
        _ids[0] = id;

        return read(image, _ids, params);
    }

    int read(cv::Mat &image, ReadParams const &params)
    {
        _ids.resize(0);

        return read(image, _ids, params);
    }

    long long frameNumber(int id) const
    {
        assert(id >= 0);
        assert(id < _nbStreams);

        return _frameNumbers[id];
    }

    double timestamp(int id) const
    {
        assert(id >= 0);
        assert(id < _nbStreams);

        return _timestamps[id];
    }

    void close()
    {
        if (!_formatContext)
            return;

        for (int i = 0; i < _nbStreams; ++i)
            avcodec_close(codecContext(i));

        avformat_close_input(&_formatContext);
        _formatContext = 0;

        _nbStreams = 0;
        _nbVideoStreams = 0;
        _nbReadableVideoStreams = 0;

        if (_frame)
        {
            av_frame_free(&_frame);
            _frame = 0;
        }

        _eof = true;
        _lastStreamId = -1;

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

        _frameNumbers.clear();
        _timestamps.clear();

        _ids.clear();
    }

/*
    bool AVDemuxer::seek(qint64 pos)
    {
        if ((!a_codec_context && !v_codec_context) || !format_context) {
            qWarning("can not seek. context not ready: %p %p %p", a_codec_context, v_codec_context, format_context);
            return false;
        }
        //duration: unit is us (10^-6 s, AV_TIME_BASE)
        qint64 upos = pos*1000LL;
        if (upos > durationUs() || pos < 0LL) {
            qWarning("Invalid seek position %lld %.2f. valid range [0, %lld]", upos, double(upos)/double(durationUs()), durationUs());
            return false;
        }
        if (seek_timer.isValid()) {
            //why sometimes seek_timer.elapsed() < 0
            if (!seek_timer.hasExpired(kSeekInterval)) {
                qDebug("seek too frequent. ignore");
                return false;
            }
            seek_timer.restart();
        } else {
            seek_timer.start();
        }

        QMutexLocker lock(&mutex);
        Q_UNUSED(lock);
    #if 0
        //t: unit is s
        qreal t = q;// * (double)format_context->duration; //
        int ret = av_seek_frame(format_context, -1, (int64_t)(t*AV_TIME_BASE), t > pkt->pts ? 0 : AVSEEK_FLAG_BACKWARD);
        qDebug("[AVDemuxer] seek to %f %f %lld / %lld", q, pkt->pts, (int64_t)(t*AV_TIME_BASE), durationUs());
    #else
        //TODO: pkt->pts may be 0, compute manually.

        bool backward = upos <= (int64_t)(pkt->pts*AV_TIME_BASE);
        qDebug("[AVDemuxer] seek to %f %f %lld / %lld backward=%d", double(upos)/double(durationUs()), pkt->pts, upos, durationUs(), backward);
        //AVSEEK_FLAG_BACKWARD has no effect? because we know the timestamp
        // FIXME: back flag is opposite? otherwise seek is bad and may crash?
        // If stream_index is (-1), a default
         // stream is selected, and timestamp is automatically converted
         // from AV_TIME_BASE units to the stream specific time_base.

        int seek_flag = (backward ? 0 : AVSEEK_FLAG_BACKWARD); //AVSEEK_FLAG_ANY
        //bool seek_bytes = !!(format_context->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", format_context->iformat->name);
        int ret = av_seek_frame(format_context, -1, upos, seek_flag);
        //avformat_seek_file()
    #endif
        if (ret < 0) {
            qWarning("[AVDemuxer] seek error: %s", av_err2str(ret));
            return false;
        }
        //replay
        qDebug("startTime: %lld", startTime());
        if (upos <= startTime()) {
            qDebug("************seek to beginning. started = false");
            started_ = false;
            if (a_codec_context)
                a_codec_context->frame_number = 0;
            if (v_codec_context)
                v_codec_context->frame_number = 0; //TODO: why frame_number not changed after seek?
            if (s_codec_contex)
                s_codec_contex->frame_number = 0;
        }
        return true;
    }*/
    bool    seekFrame(int64_t frame)
    {
        if(-1==_frame->pkt_pos)
        {
            av_seek_frame(_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
            _eof  = false;
        }else{
            double sec = ((double)frame/frameRate(0))*AV_TIME_BASE;
            if(sec<0)
            {
                sec=0;
            }
            if(sec>double(_formatContext->duration))
            {
                sec = double(_formatContext->duration);
            }

                //to seconds frame
                int64_t upos = (int64_t)sec;//1000LL;

                bool backward = upos <= (int64_t)(_frame->pkt_pos);
          //      printf("pos = %lld",_frame->pkt_pos);
               // printf("[AVDemuxer] seek to %f %f %lld / %lld backward=%d", double(upos)/double(_formatContext->duration), _frame->pkt_pos, upos, _formatContext->duration, backward);
                int seek_flag = (backward ? 0 : AVSEEK_FLAG_BACKWARD); //AVSEEK_FLAG_ANY
               int ret = av_seek_frame(_formatContext, -1, upos, seek_flag);

                if (ret < 0) {
               //     printf("[AVDemuxer] seek error");
                    return false;
                }
        }
            //replay

            return true;
    }
private:
    AVStream *stream(int id) const
    {
        assert(_formatContext);
        assert(id >= 0);
        assert(id < _nbStreams);

        return _formatContext->streams[id];
    }

    AVCodecContext *codecContext(int id) const
    {
        return stream(id)->codec;
    }

    int readFrame(std::vector<int> const &ids)
    {
        if (_eof)
            return STS_EOF;

        int gotFrame = 0;
        int id;
        AVPacket pkt;

        av_init_packet(&pkt);
        pkt.data = 0;
        pkt.size = 0;

        while (true)
        {
            int err = av_read_frame(_formatContext, &pkt);

            if (err < 0)
            {
                if (err == AVERROR_EOF || url_feof(_formatContext->pb))
                {
                    if (!pkt.data)
                        _eof = true; // Все пакеты переданы декодерам.
                }
                else
                {
                    av_free_packet(&pkt);

                    if (err == AVERROR(EAGAIN))
                        return STS_EAGAIN;

                    if (err == AVERROR(ENOMEM))
                        throw std::bad_alloc();

                    throw Error(ERR_RW_FRAME, "failed to read frame");
                }
            }

            err = 0;

            // _lastStreamId используется для организации равномерной выборки кадров,
            // буферизованных декодерами.
            int i = 0;
            id = (_eof && _lastStreamId >= 0) ? (_lastStreamId + 1) % _nbStreams : 0;

            // Если _eof == false, то пакет либо передается декодеру, либо пропускается в
            // зависимости от pkt.stream_index. Если _eof == true, то забираются кадры,
            // буферизованные в декодерах.
            for (; i < _nbStreams; ++i, id = (id + 1) % _nbStreams)
                if (haveVideoDecoder(id) && (_eof || (id == pkt.stream_index &&
                    (ids.empty() || std::find(ids.begin(), ids.end(), id) != ids.end()))))
                {
                    err = avcodec_decode_video2(codecContext(id), _frame, &gotFrame, &pkt);

                    if (err < 0 || !_eof || gotFrame)
                        break;
                }

            av_free_packet(&pkt);

            if (err == AVERROR(ENOMEM))
                throw std::bad_alloc();
            if (err < 0)
                throw Error(ERR_ENC_DEC_VIDEO, "failed to decode video frame packet");

            if (gotFrame || (_eof && i == _nbStreams))
                break; // Декодирован очередной кадр или декодировать больше нечего.
        }

        if (gotFrame)
        {
            _frameNumbers[id]++;

            int64_t pts = av_frame_get_best_effort_timestamp(_frame);
            int64_t startTime = stream(id)->start_time;

            pts = (pts == AV_NOPTS_VALUE) ?
                        0 : startTime == AV_NOPTS_VALUE ? pts : startTime + pts;
            _timestamps[id] = pts * av_q2d(stream(id)->time_base);

            _eof = false; // В декодерах могут еще оставаться буферизованные кадры.
            _lastStreamId = id;

            return id;
        }else{
            _frameNumbers[id]=0;
        }

        return STS_EOF;
    }

    static bool pixFmtMono(AVPixelFormat pixFmt)
    {
        return pixFmt == AV_PIX_FMT_MONOBLACK ||
                pixFmt == AV_PIX_FMT_MONOWHITE;
    }

    static bool pixFmtGray(AVPixelFormat pixFmt)
    {
        return pixFmt == AV_PIX_FMT_GRAY8 ||
                pixFmt == AV_PIX_FMT_GRAY8A ||
                pixFmt == AV_PIX_FMT_GRAY16BE ||
                pixFmt == AV_PIX_FMT_GRAY16LE;
    }

    static int bitsPerPixel(AVPixelFormat pixFmt)
    {
        AVPixFmtDescriptor const *desc = av_pix_fmt_desc_get(pixFmt);

        return desc ? av_get_bits_per_pixel(desc) : 0;
    }

    static int nbComponents(AVPixelFormat pixFmt)
    {
        AVPixFmtDescriptor const *desc = av_pix_fmt_desc_get(pixFmt);

        return desc ? desc->nb_components : 0;
    }

    static AVPixelFormat dstPixFmt(AVPixelFormat srcPixFmt)
    {
        if (srcPixFmt < 0)
            return AV_PIX_FMT_NONE;

        int bpp = bitsPerPixel(srcPixFmt);
        int nbc = nbComponents(srcPixFmt);

        if (bpp <= 0 || nbc <= 0)
            return AV_PIX_FMT_NONE;

        if (pixFmtMono(srcPixFmt) || pixFmtGray(srcPixFmt))
        {
            if (bpp <= 8)
                return AV_PIX_FMT_GRAY8;

            return AV_PIX_FMT_GRAY16;
        }

        if (nbc <= 3)
        {
            if (bpp <= 24)
                return AV_PIX_FMT_BGR24;

            return AV_PIX_FMT_BGR48;
        }

        if (bpp <= 32)
            return AV_PIX_FMT_BGR24;

        return AV_PIX_FMT_BGR48;
    }



    AVFormatContext *_formatContext;
    int _nbStreams;
    int _nbVideoStreams;
    int _nbReadableVideoStreams;
    AVFrame *_frame;
    bool _eof;
    int _lastStreamId;
    AVFrame *_dstFrame;
    unsigned char *_dstFrameBuf;
    std::ptrdiff_t _dstFrameBufSize;
    std::vector<long long> _frameNumbers;
    std::vector<double> _timestamps;
    std::vector<int> _ids;
};

VideoReaderImpl *videoReaderImpl(void *impl)
{
    return static_cast<VideoReaderImpl *>(impl);
}

}

VideoReader::VideoReader():
    _impl(new VideoReaderImpl)
{
}

bool VideoReader::seek(unsigned int pos)
{
    return videoReaderImpl(_impl)->seekFrame(pos);
}

VideoReader::~VideoReader()
{
    delete videoReaderImpl(_impl);
}

void VideoReader::open(std::string const &fileName, int nbThreads)
{
    videoReaderImpl(_impl)->open(fileName, nbThreads);
}

bool VideoReader::getMetadata(Metadata &metadata) const
{
    return videoReaderImpl(_impl)->getMetadata(metadata);
}

int VideoReader::nbStreams() const
{
    return videoReaderImpl(_impl)->nbStreams();
}

int VideoReader::nbThreads(int id) const
{
    return videoReaderImpl(_impl)->nbThreads(id);
}

std::string VideoReader::codecDescription(int id) const
{
    return videoReaderImpl(_impl)->codecDescription(id);
}

char const *VideoReader::codecName(int id) const
{
    return videoReaderImpl(_impl)->codecName(id);
}

std::string VideoReader::fourCC(int id) const
{
    return videoReaderImpl(_impl)->fourCC(id);
}

bool VideoReader::isVideoStream(int id) const
{
    return videoReaderImpl(_impl)->isVideoStream(id);
}

int VideoReader::nbVideoStreams() const
{
    return videoReaderImpl(_impl)->nbVideoStreams();
}

bool VideoReader::haveVideoDecoder(int id) const
{
    return videoReaderImpl(_impl)->haveVideoDecoder(id);
}

int VideoReader::nbReadableVideoStreams() const
{
    return videoReaderImpl(_impl)->nbReadableVideoStreams();
}

long long VideoReader::nbFrames(int id) const
{
    return videoReaderImpl(_impl)->nbFrames(id);
}

double VideoReader::duration(int id) const
{
    return videoReaderImpl(_impl)->duration(id);
}

double VideoReader::frameRate(int id) const
{
    return videoReaderImpl(_impl)->frameRate(id);
}

double VideoReader::avgFrameRate(int id) const
{
    return videoReaderImpl(_impl)->avgFrameRate(id);
}

char const *VideoReader::pixelFormat(int id) const
{
    return videoReaderImpl(_impl)->pixelFormat(id);
}

int VideoReader::width(int id) const
{
    return videoReaderImpl(_impl)->width(id);
}

int VideoReader::height(int id) const
{
    return videoReaderImpl(_impl)->height(id);
}

int VideoReader::width(int id, ReadParams const &params) const
{
    return videoReaderImpl(_impl)->width(id, params);
}

int VideoReader::height(int id, ReadParams const &params) const
{
    return videoReaderImpl(_impl)->height(id, params);
}

double VideoReader::aspectRatio(int id) const
{
    return videoReaderImpl(_impl)->aspectRatio(id);
}

int VideoReader::bitRate(int id) const
{
    return videoReaderImpl(_impl)->bitRate(id);
}

int VideoReader::bitRateTolerance(int id) const
{
    return videoReaderImpl(_impl)->bitRateTolerance(id);
}

int VideoReader::gopSize(int id) const
{
    return videoReaderImpl(_impl)->gopSize(id);
}

int VideoReader::maxBFrames(int id) const
{
    return videoReaderImpl(_impl)->maxBFrames(id);
}

int VideoReader::read(cv::Mat &image, std::vector<int> const &ids, ReadParams const &params)
{
    return videoReaderImpl(_impl)->read(image, ids, params);
}

int VideoReader::read(cv::Mat &image, int id, ReadParams const &params)
{
    return videoReaderImpl(_impl)->read(image, id, params);
}

int VideoReader::read(cv::Mat &image, ReadParams const &params)
{
    return videoReaderImpl(_impl)->read(image, params);
}

long long VideoReader::frameNumber(int id) const
{
    return videoReaderImpl(_impl)->frameNumber(id);
}

double VideoReader::timestamp(int id) const
{
    return videoReaderImpl(_impl)->timestamp(id);
}

void VideoReader::close()
{
    videoReaderImpl(_impl)->close();
}
}
