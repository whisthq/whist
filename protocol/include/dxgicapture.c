/*
 * This file contains the implementation of DXGI screen capture.

 Protocol version: 1.0
 Last modification: 1/15/2020

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2020
*/

#include "dxgicapture.h"

ID3D11Device *createDirect3D11Device(IDXGIAdapter1 * pOutputAdapter) {
  D3D_FEATURE_LEVEL aFeatureLevels[] =
    {
      D3D_FEATURE_LEVEL_11_0
    };

  ID3D11Device * pDevice;
  D3D_FEATURE_LEVEL featureLevel;

  ID3D11DeviceContext * pDeviceContext;
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

IDXGIOutput1 *findAttachedOutput(IDXGIFactory1 *factory) {
  IDXGIAdapter1* pAdapter;

  UINT adapterIndex = 0;
  while (factory->lpVtbl->EnumAdapters1(factory, adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
    IDXGIAdapter2* pAdapter2 = (IDXGIAdapter2*) pAdapter;
        
    UINT outputIndex = 0;
    IDXGIOutput * pOutput;

    DXGI_ADAPTER_DESC aDesc;
    pAdapter2->lpVtbl->GetDesc(pAdapter2, &aDesc);

    HRESULT hEnumOutputs = pAdapter->lpVtbl->EnumOutputs(pAdapter, outputIndex, &pOutput);
    while(hEnumOutputs != DXGI_ERROR_NOT_FOUND) {
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

void CreateDXGIDevice(DXGIDevice *device) {
  //Initialize the factory pointer
  IDXGIFactory2* factory;
  
  //Actually create it
  HRESULT hCreateFactory = CreateDXGIFactory1(&IID_IDXGIFactory2, (void**)(&factory));
  if (hCreateFactory != S_OK) {
    printf("ERROR: 0x%X\n", hCreateFactory);
    return 1;
  }

  IDXGIOutput1 *output = findAttachedOutput(factory);

  void * pOutputParent;
  if (output->lpVtbl->GetParent(output, &IID_IDXGIAdapter1, &pOutputParent) != S_OK) {
    return -1;
  }

  IDXGIAdapter1 * adapter = (IDXGIAdapter1 *)pOutputParent;
  ID3D11Device * pD3Device = createDirect3D11Device(adapter);

  DXGI_OUTPUT_DESC oDesc;
  output->lpVtbl->GetDesc(output, &oDesc);

  DXGI_ADAPTER_DESC aDesc;
  adapter->lpVtbl->GetDesc(adapter, &aDesc);

  printf("Duplicating '%S' attached to '%S'\n", oDesc.DeviceName, aDesc.Description);

  HRESULT hDuplicateOutput = output->lpVtbl->DuplicateOutput(output, pD3Device, &device->duplication);
  if (hDuplicateOutput != S_OK) {
    printf("Error duplicating output: 0x%X\n", hDuplicateOutput);
    return 1;
  }

  DXGI_OUTDUPL_DESC odDesc;
  device->duplication->lpVtbl->GetDesc(device->duplication, &odDesc);

  printf("Able to duplicate a %ix%i desktop\n", odDesc.ModeDesc.Width, odDesc.ModeDesc.Height);
}

HRESULT CaptureScreen(DXGIDevice *device) {
  HRESULT hr;
  hr = device->duplication->lpVtbl->AcquireNextFrame(device->duplication, 17, &device->frame_info, &device->resource);
  if(FAILED(hr)) {
    return hr;
  }
  hr = device->duplication->lpVtbl->MapDesktopSurface(device->duplication, &device->frame_data); 
  if (FAILED(hr)) {
      printf("Not in system memory\n");
      ReleaseScreen(device);
      return hr;
  }
  printf("Screen captured\n");
  return hr; 
}

HRESULT ReleaseScreen(DXGIDevice *device) {
  HRESULT hr;
  hr = device->duplication->lpVtbl->UnMapDesktopSurface(device->duplication);
  if(FAILED(hr)) {
    return hr;
  }
  hr = device->duplication->lpVtbl->ReleaseFrame(device->duplication);
  return hr;
}