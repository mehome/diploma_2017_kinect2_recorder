/*
* Copyright (c) 2017 Alexander Menkin
* Use of this source code is governed by an MIT-style license that can be found in the LICENSE file at
* https://github.com/miloiloloo/diploma_2017_kinect2_recorder
*/

#include "Kinect2Wrapper.h"
#include "Kinect2SafeRelease.h"

namespace kinect2reader
{

    Kinect2Wrapper::Kinect2Wrapper() :
        _pKinectSensor(nullptr),
        _pMultiSourceFrameReader(nullptr),
        _pColorFrame(nullptr),
        _pDepthFrame(nullptr)
    {
        HRESULT hr = GetDefaultKinectSensor(&_pKinectSensor);
        if (FAILED(hr))
        {
            _pKinectSensor = nullptr;
            throw Kinect2WrapperInitException();
        }
        hr = _pKinectSensor->Open();
        if (FAILED(hr))
        {
            SafeRelease(_pKinectSensor);
            _pKinectSensor = nullptr;
            throw Kinect2WrapperInitException();
        }
    }

    Kinect2Wrapper::~Kinect2Wrapper()
    {
        SafeRelease(_pColorFrame);
        _pColorFrame = nullptr;
        SafeRelease(_pDepthFrame);
        _pDepthFrame = nullptr;
        SafeRelease(_pMultiSourceFrameReader);
        _pMultiSourceFrameReader = nullptr;
        SafeRelease(_pKinectSensor);
        _pKinectSensor = nullptr;
    }

    IColorFrame * Kinect2Wrapper::GetColorFrame()
    {
        return _pColorFrame;
    }

    IDepthFrame * Kinect2Wrapper::GetDepthFrame()
    {
        return _pDepthFrame;
    }

    void Kinect2Wrapper::SetFrameSourceTypes(DWORD frameSourceTypes)
    {
        SafeRelease(_pColorFrame);
        _pColorFrame = nullptr;
        SafeRelease(_pDepthFrame);
        _pDepthFrame = nullptr;
        SafeRelease(_pMultiSourceFrameReader);
        _pMultiSourceFrameReader = nullptr;
        if (frameSourceTypes == FrameSourceTypes::FrameSourceTypes_None)
        {
            return;
        }
        HRESULT hr = _pKinectSensor->OpenMultiSourceFrameReader(frameSourceTypes, &_pMultiSourceFrameReader);
        if (FAILED(hr))
        {
            _pMultiSourceFrameReader = nullptr;
            throw Kinect2WrapperFailedException();
        }
    }

    void Kinect2Wrapper::Update()
    {
        SafeRelease(_pColorFrame);
        _pColorFrame = nullptr;
        SafeRelease(_pDepthFrame);
        _pDepthFrame = nullptr;
        if (_pMultiSourceFrameReader == nullptr)
        {
            return;
        }
        IMultiSourceFrame * pMultiSourceFrame = nullptr;
        HRESULT hr = _pMultiSourceFrameReader->AcquireLatestFrame(&pMultiSourceFrame);
        if (FAILED(hr)) {
            pMultiSourceFrame = nullptr;
            return;
        }
        IColorFrameReference* pColorFrameReference = nullptr;
        hr = pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
        if (SUCCEEDED(hr))
        {
            hr = pColorFrameReference->AcquireFrame(&_pColorFrame);
            if (FAILED(hr))
            {
                _pColorFrame = nullptr;
            }
        }
        SafeRelease(pColorFrameReference);
        pColorFrameReference = nullptr;
        IDepthFrameReference* pDepthFrameReference = nullptr;
        hr = pMultiSourceFrame->get_DepthFrameReference(&pDepthFrameReference);
        {
            hr = pDepthFrameReference->AcquireFrame(&_pDepthFrame);
            if (FAILED(hr))
            {
                _pDepthFrame = nullptr;
            }
        }
        SafeRelease(pDepthFrameReference);
        pDepthFrameReference = nullptr;
        SafeRelease(pMultiSourceFrame);
        pMultiSourceFrame = nullptr;
    }

}
