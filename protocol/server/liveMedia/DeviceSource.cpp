/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A template for a MediaSource encapsulating an audio/video input device
//
// NOTE: Sections of this code labeled "%%% TO BE WRITTEN %%%" are incomplete, and need to be written by the programmer
// (depending on the features of the particular device).
// Implementation

#include "DeviceSource.hh"
#include <GroupsockHelper.hh> // for "gettimeofday()"
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>

#define returnIfError(x) \
    if (FAILED(x))\
    {\
        printf("FAIL");\
        return;\
    }

// buffers for stream
static uint8_t* buf; // [8192]; // try star and not star
int DDI_stream;

DeviceSource*
DeviceSource::createNew(UsageEnvironment& env) {
    static DeviceSource* source = new DeviceSource(env);
  return source; // not using parameters
}

EventTriggerId DeviceSource::eventTriggerId = 0;

unsigned DeviceSource::referenceCount = 0;

DeviceSource::DeviceSource(UsageEnvironment& env):FramedSource(env) {
  if (referenceCount == 0) {
      // Any global initialization of the device would be done here:
      HRESULT hr = S_OK;

      // Driver types supported
      D3D_DRIVER_TYPE DriverTypes[] =
              {
                      D3D_DRIVER_TYPE_HARDWARE,
                      D3D_DRIVER_TYPE_WARP,
                      D3D_DRIVER_TYPE_REFERENCE,
              };
      UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

      // Feature levels supported
      D3D_FEATURE_LEVEL FeatureLevels[] =
              {
                      D3D_FEATURE_LEVEL_11_0,
                      D3D_FEATURE_LEVEL_10_1,
                      D3D_FEATURE_LEVEL_10_0,
                      D3D_FEATURE_LEVEL_9_1
              };
      UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
      D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;

      // Create device
      for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
      {
          hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, /*D3D11_CREATE_DEVICE_DEBUG*/0, FeatureLevels, NumFeatureLevels,
                                 D3D11_SDK_VERSION, &pD3DDev, &FeatureLevel, &pCtx);
          if (SUCCEEDED(hr))
          {
              // Device creation succeeded, no need to loop anymore
              break;
          }
      }
      returnIfError(hr);

      if (!pDDAWrapper)
      {
          std::cout << "INIT THE USELESS" << std::endl;
          pDDAWrapper = new DDAImpl(pD3DDev, pCtx);
          hr = pDDAWrapper->Init();
          // returnIfError(hr);
      }
      returnIfError(hr);

      if (!pEnc)
      {
          DWORD w = bNoVPBlt ? pDDAWrapper->getWidth() : encWidth;
          DWORD h = bNoVPBlt ? pDDAWrapper->getHeight() : encHeight;
          NV_ENC_BUFFER_FORMAT fmt = bNoVPBlt ? NV_ENC_BUFFER_FORMAT_ARGB : NV_ENC_BUFFER_FORMAT_NV12;
          pEnc = new NvEncoderD3D11(pD3DDev, w, h, fmt);
          if (!pEnc)
          {
              returnIfError(E_FAIL);
          }

          ZeroMemory(&encInitParams, sizeof(encInitParams));
          ZeroMemory(&encConfig, sizeof(encConfig));
          encInitParams.encodeConfig = &encConfig;
          encInitParams.encodeWidth = w;
          encInitParams.encodeHeight = h;
          encInitParams.maxEncodeWidth = pDDAWrapper->getWidth();
          encInitParams.maxEncodeHeight = pDDAWrapper->getHeight();
          encConfig.gopLength = 5;

          try
          {
              pEnc->CreateDefaultEncoderParams(&encInitParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_LOW_LATENCY_HP_GUID);
              pEnc->CreateEncoder(&encInitParams);

          }
          catch (...)
          {
              returnIfError(E_FAIL);
          }
      }
      returnIfError(hr);

      if (!pColorConv)
      {
          pColorConv = new RGBToNV12(pD3DDev, pCtx);
          HRESULT hr = pColorConv->Init();
          returnIfError(hr);
      }
      returnIfError(hr);
  }
  ++referenceCount;

  // Any instance-specific initialization of the device would be done here:
  // No instance-specific initialization necessary

  // We arrange here for our "deliverFrame" member function to be called
  // whenever the next frame of data becomes available from the device.
  //
  // If the device can be accessed as a readable socket, then one easy way to do this is using a call to
  //     envir().taskScheduler().turnOnBackgroundReadHandling( ... )
  // (See examples of this call in the "liveMedia" directory.)
  //
  std::cout << "Test1" << std::endl;
  // If, however, the device *cannot* be accessed as a readable socket, then instead we can implement it using 'event triggers':
  // Create an 'event trigger' for this device (if it hasn't already been done):
  if (eventTriggerId == 0) {
    eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
  }
}

DeviceSource::~DeviceSource() {
  // Any instance-specific 'destruction' (i.e., resetting) of the device would be done here:
  envir().taskScheduler().deleteEventTrigger(eventTriggerId);
  eventTriggerId = 0;

  --referenceCount;
  if (referenceCount == 0) {
    // Nothing global destruction
  }
}

void DeviceSource::doGetNextFrame() {
    start:
    // This function is called (by our 'downstream' object) when it asks for new data.
    vPacket.clear();
    static const int WAIT_BASE = 20;
    HRESULT hr = S_OK;
    LARGE_INTEGER start = { 0 };
    LARGE_INTEGER end = { 0 };
    LARGE_INTEGER interval = { 0 };
    LARGE_INTEGER freq = { 0 };
    int wait = WAIT_BASE;

    QueryPerformanceFrequency(&freq);

    std::cout << "Test2" << std::endl;

    // Reset waiting time for the next screen capture attempt
#define RESET_WAIT_TIME(start, end, interval, freq)         \
    QueryPerformanceCounter(&end);                          \
    interval.QuadPart = end.QuadPart - start.QuadPart;      \
    MICROSEC_TIME(interval, freq);                          \
    wait = (int)(WAIT_BASE - (interval.QuadPart * 1000));
    // get start timestamp.
    // use this to adjust the waiting period in each capture attempt to approximately attempt 60 captures in a second
    QueryPerformanceCounter(&start);
    // Get a frame from DDA
    hr = pDDAWrapper->GetCapturedFrame(&pDupTex2D, wait); // Release after preproc
    if (FAILED(hr))
    {
        std::cout << "Test3" << std::endl;
        failCount++;
    }
    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
    {
        std::cout << "Capture timeout" << std::endl;
        goto start;
        //handleClosure();
        return;
    }
    else
    {
        if (FAILED(hr))
        {
            handleClosure();
            return;
        }
        RESET_WAIT_TIME(start, end, interval, freq);
        // Preprocess for encoding

        const NvEncInputFrame *pEncInput = pEnc->GetNextInputFrame();
        pEncBuf = (ID3D11Texture2D *)pEncInput->inputPtr;
        if (bNoVPBlt)
        {
            pCtx->CopySubresourceRegion(pEncBuf, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, pDupTex2D, 0, NULL);
        }
        else
        {
            hr = pColorConv->Convert(pDupTex2D, pEncBuf);
        }
        SAFE_RELEASE(pDupTex2D);
        // return;

        pEncBuf->AddRef();  // Release after encode

        if (FAILED(hr))
        {
            printf("Preproc failed with error 0x%08x\n", hr);
            buf = nullptr;
            handleClosure();
            return;
        }
        try
        {
            pEnc->EncodeFrame(vPacket);
            std::cout << "Size of capture: " << (vPacket.empty() ? 0 : vPacket[0].size()) << std::endl;
            buf = vPacket.empty() ? nullptr : vPacket[0].data();
        }
        catch (...)
        {
            handleClosure();
            return;
        }
    }
    if(buf == nullptr) {
        handleClosure();
        return;
    }
    // Note: If, for some reason, the source device stops being readable (e.g., it gets closed), then you do the following:
    // if (0 /* the source stops being readable */ /*%%% TO BE WRITTEN %%%*/) {
    //  handleClosure();
    //  return;
    // }

    // If a new frame of data is immediately available to be delivered, then do this now:
    // if (0 /* a new frame of data is immediately available to be delivered*/ /*%%% TO BE WRITTEN %%%*/) {
    if(!vPacket.empty())
        deliverFrame();
    // }

    // No new data is immediately available to be delivered.  We don't do anything more here.
    // Instead, our event trigger must be called (e.g., from a separate thread) when new data becomes available.
}

void DeviceSource::deliverFrame0(void* clientData) {
  ((DeviceSource*)clientData)->deliverFrame();
}

void DeviceSource::deliverFrame() {
  // This function is called when new frame data is available from the device.
  // We deliver this data by copying it to the 'downstream' object, using the following parameters (class members):
  // 'in' parameters (these should *not* be modified by this function):
  //     fTo: The frame data is copied to this address.
  //         (Note that the variable "fTo" is *not* modified.  Instead,
  //          the frame data is copied to the address pointed to by "fTo".)
  //     fMaxSize: This is the maximum number of bytes that can be copied
  //         (If the actual frame is larger than this, then it should
  //          be truncated, and "fNumTruncatedBytes" set accordingly.)
  // 'out' parameters (these are modified by this function):
  //     fFrameSize: Should be set to the delivered frame size (<= fMaxSize).
  //     fNumTruncatedBytes: Should be set iff the delivered frame would have been
  //         bigger than "fMaxSize", in which case it's set to the number of bytes
  //         that have been omitted.
  //     fPresentationTime: Should be set to the frame's presentation time
  //         (seconds, microseconds).  This time must be aligned with 'wall-clock time' - i.e., the time that you would get
  //         by calling "gettimeofday()".
  //     fDurationInMicroseconds: Should be set to the frame's duration, if known.
  //         If, however, the device is a 'live source' (e.g., encoded from a camera or microphone), then we probably don't need
  //         to set this variable, because - in this case - data will never arrive 'early'.
  // Note the code below.

  if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet

  u_int8_t* newFrameDataStart = (u_int8_t*) buf + 4; // defined buffer at the beginning of the file
  unsigned newFrameSize = vPacket[0].size() - 4; // defined buffer at the beginning of the file

  // Deliver the data here:
  if (newFrameSize > fMaxSize) {
      std::cout << "MAX SIZE: " << fMaxSize << std::endl;
    fFrameSize = fMaxSize;
    fNumTruncatedBytes = newFrameSize - fMaxSize;
  } else {
    fFrameSize = newFrameSize;
  }
  gettimeofday(&fPresentationTime, NULL); // If you have a more accurate time - e.g., from an encoder - then use that instead.
  std::cout << fFrameSize << std::endl;
  // If the device is *not* a 'live source' (e.g., it comes instead from a file or buffer), then set "fDurationInMicroseconds" here.
  memmove(fTo, newFrameDataStart, fFrameSize);
  // After delivering the data, inform the reader that it is now available:
  FramedSource::afterGetting(this);
}


// The following code would be called to signal that a new frame of data has become available.
// This (unlike other "LIVE555 Streaming Media" library code) may be called from a separate thread.
// (Note, however, that "triggerEvent()" cannot be called with the same 'event trigger id' from different threads.
// Also, if you want to have multiple device threads, each one using a different 'event trigger id', then you will need
// to make "eventTriggerId" a non-static member variable of "DeviceSource".)
void signalNewFrameData() {
  TaskScheduler* ourScheduler = NULL; // Not needed for us
  DeviceSource* ourDevice  = NULL; // Not needed for us

  if (ourScheduler != NULL) { // sanity check
    ourScheduler->triggerEvent(DeviceSource::eventTriggerId, ourDevice);
  }
}
