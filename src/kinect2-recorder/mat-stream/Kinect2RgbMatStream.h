/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include "MatStream.h"
#include "kinect2-reader/Kinect2Accessor.h"

namespace kinect2recorder
{

    class RgbMatStream : public MatStream
    {
    private:
        const static int WIDTH = 1920;
        const static int HEIGHT = 1080;
        const static int BUFFER_LENGTH = WIDTH * HEIGHT * sizeof(RGBQUAD);
        const static int BUFFER_SIZE = BUFFER_LENGTH;
        BYTE * _buffer;
        kinect2reader::Kinect2Accessor * _pKinect2Accessor;
        cv::Size _size;
    public:
        RgbMatStream(kinect2reader::Kinect2Accessor * pKinect2Accessor);
        ~RgbMatStream();
        cv::Mat * GetMat();
        void SetSize(cv::Size size);
        int GetWidth();
        int GetHeight();
    };

}
