/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include<Kinect.h>

namespace kinect2reader {

    class Kinect2Accessor
    {
    public:
		Kinect2Accessor() {}
        virtual IColorFrame * GetColorFrame() = 0;
        virtual IDepthFrame * GetDepthFrame() = 0;
    };

}
