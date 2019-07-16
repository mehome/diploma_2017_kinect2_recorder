/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at 
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include <opencv2/core/core.hpp>

namespace kinect2recorder
{

    class MatStream
    {
    public:
        MatStream() {}
        virtual cv::Mat * GetMat() = 0;
        virtual void SetSize(cv::Size size) = 0;
        virtual int GetWidth() = 0;
        virtual int GetHeight() = 0;
    };

}
