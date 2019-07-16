#ifndef __VIDEO_IO__VIDEO_READER_H
#define __VIDEO_IO__VIDEO_READER_H

#include <VideoIO/Defs.h>
#include <vector>
#include <opencv2/core/core.hpp>

#include <VideoIO/AnnoyingWarningsOff.h>

namespace video_io
{

class VIDEO_IO_API VideoReader
{
public:
    DERIVED_VIDEO_IO_ERROR(Error, "VideoReader error")

    struct VIDEO_IO_API ReadParams
    {
        ReadParams();

        // Исправлять aspect ratio?
        // Значение по умолчанию: fixAspectRatio = false.
        bool fixAspectRatio;

        // Ограничение на максимальное и минимальное значения aspect ratio, которые
        // будут исправляться: aspectRatioTol^-1 <= aspectRatio <= aspectRatioTol.
        // Допустимые значения: aspectRatioTol >= 1.
        // Значение по умолчанию: aspectRatioTol = 2.0.
        double aspectRatioTol;

        // Тип интерполяции.
        // Значение по умолчанию: interp = INTERP_LANCZOS.
        int interp;
    };

    VideoReader();

    ~VideoReader();

    // При nbThreads <= 0 число потоков выбирается автоматически.
    void open(std::string const &fileName, int nbThreads = -1);

    // Возвращает !metadata.empty().
    bool getMetadata(Metadata &metadata) const;

    // Полное число потоков.
    int nbStreams() const;

    int nbThreads(int id) const;

    // Описание кодека.
    std::string codecDescription(int id) const;

    // Имя кодека или пустая строка.
    // Если haveVideoDecoder(id), то гарантированно возвращает имя кодека.
    char const *codecName(int id) const;

    // RIFF codec tag или пустая строка.
    // Если haveVideoDecoder(id), то гарантированно возвращает RIFF codec tag.
    std::string fourCC(int id) const;

    bool isVideoStream(int id) const;

    // Число видеопотоков.
    int nbVideoStreams() const;

    bool haveVideoDecoder(int id) const;

    // Число видеопотоков, для которых есть декодеры.
    int nbReadableVideoStreams() const;

    // Число кадров в потоке, если известно, или 0.
    long long nbFrames(int id) const;

    // Продолжительность (s) или 0.
    double duration(int id) const;

    // Real base frame rate (FPS) видеопотока или его оценка по time_base.
    // This is the lowest framerate with which all timestamps can be represented accurately
    // (it is the least common multiple of all framerates in the stream). Note, this value
    // is just a guess! For example, if the time base is 1/90000 and all frames have either
    // approximately 3600 or 1800 timer ticks, then r_frame_rate will be 50/1.
    double frameRate(int id) const;

    // Оценка среднего значения FPS видеопотока.
    double avgFrameRate(int id) const;

    // Формат пикселя декодированного кадра или пустая строка. Иногда устанавливается
    // кодеком после декодирования первого кадра.
    char const *pixelFormat(int id) const;

    // Ширина оригинального кадра.
    int width(int id) const;

    // Высота оригинального кадра.
    int height(int id) const;

    // Ширина кадра с учетом параметров чтения.
    int width(int id, ReadParams const &params) const;

    // Высота кадра с учетом параметров чтения.
    int height(int id, ReadParams const &params) const;

    // Ширина пикселя, деленная на его высоту, или 0.
    double aspectRatio(int id) const;

    // The average bit rate (bit/s) или 0.
    int bitRate(int id) const;

    // Number of bits the bitstream is allowed to diverge from the reference.
    int bitRateTolerance(int id) const;

    // Emit one I-frame every gopSize frames at most. gopSize = 0 - intra only.
    int gopSize(int id) const;

    // Maximum number of B-frames between non-B-frames.
    int maxBFrames(int id) const;

    // Отмотать на кадр с номером pos
    bool seek(unsigned int pos);

    // Чтение одного или нескольких видеопотоков. Если ids.empty(), то читаются все видеопотоки.
    // Данные выходного кадра image хранятся во внутреннем буфере VideoReader-а и остаются
    // действительными до следующего вызова read() или close(). В случае успеха возвращает
    // id потока, из которого прочитан кадр. Если кадр прочитать не удалось, то возвращает
    // STS_EOF или STS_EAGAIN (оба значения < 0). Из-за разной латентности кодеков порядок
    // чтения кадров из РАЗНЫХ видеопотоков будет отличаться от порядка их записи в файле.
    // Типы элементов выходного кадра: CV_8UC1, CV_16UC1, CV_8UC3 (BGR24), CV_16UC3 (BGR48).
    int read(cv::Mat &image, std::vector<int> const &ids, ReadParams const &params = ReadParams());

    // Чтение одного видеопотока.
    int read(cv::Mat &image, int id, ReadParams const &params = ReadParams());

    // Чтение всех видеопотоков.
    int read(cv::Mat &image, ReadParams const &params = ReadParams());

    // Число прочитанных кадров.
    long long frameNumber(int id) const;

    // Метки времени (s) последних прочитанных кадров или 0. При воспроизведении надежнее
    // полагаться на frameRate(id) и frameNumber(id).
    double timestamp(int id) const;

    void close();

private:
    VideoReader(VideoReader const &); // Not implemented.
    VideoReader &operator =(VideoReader const &); // Not implemented.

    void *_impl;

};

}

#include <VideoIO/AnnoyingWarningsOn.h>

#endif
