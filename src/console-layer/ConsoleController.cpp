/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "ConsoleController.h"
#include <vector>
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>

using namespace std;

ConsoleController::ConsoleController(Kinect2Recorder * pKinect2Recorder):
    _pKinect2Recorder(nullptr)
{
    if (pKinect2Recorder == nullptr)
    {
        return;
    }
    _pKinect2Recorder = pKinect2Recorder;
}

ConsoleController::~ConsoleController()
{
    _pKinect2Recorder = nullptr;
}

bool ConsoleController::IsActive()
{
    return _pKinect2Recorder != nullptr && _pKinect2Recorder->IsActive();
}

void ConsoleController::Command(const string commandLine)
{
    vector<string> * args = ParseLine(commandLine);
    size_t argc = args->size();
    if (argc <= 0)
    {
        delete(args);
        args = nullptr;
        return;
    }
    std::string command = args->at(0);

        if (command.compare(COMMAND_SET_DIR) == 0)
        {
            if (argc == 2)
            {
                _pKinect2Recorder->SetDirectoryPath(args->at(1));
            }
        }
        if (command.compare(COMMAND_SET_MODE) == 0)
        {
            int mode = 0;
            int i = 1;
            while (i < argc)
            {
                if (args->at(i).compare(MODE_COLOR) == 0)
                {
                    mode = mode | Kinect2Recorder::MODE_COLOR;
                }
                if (args->at(i).compare(MODE_DEPTH) == 0)
                {
                    mode = mode | Kinect2Recorder::MODE_DEPTH;
                }
                _pKinect2Recorder->SetMode(mode);
                i++;
            }
        }
        if (command.compare(COMMAND_SET_SIZE) == 0)
        {
            if (argc == 4)
            {
                int mode = 0;
                if (args->at(1).compare(MODE_COLOR) == 0)
                {
                    mode = mode | Kinect2Recorder::MODE_COLOR;
                }
                if (args->at(1).compare(MODE_DEPTH) == 0)
                {
                    mode = mode | Kinect2Recorder::MODE_DEPTH;
                }
                _pKinect2Recorder->SetSize(mode, std::stoi(args->at(2)), std::stoi(args->at(3)));
            }
        }
        if (command.compare(COMMAND_SET_FPS) == 0)
        {
            if (argc == 2)
            {
                try
                {
                    _pKinect2Recorder->SetFPS(std::stoi(args->at(1)));
                }
                catch(...)
                {
                }
            }
        }
        if (command.compare(COMMAND_START) == 0)
        {
            if (argc == 1)
            {
                _pKinect2Recorder->Start();
            }
            if (argc == 2)
            {
                try
                {
                    _pKinect2Recorder->Start(std::stoi(args->at(1)));
                }
                catch(...)
                {
                }
            }
        }
        if (command.compare(COMMAND_STOP) == 0)
        {
            _pKinect2Recorder->Stop();
        }
        if (command.compare(COMMAND_END) == 0)
        {
            _pKinect2Recorder->Deactivate();
        }
    delete(args);
    args = nullptr;
}

vector<string> * ConsoleController::ParseLine(const string line)
{
    vector<string> * pVectorArgs = new vector<string>();
    int start = 0;
    int finish = 0;
    while(finish < line.length())
    {
        if (isspace(line[finish]))
        {
            if (start < finish)
            {
                pVectorArgs->push_back(line.substr(start, finish - start));
            }
            start = finish + 1;
        }
        finish++;
    }
    if (start < finish)
    {
        string arg = line.substr(start, finish - start);
        pVectorArgs->push_back(arg);
    }
    return pVectorArgs;
}
