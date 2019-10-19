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
// C++ header

#ifndef _DEVICE_SOURCE_HH
#define _DEVICE_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

#pragma comment(lib,"d3d11.lib") // include DirectX 11 lib
#include "Defs.h"
#include "DDAImpl.h"
#include "Preproc.h"
#include <NvEncoderD3D11.h>

// The following class can be used to define specific encoder parameters
class DeviceParameters {
  // Nothing needed here for us
};

class DeviceSource: public FramedSource {
public:
  static DeviceSource* createNew(UsageEnvironment& env);

public:
  static EventTriggerId eventTriggerId;
  // Note that this is defined here to be a static class variable, because this code is intended to illustrate how to
  // encapsulate a *single* device - not a set of devices.
  // You can, however, redefine this to be a non-static member variable.

protected:
  DeviceSource(UsageEnvironment& env);
  // called only by createNew(), or by subclass constructors
  virtual ~DeviceSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  //virtual void doStopGettingFrames(); // optional

private:
  static void deliverFrame0(void* clientData);
  void deliverFrame();

private:
  static unsigned referenceCount; // used to count how many instances of this class currently exist
  DeviceParameters fParams;

public:
    // DDA wrapper object, defined in DDAImpl.h
    DDAImpl *pDDAWrapper = nullptr;
    // PreProcesor for encoding. Defined in Preproc.h
    // Preprocessing is required if captured images are of different size than encWidthxencHeight
    // This application always uses this preprocessor
    RGBToNV12 *pColorConv = nullptr;
    // NVENCODE API wrapper. Defined in NvEncoderD3D11.h. This class is imported from NVIDIA Video SDK
    NvEncoderD3D11 *pEnc = nullptr;
    // D3D11 device context used for the operations demonstrated in this application
    ID3D11Device *pD3DDev = nullptr;
    // D3D11 device context
    ID3D11DeviceContext *pCtx = nullptr;
    // D3D11 RGB Texture2D object that receives the captured image from DDA
    ID3D11Texture2D *pDupTex2D = nullptr;
    // D3D11 YUV420 Texture2D object that sends the image to NVENC for video encoding
    ID3D11Texture2D *pEncBuf = nullptr;
    // Failure count from Capture API
    UINT failCount = 0;
    // Video output dimensions
    const static UINT encWidth = 1920;
    const static UINT encHeight = 1080;
    // turn off preproc, let NVENCODE API handle colorspace conversion
    const static bool bNoVPBlt = false;
    // Encoded video bitstream packet in CPU memory
    std::vector<std::vector<uint8_t>> vPacket;
    // NVENCODEAPI session intialization parameters
    NV_ENC_INITIALIZE_PARAMS encInitParams = { 0 };
    // NVENCODEAPI video encoding configuration parameters
    NV_ENC_CONFIG encConfig = { 0 };
};

#endif
