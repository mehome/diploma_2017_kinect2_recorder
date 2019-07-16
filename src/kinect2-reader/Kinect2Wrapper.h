/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include "Kinect2Accessor.h"
#include "Kinect2WrapperInitException.h"
#include "Kinect2WrapperFailedException.h"

namespace kinect2reader
{

    class Kinect2Wrapper : public Kinect2Accessor
    {
    private:
        IKinectSensor * _pKinectSensor;
        IMultiSourceFrameReader * _pMultiSourceFrameReader;
        IColorFrame * _pColorFrame;
        IDepthFrame * _pDepthFrame;
    public:
        Kinect2Wrapper();
        ~Kinect2Wrapper();
        IColorFrame * GetColorFrame();
        IDepthFrame * GetDepthFrame();
        void SetFrameSourceTypes(DWORD frameSourceTypes);
        void Update();
    };

}
