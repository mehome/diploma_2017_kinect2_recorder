/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include "kinect2-recorder\Kinect2RecorderLogger.h"
#include <iostream>

class ConsoleLogger : public kinect2recorder::Kinect2RecorderLogger
{
private:
	const char * const LOG_PREFIX = "LOG: ";
public:
	ConsoleLogger();
	~ConsoleLogger();
	void LogFailedWhenNotActive();
	void LogFailedWhenWritingOn();
	void LogFailedWhenWritingOff();
	void LogInit();
	void LogFailedInit();
	void LogNotActive();
	void LogDirectoryPath(const std::string& path);
	void LogSetMode();
	void LogFailedSetMode();
	void LogSetSize();
	void LogFailedSetSize();
    void LogSetFPS();
    void LogSetFailedFPSWhenIncorrectValue();
	void LogKinectOff();
	void LogFailedWrite(const std::string& path, int modeNumber);
	void LogStart(const std::string& path);
    void LogFailedStartWhenIncorrectTime();
	void LogFailedStartWhenNoneMode();
	void LogFailedStartWhenOpenWithPath(const std::string& path);
	void LogFailedStartWhenSetInnerMetadata();
	void LogFailedStartWhenAddVideoStream(const std::string& path, int modeNumber);
	void LogStop(const std::string& path);
	void LogFailedStopWhenClose(const std::string& path);
};

