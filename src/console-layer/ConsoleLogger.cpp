/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "ConsoleLogger.h"
#include <string>

ConsoleLogger::ConsoleLogger() : Kinect2RecorderLogger()
{
}

ConsoleLogger::~ConsoleLogger()
{
}

void ConsoleLogger::LogFailedWhenNotActive()
{
	std::cout << LOG_PREFIX << "Kinect2Recorder is not active" << std::endl;
}

void ConsoleLogger::LogFailedWhenWritingOn()
{
	std::cout << LOG_PREFIX << "Action is incorrect when writing on" << std::endl;
}

void ConsoleLogger::LogFailedWhenWritingOff()
{
	std::cout << LOG_PREFIX << "Action is incorrect when writing off" << std::endl;
}

void ConsoleLogger::LogInit()
{
	std::cout << LOG_PREFIX << "Kinect2Recorder has been initialized" << std::endl;
}

void ConsoleLogger::LogFailedInit()
{
	std::cout << LOG_PREFIX << "Kinect2Recorder initialization has failed" << std::endl;
}

void ConsoleLogger::LogNotActive()
{
	std::cout << LOG_PREFIX << "Kinect2Record becomes not active" << std::endl;
}

void ConsoleLogger::LogDirectoryPath(const std::string& path)
{
	std::cout << LOG_PREFIX << "New directory path " << path << std::endl;
}

void ConsoleLogger::LogSetMode()
{
	std::cout << LOG_PREFIX << "Success" << std::endl;
}

void ConsoleLogger::LogFailedSetMode()
{
	std::cout << LOG_PREFIX << "Setting has failed" << std::endl;
}

void ConsoleLogger::LogSetSize()
{
	std::cout << LOG_PREFIX << "Success" << std::endl;
}

void ConsoleLogger::LogFailedSetSize()
{
	std::cout << LOG_PREFIX << "Setting has failed" << std::endl;
}

void ConsoleLogger::LogSetFPS()
{
    std::cout << LOG_PREFIX << "Success" << std::endl;
}

void ConsoleLogger::LogSetFailedFPSWhenIncorrectValue()
{
    std::cout << LOG_PREFIX << "Failed FPS setting, incorrect value" << std::endl;
}

void ConsoleLogger::LogKinectOff()
{
	std::cout << LOG_PREFIX << "Error: failed Kinect2Wrapper Update" << std::endl;
	std::cout << LOG_PREFIX << "Check Kinect2 connection" << std::endl;
}

void ConsoleLogger::LogFailedWrite(const std::string& path, int modeNumber)
{
	std::cout << LOG_PREFIX << "Failed frame writing, path: " << path << " mode: " << modeNumber << std::endl;
}

void ConsoleLogger::LogStart(const std::string& path)
{
	std::cout << LOG_PREFIX << "Writing ON, path = " << path << std::endl;
}

void ConsoleLogger::LogFailedStartWhenNoneMode()
{
	std::cout << LOG_PREFIX << "Nothing to write" << std::endl;

}

void ConsoleLogger::LogFailedStartWhenOpenWithPath(const std::string& path)
{
	std::cout << LOG_PREFIX << "Failed VideoWriter openning with path: " << path << std::endl;
}

void ConsoleLogger::LogFailedStartWhenSetInnerMetadata()
{
	std::cout << LOG_PREFIX << "Failed start: inner problem" << std::endl;
}

void ConsoleLogger::LogFailedStartWhenAddVideoStream(const std::string& path, int modeNumber)
{
	std::cout << LOG_PREFIX << "Failed start in adding videoStream, path: " << path << " modeNumber: " << modeNumber << std::endl;
}

void ConsoleLogger::LogStop(const std::string& path)
{
	std::cout << LOG_PREFIX << "Writing OFF" << std::endl;
	std::cout << LOG_PREFIX << "New video: " << path << std::endl;
}

void ConsoleLogger::LogFailedStopWhenClose(const std::string& path)
{
	std::cout << LOG_PREFIX << "Failed Stop in videoWriter closing, path: " << path << std::endl;

}

void ConsoleLogger::LogFailedStartWhenIncorrectTime()
{
    std::cout << LOG_PREFIX << "Failed Start because time is incorrect" << std::endl;

}
