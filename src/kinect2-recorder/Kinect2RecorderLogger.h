/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#pragma once
#include <iostream>

namespace kinect2recorder
{
	class Kinect2RecorderLogger
	{
	public:
		Kinect2RecorderLogger() {}
		virtual void LogFailedWhenNotActive() = 0;
		virtual void LogFailedWhenWritingOn() = 0;
		virtual void LogFailedWhenWritingOff() = 0;
		virtual void LogInit() = 0;
		virtual void LogFailedInit() = 0;
		virtual void LogNotActive() = 0;
		virtual void LogDirectoryPath(const std::string& path) = 0;
		virtual void LogSetMode() = 0;
		virtual void LogFailedSetMode() = 0;
		virtual void LogSetSize() = 0;
		virtual void LogFailedSetSize() = 0;
        virtual void LogSetFPS() = 0;
        virtual void LogSetFailedFPSWhenIncorrectValue() = 0;
		virtual void LogKinectOff() = 0;
		virtual void LogFailedWrite(const std::string& path, int modeNumber) = 0;
		virtual void LogStart(const std::string& path) = 0;
		virtual void LogFailedStartWhenNoneMode() = 0;
        virtual void LogFailedStartWhenIncorrectTime() = 0;
		virtual void LogFailedStartWhenOpenWithPath(const std::string& path) = 0;
		virtual void LogFailedStartWhenSetInnerMetadata() = 0;
		virtual void LogFailedStartWhenAddVideoStream(const std::string& path, int modeNumber) = 0;
		virtual void LogStop(const std::string& path) = 0;
		virtual void LogFailedStopWhenClose(const std::string& path) = 0;
	};
}
