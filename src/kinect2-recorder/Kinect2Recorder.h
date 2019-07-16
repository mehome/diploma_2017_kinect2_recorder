/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include "Kinect2RecorderLogger.h"
#include "Kinect2RecorderInitException.h"
#include "kinect2-reader/Kinect2Wrapper.h"
#include "Kinect2RecorderInitException.h"
#include "mat-stream/MatStream.h"
#include "VideoIO/VideoWriter.h"
#include<mutex>
#include <QElapsedTimer>

namespace kinect2recorder
{

    class Kinect2Recorder
    {
    private:
        const static int MODES_NUMBER = 2;
        const int _modes[MODES_NUMBER] =
        {
            MODE_COLOR,
            MODE_DEPTH
        };
        const FrameSourceTypes _frame_source_types[MODES_NUMBER] =
        {
            FrameSourceTypes::FrameSourceTypes_Color,
            FrameSourceTypes::FrameSourceTypes_Depth
        };
        const std::string _extension = "mkv";
        const std::string _codec_names[MODES_NUMBER] =
        {
            "wmv2",
            "ffv1"
        };
        const std::string _pix_fmt_names[MODES_NUMBER] =
        {
            "yuv420p",
            "gray16"
        };
        const std::string _windowNames[MODES_NUMBER] =
        {
            "Color",
            "Depth",
        };
        const static int DEFAULT_FPS = 29;
        const std::string DEFAULT_DIRECTORY_PATH = ".\\";
        bool _active;
        bool _writing;
        bool _modesActivity[MODES_NUMBER] =
        {
            false,
            false
        };
        bool _oldModesActivity[MODES_NUMBER] =
        {
            false,
            false
        };
        kinect2reader::Kinect2Wrapper * _pKinect2Wrapper;
        MatStream * _pFrameStreams [MODES_NUMBER] =
        {
            nullptr,
            nullptr
        };
        int _fps;
        std::string _directoryPath;
        std::string _lastPath;
        video_io::VideoWriter * _pVideoWriter;
		Kinect2RecorderLogger& _logger;
		int _mseconds;
        QElapsedTimer _elapsedTimer;
        bool _isTimer;
        std::mutex _mutex;

        /* USE ONLY INSIDE OF _mutex.lock() and _mutex.unlock() OR IN DESTRUCTOR */
        void InnerDeactivate();
        /*************************************************************************/

		/* USE ONLY INSIDE OF _mutex.lock() and _mutex.unlock() */
		bool InnerStart();
		bool InnerStop();
		/********************************************************/

    public:
        const static int MODE_NONE = 0;
        const static int MODE_COLOR = 1;
        const static int MODE_DEPTH = 2;
        Kinect2Recorder(Kinect2RecorderLogger& kinect2RecorderLogger);
        ~Kinect2Recorder();
        bool IsActive();
        void Update();
        void SetDirectoryPath(std::string directoryPath);
        void SetMode(int mode);
        void SetSize(int mode, int width, int height);
        void SetFPS(int fps);
		void Start();
		void Start(int seconds);
        void Stop();
        void Deactivate();
    };

}


