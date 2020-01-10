#ifndef WASAPICAPTURE_H  // include guard
#define WASAPICAPTURE_H

#include "fractal.h" // contains all the headers


DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(IID_IAudioCaptureClient, 0xc8adbd64, 0xe71e, 0x48a0, 0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17);

typedef struct wasapi_device{
    IMMDevice *device;
    IMMDeviceEnumerator *pMMDeviceEnumerator;
    IAudioClient *pAudioClient;
    REFERENCE_TIME hnsDefaultDevicePeriod;
    WAVEFORMATEX *pwfx;
    IAudioCaptureClient *pAudioCaptureClient;
    HANDLE hWakeUp;
    BYTE *pData;
    LONG audioBufSize;
    UINT32 nNumFramesToRead;
    DWORD dwWaitResult;
} wasapi_device;


wasapi_device *CreateAudioDevice(wasapi_device *audio_device);

void StartAudioDevice(wasapi_device *audio_device);

void DestroyAudioDevice(wasapi_device *audio_device);

#endif