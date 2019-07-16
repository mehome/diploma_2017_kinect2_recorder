/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "Kinect2Recorder.h"
#include "mat-stream/Kinect2RgbMatStream.h"
#include "mat-stream/Kinect2Gray16MatStream.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "VideoIO/VideoWriter.h"
#include <iostream>
#include <ctime>

namespace kinect2recorder
{

    Kinect2Recorder::Kinect2Recorder(Kinect2RecorderLogger& kinect2RecorderLogger) :
            _active(false),
            _writing(false),
            _pKinect2Wrapper(nullptr),
            _fps(DEFAULT_FPS),
            _directoryPath(DEFAULT_DIRECTORY_PATH),
            _lastPath(),
            _pVideoWriter(nullptr),
            _logger(kinect2RecorderLogger),
            _mseconds(0),
            _elapsedTimer(),
            _isTimer(false),
            _mutex()
    {
        try
        {
            _pKinect2Wrapper = new kinect2reader::Kinect2Wrapper();
        }
        catch (kinect2reader::Kinect2WrapperInitException)
        {
            _logger.LogFailedInit();
            throw Kinect2RecorderInitException();
        }
        _pFrameStreams[0] = new RgbMatStream(_pKinect2Wrapper);
        _pFrameStreams[1] = new Gray16MatStream(_pKinect2Wrapper);
        _active = true;
        _logger.LogInit();
    }

    Kinect2Recorder::~Kinect2Recorder()
    {
        InnerDeactivate();
    }

    void Kinect2Recorder::InnerDeactivate()
    {
        if (_pVideoWriter != nullptr)
        {
            try
            {
                _pVideoWriter->close();
            }
            catch (...)
            {

            }
            delete(_pVideoWriter);
            _pVideoWriter = nullptr;
        }
        if (_pFrameStreams[0] != nullptr)
        {
            delete(_pFrameStreams[0]);
            _pFrameStreams[0] = nullptr;
        }
        if (_pFrameStreams[1] != nullptr)
        {
            delete(_pFrameStreams[1]);
            _pFrameStreams[1] = nullptr;
        }
        if (_pKinect2Wrapper != nullptr)
        {
            delete(_pKinect2Wrapper);
            _pKinect2Wrapper = nullptr;
        }
        _active = false;
        _logger.LogNotActive();
    }

    bool Kinect2Recorder::InnerStart()
    {
        if (!_active)
        {
            _logger.LogFailedWhenNotActive();
            return false;
        }
        if (_writing)
        {
            _logger.LogFailedWhenWritingOn();
            return false;
        }
        bool anyMode = false;
        for (int i = 0; i < MODES_NUMBER && !anyMode; i++)
        {
            if (_modesActivity[i])
            {
                anyMode = true;
            }
        }
        if (!anyMode)
        {
            _logger.LogFailedStartWhenNoneMode();
            return false;
        }
        _pVideoWriter = new video_io::VideoWriter();
        std::time_t time;
        std::tm* timeinfo;
        char nameBuffer [80];
        std::time(&time);
        timeinfo = std::localtime(&time);
        std::strftime(nameBuffer,80,"%Y-%m-%d-%H-%M-%S",timeinfo);
        _lastPath = _directoryPath + std::string(nameBuffer) + std::string(".") + _extension;
        try
        {
            _pVideoWriter->open(_lastPath.c_str());
        }
        catch (...)
        {
            _logger.LogFailedStartWhenOpenWithPath(_lastPath);
            return false;
        }
        try
        {
            video_io::Metadata metadata;
            metadata.insert(video_io::Metadata::value_type("title", "title"));
            metadata.insert(video_io::Metadata::value_type("Camera0", "f: 1000, k: 1, m0: (255, 270), gamma: 1"));
            metadata.insert(video_io::Metadata::value_type("Camera1", "f: 2000, k: 1, m0: (235, 288), gamma: 1"));
            metadata.insert(video_io::Metadata::value_type("Camera2", "f: 2500, k: 1, m0: (225, 271), gamma: 1"));
            _pVideoWriter->setMetadata(metadata);
        }
        catch (...)
        {
            _logger.LogFailedStartWhenOpenWithPath(_lastPath);
            try
            {
                _pVideoWriter->close();
            }
            catch (...)
            {
            }
            delete(_pVideoWriter);
            _pVideoWriter = nullptr;
            return false;
        }

        for (int i = 0; i < MODES_NUMBER; i++)
        {
            try
            {
                if (_modesActivity[i])
                {
                    video_io::VideoWriter::VideoStreamParams videoStreamParams;
                    videoStreamParams.codecName = _codec_names[i];
                    videoStreamParams.pixelFormat = _pix_fmt_names[i];
                    videoStreamParams.frameRate = _fps;
                    videoStreamParams.width = _pFrameStreams[i]->GetWidth();
                    videoStreamParams.height = _pFrameStreams[i]->GetHeight();
                    videoStreamParams.findBestPixelFormat = false;
                    _pVideoWriter->addVideoStream(videoStreamParams);
                }
            }
            catch (...)
            {
                _logger.LogFailedStartWhenAddVideoStream(_lastPath, i);
                try
                {
                    _pVideoWriter->close();
                }
                catch (...)
                {
                }
                delete(_pVideoWriter);
                _pVideoWriter = nullptr;
                return false;
            }
        }
        return true;
    }

    bool Kinect2Recorder::InnerStop()
    {
        if (!_active)
        {
            _logger.LogFailedWhenNotActive();
            return false;
        }
        if (!_writing)
        {
            _logger.LogFailedWhenWritingOff();
            return false;
        }
        _isTimer = false;
        try
        {
            _pVideoWriter->close();
        }
        catch (...)
        {
            _logger.LogFailedStopWhenClose(_lastPath);
            delete(_pVideoWriter);
            _pVideoWriter = nullptr;
            _writing = false;
            return false;
        }
        delete(_pVideoWriter);
        _pVideoWriter = nullptr;
        _writing = false;
        _logger.LogStop(_lastPath);
        return true;
    }

    bool Kinect2Recorder::IsActive()
    {
        return _active;
    }

    void Kinect2Recorder::SetDirectoryPath(std::string directoryPath)
    {
        _mutex.lock();
        if (!_active)
        {
            _logger.LogFailedWhenNotActive();
            _mutex.unlock();
            return;
        }
        if (_writing)
        {
            _logger.LogFailedWhenWritingOn();
            _mutex.unlock();
            return;
        }
        _directoryPath = directoryPath + std::string("\\");
        _logger.LogDirectoryPath(directoryPath);
        _mutex.unlock();
    }

    void Kinect2Recorder::SetMode(int mode)
    {
        _mutex.lock();
        if (!_active)
        {
            _logger.LogFailedWhenNotActive();
            _mutex.unlock();
            return;
        }
        if (_writing)
        {
            _logger.LogFailedWhenWritingOn();
            _mutex.unlock();
            return;
        }
        DWORD frameSourceTypes = FrameSourceTypes::FrameSourceTypes_None;
        for (int i = 0; i < MODES_NUMBER; i++)
        {
            _modesActivity[i] = false;
        }
        for (int i = 0; i < MODES_NUMBER; i++)
        {
            if (_modes[i] & mode)
            {
                _modesActivity[i] = true;
                frameSourceTypes = frameSourceTypes | _frame_source_types[i];
            }
        }
        try
        {
            _pKinect2Wrapper->SetFrameSourceTypes(frameSourceTypes);
        }
        catch (kinect2reader::Kinect2WrapperFailedException)
        {
            _logger.LogFailedSetMode();
            InnerDeactivate();
            _mutex.unlock();
            return;
        }
        _logger.LogSetMode();
        _mutex.unlock();
    }

    void Kinect2Recorder::SetSize(int mode, int width, int height)
    {
        _mutex.lock();
        if (!_active)
        {
            _logger.LogFailedWhenNotActive();
            _mutex.unlock();
            return;
        }
        if (_writing)
        {
            _logger.LogFailedWhenWritingOn();
            _mutex.unlock();
            return;
        }
        if (width <= 0 || height <= 0)
        {
            _logger.LogFailedSetSize();
            _mutex.unlock();
            return;
        }
        cv::Size size(width, height);
        for(int i = 0; i < MODES_NUMBER; i++)
        {
            if (_modes[i] & mode)
            {
                _pFrameStreams[i]->SetSize(size);
            }
        }
        _logger.LogSetSize();
        _mutex.unlock();
    }

    void Kinect2Recorder::SetFPS(int fps)
    {
        _mutex.lock();
        if (fps <= 0)
        {
            _logger.LogSetFailedFPSWhenIncorrectValue();
            _mutex.unlock();
            return;
        }
        _fps = fps;
        _logger.LogSetFPS();
        _mutex.unlock();
    }

    void Kinect2Recorder::Update()
    {
        _mutex.lock();
        if (!_active)
        {
            _mutex.unlock();
            return;
        }
        if (_isTimer)
        {
            if (_elapsedTimer.elapsed() >= _mseconds)
            {
                InnerStop();
            }
        }
        for(int i = 0; i < MODES_NUMBER; i++)
        {
            if (_oldModesActivity[i] != _modesActivity[i])
            {
                if (_oldModesActivity[i])
                {
                    cv::destroyWindow(_windowNames[i]);
                }
                _oldModesActivity[i] = _modesActivity[i];
            }
        }
        try
        {
            _pKinect2Wrapper->Update();
        }
        catch(kinect2reader::Kinect2WrapperFailedException)
        {
            _logger.LogKinectOff();
            InnerDeactivate();
            _mutex.unlock();
            return;
        }
        cv::Mat * mats[MODES_NUMBER];
        bool allMats = true;
        for (int i = 0; i < MODES_NUMBER; i++)
        {
            mats[i] = nullptr;
            if (_modesActivity[i])
            {
                cv::Mat * pMat = _pFrameStreams[i]->GetMat();
                if (pMat != nullptr)
                {
                    mats[i] = pMat;
                }
                else
                {
                    allMats = false;
                }
            }
        }
        for (int i = 0; i < MODES_NUMBER; i++)
        {
            if (mats[i] != nullptr)
            {
                cv::imshow(_windowNames[i], *(mats[i]));
                cv::waitKey(1);
            }
        }
        if (_writing && allMats)
        {
            int videoStreamNumber = 0;
            for(int i = 0; i < MODES_NUMBER; i++)
            {
                if (_modesActivity[i])
                {
                    try
                    {
                        _pVideoWriter->write(*(mats[i]), videoStreamNumber);
                    }
                    catch (...)
                    {
                        _logger.LogFailedWrite(_lastPath, i);
                    }
                    videoStreamNumber++;
                }
            }
        }
        for (int i = 0; i < MODES_NUMBER; i++)
        {
            if (mats[i] != nullptr)
            {
                delete(mats[i]);
                mats[i] = nullptr;
            }
        }
        _mutex.unlock();
    }

    void Kinect2Recorder::Start()
    {
        _mutex.lock();
        if (InnerStart())
        {
            _logger.LogStart(_lastPath);
            _writing = true;
        }
        _mutex.unlock();
    }

    void Kinect2Recorder::Start(int seconds)
    {
        _mutex.lock();
        if (seconds <= 0)
        {
            _logger.LogFailedStartWhenIncorrectTime();
            _mutex.unlock();
            return;
        }
        if (InnerStart())
        {
            _mseconds = 1000 * seconds;
            _elapsedTimer.restart();
            _isTimer = true;
            _writing = true;
            _logger.LogStart(_lastPath);
        }
        _mutex.unlock();
    }

    void Kinect2Recorder::Stop()
    {
        _mutex.lock();
        InnerStop();
        _mutex.unlock();
    }

    void Kinect2Recorder::Deactivate()
    {
        _mutex.lock();
        InnerDeactivate();
        _mutex.unlock();
    }

}
