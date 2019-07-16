/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "MessageException.h"

MessageException::MessageException(const char * message) :
    _message(message)
{
}

MessageException::~MessageException()
{
    _message = nullptr;
}

const char * MessageException::GetExceptionMessage()
{
    return _message;
}
