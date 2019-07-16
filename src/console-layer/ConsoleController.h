/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include <iostream>
#include <vector>
#include "kinect2-recorder/Kinect2Recorder.h"

using namespace std;
using namespace kinect2recorder;

class ConsoleController
{
private:

    const string COMMAND_SET_DIR = "dir";
    const string COMMAND_SET_MODE = "mode";
    const string COMMAND_SET_SIZE = "size";
    const string COMMAND_SET_FPS = "fps";
    const string COMMAND_START = "start";
    const string COMMAND_STOP = "stop";
    const string COMMAND_END = "end";
    const string MODE_COLOR = "c";
    const string MODE_DEPTH = "d";
    Kinect2Recorder * _pKinect2Recorder;
    vector<string> * ParseLine(const string line);
public:    
    ConsoleController(Kinect2Recorder * pKinect2Recorder);
    ~ConsoleController();
    bool IsActive();
    void Command(string commandLine);
};
