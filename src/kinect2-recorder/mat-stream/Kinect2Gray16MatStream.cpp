/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "Kinect2Gray16MatStream.h"
#include "MatStreamInitException.h"
#include "kinect2-reader/Kinect2SafeRelease.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace kinect2recorder
{

    Gray16MatStream::Gray16MatStream(kinect2reader::Kinect2Accessor * pKinect2Accessor) :
        _buffer(nullptr),
        _pKinect2Accessor(nullptr),
        _size(cv::Size(WIDTH, HEIGHT))
    {
        if (pKinect2Accessor == nullptr)
        {
            throw MatStreamInitException("Gray16MatStream::Gray16MatStream: pKinect2Accessor is nullptr");
        }
        _pKinect2Accessor = pKinect2Accessor;
        _buffer = new UINT16[BUFFER_LENGTH];
    }

    Gray16MatStream::~Gray16MatStream()
    {
        _pKinect2Accessor = nullptr;
        if (_buffer != nullptr)
        {
            delete[](_buffer);
            _buffer = nullptr;
        }
    }

    cv::Mat * Gray16MatStream::GetMat()
    {
        IDepthFrame * pDepthFrame = _pKinect2Accessor->GetDepthFrame();
        if (pDepthFrame == nullptr)
        {
            return nullptr;
        }
        IFrameDescription * pFrameDescription = nullptr;
        int width = 0;
        int height = 0;
        bool correct = false;
        HRESULT hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
        if (SUCCEEDED(hr))
        {
            hr = pFrameDescription->get_Width(&width);
        }
        if (SUCCEEDED(hr))
        {
            hr = pFrameDescription->get_Height(&height);
        }
        SafeRelease(pFrameDescription);
        pFrameDescription = nullptr;
        if (SUCCEEDED(hr) && width == WIDTH && height == HEIGHT)
        {
            UINT16 * tmpBuffer = nullptr;
            UINT bufferSize = 0;
            hr = pDepthFrame->AccessUnderlyingBuffer(&bufferSize, &tmpBuffer);
            if (SUCCEEDED(hr) && bufferSize == BUFFER_LENGTH)
            {
                memcpy(_buffer, tmpBuffer, BUFFER_SIZE);
                correct = true;
            }
            tmpBuffer = nullptr;
        }
        if (correct)
        {
            cv::Mat * pMat = new cv::Mat(HEIGHT, WIDTH, CV_16UC1, reinterpret_cast<void*>(_buffer));
            cv::resize(*pMat, *pMat, _size);
            return pMat;
        }
        return nullptr;
    }

    void Gray16MatStream::SetSize(cv::Size size)
    {
        _size = size;
    }

    int Gray16MatStream::GetWidth()
    {
        return _size.width;
    }

    int Gray16MatStream::GetHeight()
    {
        return _size.height;
    }

}
