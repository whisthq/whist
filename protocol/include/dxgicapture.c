/*
 * This file contains the implementation of DXGI screen capture.

 Protocol version: 1.0
 Last modification: 1/15/2020

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2020
*/

#include "dxgicapture.h"

ID3D11Device* createDirect3D11Device(IDXGIAdapter1* pOutputAdapter) {
    D3D_FEATURE_LEVEL aFeatureLevels[] =
    {
      D3D_FEATURE_LEVEL_11_0
    };

    ID3D11Device* pDevice;
    D3D_FEATURE_LEVEL featureLevel;

    ID3D11DeviceContext* pDeviceContext;
    HRESULT hCreateDevice;
    hCreateDevice = D3D11CreateDevice(pOutputAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        NULL,
        0,
        aFeatureLevels,
        ARRAYSIZE(aFeatureLevels),
        D3D11_SDK_VERSION,
        &pDevice,
        &featureLevel,
        &pDeviceContext);
    if (hCreateDevice == S_OK) {
        return pDevice;
    }
    return NULL;
}

IDXGIOutput1* findAttachedOutput(IDXGIFactory1* factory) {
    IDXGIAdapter1* pAdapter;

    UINT adapterIndex = 0;
    while (factory->lpVtbl->EnumAdapters1(factory, adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        IDXGIAdapter2* pAdapter2 = (IDXGIAdapter2*)pAdapter;

        UINT outputIndex = 0;
        IDXGIOutput* pOutput;

        DXGI_ADAPTER_DESC aDesc;
        pAdapter2->lpVtbl->GetDesc(pAdapter2, &aDesc);

        HRESULT hEnumOutputs = pAdapter->lpVtbl->EnumOutputs(pAdapter, outputIndex, &pOutput);
        while (hEnumOutputs != DXGI_ERROR_NOT_FOUND) {
            if (hEnumOutputs != S_OK) {
                return NULL;
            }

            DXGI_OUTPUT_DESC oDesc;
            pOutput->lpVtbl->GetDesc(pOutput, &oDesc);

            if (oDesc.AttachedToDesktop) {
                return (IDXGIOutput1*)pOutput;
            }

            ++outputIndex;
            hEnumOutputs = pAdapter->lpVtbl->EnumOutputs(pAdapter, outputIndex, &pOutput);
        }

        ++adapterIndex;
    }
    return -1;
}

int CreateDXGIDevice(DXGIDevice* device) {
    //Initialize the factory pointer
    FILE *fp;
    IDXGIFactory1* factory;

    //Actually create it
    HRESULT hCreateFactory = CreateDXGIFactory1(&IID_IDXGIFactory2, (void**)(&factory));
    if (hCreateFactory != S_OK) {
        printf("Error creating Factory: 0x%X\n", hCreateFactory);
        return -1;
    }

    IDXGIOutput1* output = findAttachedOutput(factory);

    void* pOutputParent;
    HRESULT hGetParent = output->lpVtbl->GetParent(output, &IID_IDXGIAdapter1, &pOutputParent);
    if (hGetParent != S_OK) {
        printf("Error getting patent: 0x%X\n", hGetParent);
        return -1;
    }

    IDXGIAdapter1* adapter = (IDXGIAdapter1*)pOutputParent;
    ID3D11Device* pD3Device = createDirect3D11Device(adapter);

    DXGI_OUTPUT_DESC oDesc;

    HRESULT hOutputDesc = output->lpVtbl->GetDesc(output, &oDesc);
    if (hOutputDesc != S_OK) {
        printf("Error getting output description: 0x%X\n", hOutputDesc);
        return -1;
    }

    DXGI_ADAPTER_DESC aDesc;
    HRESULT hAdapterDesc = adapter->lpVtbl->GetDesc(adapter, &aDesc);
    if (hAdapterDesc != S_OK) {
        printf("Error getting adapter description: 0x%X\n", hAdapterDesc);
        return -1;
    }

    printf("Duplicating '%S' attached to '%S'\n", oDesc.DeviceName, aDesc.Description);
    fp = fopen("/log1.txt", "a+");
    fprintf(fp, "Duplicating '%S' attached to '%S'\n", oDesc.DeviceName, aDesc.Description);
    fclose(fp);

    HRESULT hDuplicateOutput = output->lpVtbl->DuplicateOutput(output, pD3Device, &device->duplication);
    if (hDuplicateOutput != S_OK) {
        printf("Error duplicating output: 0x%X\n", hDuplicateOutput);
        return -1;
    }

    DXGI_OUTDUPL_DESC odDesc;
    device->duplication->lpVtbl->GetDesc(device->duplication, &odDesc);

    printf("Able to duplicate a %ix%i desktop\n", odDesc.ModeDesc.Width, odDesc.ModeDesc.Height);
    fp = fopen("/log1.txt", "a+");
    fprintf(fp, "Able to duplicate a %ix%i desktop\n", odDesc.ModeDesc.Width, odDesc.ModeDesc.Height);
    fclose(fp);
    
    // get width and height
    HMONITOR hMonitor = oDesc.Monitor;
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &monitorInfo);
    DEVMODE devMode;
    devMode.dmSize = sizeof(DEVMODE);
    devMode.dmDriverExtra = 0;
    EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode);

    device->width = devMode.dmPelsWidth;
    device->height = devMode.dmPelsHeight;

    printf("Dimensions are %ix%i\n", device->width, device->height);
    fp = fopen("/log1.txt", "a+");
    fprintf(fp, "Dimensions are %ix%i\n", device->width, device->height);
    fclose(fp);
    
    return 0;
}

int DestroyDXGIDevice(DXGIDevice* device) {
    if (device->duplication) {
        device->duplication->lpVtbl->Release(device->duplication);
        device->duplication = NULL;
    }
    return 0;
}

HRESULT CaptureScreen(DXGIDevice* device) {
    HRESULT hr;
    hr = device->duplication->lpVtbl->AcquireNextFrame(device->duplication, 0, &device->frame_info, &device->resource);
    if (FAILED(hr)) {
        return hr;
    }
    int accumulated_frames = device->frame_info.AccumulatedFrames;
    if (accumulated_frames > 1) {
        mprintf("Accumulated Frames %d\n", accumulated_frames);
    }
    hr = device->duplication->lpVtbl->MapDesktopSurface(device->duplication, &device->frame_data);
    if (FAILED(hr)) {
        ReleaseScreen(device);
        return hr;
    }
    return hr;
}

HRESULT ReleaseScreen(DXGIDevice* device) {
    HRESULT hr;
    hr = device->duplication->lpVtbl->UnMapDesktopSurface(device->duplication);
    if (FAILED(hr)) {
        return hr;
    }
    hr = device->duplication->lpVtbl->ReleaseFrame(device->duplication);
    return hr;
}