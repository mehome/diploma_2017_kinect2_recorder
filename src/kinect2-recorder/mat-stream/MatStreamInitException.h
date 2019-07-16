/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include "message-exception/MessageException.h"

namespace kinect2recorder
{

    class MatStreamInitException : public MessageException
    {
    public:
        MatStreamInitException(const char * message) : MessageException(message) {}
    };

}
