/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "Kinect2RgbMatStream.h"
#include "MatStreamInitException.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace kinect2recorder
{

    RgbMatStream::RgbMatStream(kinect2reader::Kinect2Accessor * pKinect2Accessor) :
        _buffer(nullptr),
        _pKinect2Accessor(nullptr),
        _size(cv::Size(WIDTH, HEIGHT))
    {
        if (pKinect2Accessor == nullptr)
        {
            throw new MatStreamInitException("RgbMatStream::RgbMatStream: pKinect2Accessor is nullptr");
        }
        _pKinect2Accessor = pKinect2Accessor;
        _buffer = new BYTE[BUFFER_LENGTH];
    }

    RgbMatStream::~RgbMatStream()
    {
        _pKinect2Accessor = nullptr;
        if (_buffer != nullptr)
        {
            delete[](_buffer);
            _buffer = nullptr;
        }
    }

    cv::Mat * RgbMatStream::GetMat()
    {
        IColorFrame * pColorFrame = _pKinect2Accessor->GetColorFrame();
        if (pColorFrame == nullptr)
        {
            return nullptr;
        }
        IFrameDescription * pFrameDescription = nullptr;
        int width = 0;
        int height = 0;
        ColorImageFormat imageFormat = ColorImageFormat_None;
        HRESULT hr;
        bool correct = false;
        hr = pColorFrame->get_FrameDescription(&pFrameDescription);
        if (SUCCEEDED(hr))
        {
            hr = pFrameDescription->get_Width(&width);
        }
        if (SUCCEEDED(hr))
        {
            hr = pFrameDescription->get_Height(&height);
        }
        if (SUCCEEDED(hr))
        {
            hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
        }
        if (SUCCEEDED(hr) && width == WIDTH && height == HEIGHT)
        {
            if (imageFormat == ColorImageFormat_Bgra)
            {
                BYTE * tmpBuffer = nullptr;
                UINT bufferSize = 0;
                hr = pColorFrame->AccessRawUnderlyingBuffer(&bufferSize, &tmpBuffer);
                if (SUCCEEDED(hr) && bufferSize == BUFFER_LENGTH)
                {
                    memcpy(_buffer, tmpBuffer, bufferSize);
                    correct = true;
                }
                tmpBuffer = nullptr;
            }
            else
            {
                hr = pColorFrame->CopyConvertedFrameDataToArray(BUFFER_SIZE, _buffer, ColorImageFormat_Bgra);
                correct = true;
            }
        }
        if (correct)
        {
            cv::Mat * pMat = new cv::Mat(HEIGHT, WIDTH, CV_8UC4, reinterpret_cast<void*>(_buffer));
            cv::resize(*pMat, *pMat, _size);
            cv::cvtColor(*pMat, *pMat, CV_RGBA2RGB);
            return pMat;
        }
        return nullptr;
    }

    void RgbMatStream::SetSize(cv::Size size)
    {
        _size = size;
    }

    int RgbMatStream::GetWidth()
    {
        return _size.width;
    }

    int RgbMatStream::GetHeight()
    {
        return _size.height;
    }

}
