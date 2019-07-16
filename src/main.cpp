/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "console-layer/ConsoleController.h"
#include "console-layer/ConsoleLogger.h"
#include "kinect2-recorder/Kinect2Recorder.h"
#include <thread>

/* To read console */
void ConsoleReaderThreadFunction(ConsoleController * pConsoleController)
{
    if (pConsoleController == nullptr)
    {
		throw;
    }
    string line;
    while(pConsoleController->IsActive())
    {
        std::getline(cin, line);
        pConsoleController->Command(line);
    }
}

int main()
{
    Kinect2Recorder * kinect2Recorder = new Kinect2Recorder(ConsoleLogger());
    ConsoleController * cc = new ConsoleController(kinect2Recorder);
    std::thread thr(ConsoleReaderThreadFunction, cc);
    kinect2Recorder->SetSize(Kinect2Recorder::MODE_COLOR, 960, 540);
    kinect2Recorder->SetSize(Kinect2Recorder::MODE_DEPTH, 512, 424);
    try
    {
        while (kinect2Recorder->IsActive())
        {
                kinect2Recorder->Update();
        }
    }
    catch(...)
    {
        try
        {
            kinect2Recorder->Deactivate();
        }
        catch(...)
        {
        }
    }
    thr.join();
    delete(cc);
    delete(kinect2Recorder);
    return 0;
}
