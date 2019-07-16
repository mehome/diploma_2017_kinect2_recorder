#ifndef __VIDEO_IO__VIDEO_WRITER_H
#define __VIDEO_IO__VIDEO_WRITER_H

#include <VideoIO/Defs.h>
#include <opencv2/core/core.hpp>

#include <VideoIO/AnnoyingWarningsOff.h>

namespace video_io
{

class VIDEO_IO_API VideoWriter
{
public:
    DERIVED_VIDEO_IO_ERROR(Error, "VideoWriter error")

    struct VIDEO_IO_API VideoStreamParams
    {
        VideoStreamParams();

        // Имя кодека (ffmpeg) или RIFF codec tag (FourCC).
        // Пустая строка - попытаться использовать кодек по умолчанию для заданного формата
        // файла. RIFF теги видеокодеков перечислены в файле libavformat/riff.c. Список
        // кодеков есть так же в документации в файле general.html.
        // Значение по умолчанию: codecName = "ffv1".
        std::string codecName;

        // Frame rate (FPS). Обязательный параметр.
        // Используется для инициализации параметра time_base контекста кодека.
        // time_base is the fundamental unit of time (in seconds) in terms of which frame
        // timestamps are represented. For fixed-fps content, time_base should be fps^-1 and
        // timestamp increments should be identical to 1.
        // Допустимые значения: frameRate > 0.
        double frameRate;

        // Строка, определяющая формат пикселя, используемый при кодировании и сохранении
        // изображений в файле (см. libavutil/pixdesc.c). Обязательный параметр.
        std::string pixelFormat;

        // Если заданный формат пикселя не поддерживается кодеком, то автоматически выбрать
        // наилучший из числа поддерживаемых кодеком?
        // Значение по умолчанию: findBestPixelFormat = false.
        bool findBestPixelFormat;

        // Ширина кадра. Обязательный параметр.
        // Допустимые значения: width - четное число, width > 0.
        int width;

        // Высота кадра. Обязательный параметр.
        // Допустимые значения: height - четное число, height > 0.
        int height;

        // Ширина пикселя, деленная на его высоту. aspectRatio = 0 - значение не определено.
        // aspectRatio < 0 - использовать значение по умолчанию, определяемое ffmpeg.
        // Допустимые значения: aspectRatio <= 10.
        // Значение по умолчанию: aspectRatio = -1.
        double aspectRatio;

        // The average bit rate (bit/s). Unused for constant quantizer encoding.
        // bitRate < 0 - использовать значение по умолчанию, определяемое ffmpeg.
        // Значение по умолчанию: bitRate = -1.
        int bitRate;

        // Number of bits the bitstream is allowed to diverge from the reference. The reference
        // can be CBR (for CBR pass1) or VBR (for pass2). Unused for constant quantizer encoding.
        // bitRateTolerance < 0 - использовать значение по умолчанию, определяемое ffmpeg.
        // Значение по умолчанию: bitRateTolerance = -1.
        int bitRateTolerance;

        // Emit one I-frame every gopSize frames at most. Set gopSize = 0 for intra only.
        // gopSize < 0 - использовать значение по умолчанию, определяемое ffmpeg.
        // Значение по умолчанию: gopSize = -1.
        int gopSize;

        // Maximum number of B-frames between non-B-frames.
        // maxBFrames < 0 - использовать значение по умолчанию, определяемое ffmpeg.
        // Значение по умолчанию: maxBFrames = -1.
        int maxBFrames;

        // Другие опции контекста кодека AVCodecContext (см. libavcodec/avcodec.h).
        // Формат строки: "опция1=значение1:опция2=значение2: ... :опцияN=значениеN".
        // Полный список опций находится в файле libavcodec/options_table.h.
        std::string options;
    };

    VideoWriter();

    // Вызывает close(). Перехватывает все исключения, сгенерированные в теле деструктора,
    // поэтому, при завершении записи видеофайла рекомендуется явно вызывать close().
    ~VideoWriter();

    // При nbThreads <= 0 число потоков выбирается автоматически.
    void open(std::string const &fileName, int nbThreads = -1);

    void setMetadata(Metadata const &metadata);

    // Добавляет видеопоток. Все потоки должны быть добавлены до записи первого кадра.
    int addVideoStream(VideoStreamParams const &params);

    // Полное число потоков.
    int nbStreams() const;

    int nbThreads(int id) const;

    // Описание кодека.
    std::string codecDescription(int id) const;

    // Имя кодека.
    char const *codecName(int id) const;

    // RIFF codec tag.
    std::string fourCC(int id) const;

    double frameRate(int id) const;

    char const *pixelFormat(int id) const;

    int width(int id) const;

    int height(int id) const;

    double aspectRatio(int id) const;

    int bitRate(int id) const;

    int bitRateTolerance(int id) const;

    int gopSize(int id) const;

    int maxBFrames(int id) const;

    // Запись кадра в поток id. Из-за разной латентности кодеков порядок записи в файле кадров,
    // относящимся к РАЗНЫМ видеопотокам, будет отличаться от порядка их передачи на запись.
    // Типы элементов входного кадра: CV_8UC1, CV_16UC1, CV_8UC3 (BGR24), CV_16UC3 (BGR48).
    void write(cv::Mat &image, int id);

    // Число записанных кадров.
    long long frameNumber(int id) const;

    // Метки времени (s) последних записанных кадров.
    double timestamp(int id) const;

    // Может сгенерировать исключение, т.к. выполняет flush кодеков и запись трэйлера.
    void close();

private:
    VideoWriter(VideoWriter const &); // Not implemented.
    VideoWriter &operator =(VideoWriter const &); // Not implemented.

    void *_impl;
};

}

#include <VideoIO/AnnoyingWarningsOn.h>

#endif
