/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once

class MessageException
{
private:
    const char * _message;
public:
    MessageException(const char * message);
    ~MessageException();
    const char * GetExceptionMessage();
};
