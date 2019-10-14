#ifndef _STREAM_SOURCE_HH
#define _STREAM_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH

#include "FramedSource.hh"
#endif

#pragma comment(lib,"d3d11.lib") // include DirectX 11 lib

#include "Defs.h"
#include "DDAImpl.h"
#include "Preproc.h"
#include <NvEncoderD3D11.h>

class StreamSource: public FramedSource {
public:
  static StreamSource* createNew(UsageEnvironment& env);
    explicit StreamSource(UsageEnvironment& env);
    ~StreamSource() override;
private:
  void doGetNextFrame() override;
public:
  class DemoApplication2
{
      //printf(__FUNCTION__(": Line %d, File %s Returning error 0x%08x\n"), __LINE__, __FILE__, x);
    /// Demo Application Core class
#define returnIfError(x) \
    if (FAILED(x))\
    {\
        printf("FAIL");\
        return x;\
    }

private:
    /// DDA wrapper object, defined in DDAImpl.h
    DDAImpl *pDDAWrapper = nullptr;
    /// PreProcesor for encoding. Defined in Preproc.h
    /// Preprocessingis required if captured images are of different size than encWidthxencHeight
    /// This application always uses this preprocessor
    RGBToNV12 *pColorConv = nullptr;
    /// NVENCODE API wrapper. Defined in NvEncoderD3D11.h. This class is imported from NVIDIA Video SDK
    NvEncoderD3D11 *pEnc = nullptr;
    /// D3D11 device context used for the operations demonstrated in this application
    ID3D11Device *pD3DDev = nullptr;
    /// D3D11 device context
    ID3D11DeviceContext *pCtx = nullptr;
    /// D3D11 RGB Texture2D object that recieves the captured image from DDA
    ID3D11Texture2D *pDupTex2D = nullptr;
    /// D3D11 YUV420 Texture2D object that sends the image to NVENC for video encoding
    ID3D11Texture2D *pEncBuf = nullptr;
    /// Output video bitstream file handle
    FILE *fp = nullptr;
    /// Failure count from Capture API
    UINT failCount = 0;
    /// Video output dimensions
    const static UINT encWidth = 1920;
    const static UINT encHeight = 1080;
    /// turn off preproc, let NVENCODE API handle colorspace conversion
    const static bool bNoVPBlt = false;
    /// Video output file name
    const char fnameBase[64] = "DDATest_%d.h264";
public:
    /// Encoded video bitstream packet in CPU memory
    std::vector<std::vector<uint8_t>> vPacket;
private:
    /// NVENCODEAPI session intialization parameters
    NV_ENC_INITIALIZE_PARAMS encInitParams = { 0 };
    /// NVENCODEAPI video encoding configuration parameters
    NV_ENC_CONFIG encConfig = { 0 };
private:
    /// Initialize DXGI pipeline
    HRESULT InitDXGI()
    {
        HRESULT hr = S_OK;
        /// Driver types supported
        D3D_DRIVER_TYPE DriverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

        /// Feature levels supported
        D3D_FEATURE_LEVEL FeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_1
        };
        UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
        D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;

        /// Create device
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
        return hr;
    }

    /// Initialize DDA handler
    HRESULT InitDup()
    {
        HRESULT hr = S_OK;
        if (!pDDAWrapper)
        {
            std::cout << "INIT THE USELESS" << std::endl;
            pDDAWrapper = new DDAImpl(pD3DDev, pCtx);
            hr = pDDAWrapper->Init();
            returnIfError(hr);
        }
        return hr;
    }

    /// Initialize NVENCODEAPI wrapper
    HRESULT InitEnc()
    {
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
        return S_OK;
    }

    /// Initialize preprocessor
    HRESULT InitColorConv()
    {
        if (!pColorConv)
        {
            pColorConv = new RGBToNV12(pD3DDev, pCtx);
            HRESULT hr = pColorConv->Init();
            returnIfError(hr);
        }
        return S_OK;
    }

    /// Initialize Video output file
    HRESULT InitOutFile()
    {
        if (!fp)
        {
            char fname[64] = { 0 };
            sprintf_s(fname, (const char *)fnameBase, failCount);
            errno_t err = fopen_s(&fp, fname, "wb");
            returnIfError(err);
        }
        return S_OK;
    }

    /// Write encoded video output to file
    u_int8_t* WriteEncOutput()
    {
        for (std::vector<uint8_t> &packet : vPacket) {
            //std::cout << "Write" << std::endl;
            fwrite(packet.data(), packet.size(), 1, fp);
        }
        //std::cout << "SIZE_PACKET : " << vPacket[0].size() << std::endl;
		return vPacket[0].data();
    }

public:
    /// Initialize demo application
    HRESULT Init()
    {
        HRESULT hr = S_OK;

        hr = InitDXGI();
        returnIfError(hr);

        hr = InitDup();
        returnIfError(hr);


        hr = InitEnc();
        returnIfError(hr);

        hr = InitColorConv();
        returnIfError(hr);

        hr = InitOutFile();
        returnIfError(hr);
        return hr;
    }

    /// Capture a frame using DDA
    HRESULT Capture(int wait)
    {
        HRESULT hr = pDDAWrapper->GetCapturedFrame(&pDupTex2D, wait); // Release after preproc
        if (FAILED(hr))
        {
            failCount++;
        }
        return hr;
    }

    /// Preprocess captured frame
    HRESULT Preproc()
    {
        HRESULT hr = S_OK;
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
        returnIfError(hr);

        pEncBuf->AddRef();  // Release after encode
        return hr;
    }

    /// Encode the captured frame using NVENCODEAPI
    u_int8_t* Encode()
    {
        HRESULT hr = S_OK;
        try
        {
            pEnc->EncodeFrame(vPacket);
            u_int8_t* result = WriteEncOutput();
            return result;
        }
        catch (...)
        {
            return nullptr;
        }
        return nullptr;
    }

    /// Release all resources
    void Cleanup(bool bDelete = true)
    {
        if (pDDAWrapper)
        {
            pDDAWrapper->Cleanup();
            delete pDDAWrapper;
            pDDAWrapper = nullptr;
        }

        if (pColorConv)
        {
            pColorConv->Cleanup();
        }

        if (pEnc)
        {
            ZeroMemory(&encInitParams, sizeof(NV_ENC_INITIALIZE_PARAMS));
            ZeroMemory(&encConfig, sizeof(NV_ENC_CONFIG));
        }

        SAFE_RELEASE(pDupTex2D);
        if (bDelete)
        {
            if (pEnc)
            {
                /// Flush the encoder and write all output to file before destroying the encoder
                pEnc->EndEncode(vPacket);
                //FIXME
                //WriteEncOutput();
                pEnc->DestroyEncoder();
                if (bDelete)
                {
                    delete pEnc;
                    pEnc = nullptr;
                }

                ZeroMemory(&encInitParams, sizeof(NV_ENC_INITIALIZE_PARAMS));
                ZeroMemory(&encConfig, sizeof(NV_ENC_CONFIG));
            }

            if (pColorConv)
            {
                delete pColorConv;
                pColorConv = nullptr;
            }
            SAFE_RELEASE(pD3DDev);
            SAFE_RELEASE(pCtx);
        }
    }
    DemoApplication2() {}
    ~DemoApplication2()
    {
        Cleanup(true); 
    }
};
};

#endif
