#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <getopt.h>

#include "x11capture.h"
#include "x11nvidiacapture.h"

#define APP_VERSION 4

#define LIB_NVFBC_NAME "libnvidia-fbc.so.1"

int CreateNvidiaCaptureDevice(NvidiaCaptureDevice* device) {
    NVFBC_SIZE frameSize = { 0, 0 };
    NVFBC_BOOL printStatusOnly = NVFBC_FALSE;

    NVFBC_TRACKING_TYPE trackingType = NVFBC_TRACKING_DEFAULT;
    char outputName[NVFBC_OUTPUT_NAME_LEN];
    uint32_t outputId = 0;

    void *libNVFBC = NULL;
    PNVFBCCREATEINSTANCE NvFBCCreateInstance_ptr = NULL;

    NVFBCSTATUS fbcStatus;

    NVFBC_CREATE_HANDLE_PARAMS createHandleParams;
    NVFBC_GET_STATUS_PARAMS statusParams;
    NVFBC_CREATE_CAPTURE_SESSION_PARAMS createCaptureParams;

    NVFBC_HWENC_CONFIG encoderConfig;
    NVFBC_TOHWENC_SETUP_PARAMS setupParams;

    NVFBC_HWENC_CODEC codec = NVFBC_HWENC_CODEC_H264;

    NvFBCUtilsPrintVersions(APP_VERSION);

    /*
     * Dynamically load the NvFBC library.
     */
    libNVFBC = dlopen(LIB_NVFBC_NAME, RTLD_NOW);
    if (libNVFBC == NULL) {
        fprintf(stderr, "Unable to open '%s' (%s)\n", LIB_NVFBC_NAME, dlerror());
        return -1;
    }

    /*
     * Resolve the 'NvFBCCreateInstance' symbol that will allow us to get
     * the API function pointers.
     */
    NvFBCCreateInstance_ptr =
        (PNVFBCCREATEINSTANCE) dlsym(libNVFBC, "NvFBCCreateInstance");
    if (NvFBCCreateInstance_ptr == NULL) {
        fprintf(stderr, "Unable to resolve symbol 'NvFBCCreateInstance'\n");
        return -1;
    }

    /*
     * Create an NvFBC instance.
     *
     * API function pointers are accessible through pFn.
     */
    memset(&device->pFn, 0, sizeof(device->pFn));

    device->pFn.dwVersion = NVFBC_VERSION;

    fbcStatus = NvFBCCreateInstance_ptr(&device->pFn);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "Unable to create NvFBC instance (status: %d)\n",
                fbcStatus);
        return -1;
    }

    /*
     * Create a session handle that is used to identify the client.
     */
    memset(&createHandleParams, 0, sizeof(createHandleParams));

    createHandleParams.dwVersion = NVFBC_CREATE_HANDLE_PARAMS_VER;

    fbcStatus = device->pFn.nvFBCCreateHandle(&device->fbcHandle, &createHandleParams);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    /*
     * Get information about the state of the display driver.
     *
     * This call is optional but helps the application decide what it should
     * do.
     */
    memset(&statusParams, 0, sizeof(statusParams));

    statusParams.dwVersion = NVFBC_GET_STATUS_PARAMS_VER;

    fbcStatus = device->pFn.nvFBCGetStatus(device->fbcHandle, &statusParams);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    if (printStatusOnly) {
        NvFBCUtilsPrintStatus(&statusParams);
        return EXIT_SUCCESS;
    }

    if (statusParams.bCanCreateNow == NVFBC_FALSE) {
        fprintf(stderr, "It is not possible to create a capture session "
                        "on this system.\n");
        return -1;
    }

    if (trackingType == NVFBC_TRACKING_OUTPUT) {
        if (!statusParams.bXRandRAvailable) {
            fprintf(stderr, "The XRandR extension is not available.\n");
            fprintf(stderr, "It is therefore not possible to track an RandR output.\n");
            return -1;
        }

        outputId = NvFBCUtilsGetOutputId(statusParams.outputs,
                                         statusParams.dwOutputNum,
                                         outputName);
        if (outputId == 0) {
            fprintf(stderr, "RandR output '%s' not found.\n", outputName);
            return -1;
        }
    }

    /*
     * Create a capture session.
     */
    printf("Creating a capture session of HW compressed frames.\n");

    memset(&createCaptureParams, 0, sizeof(createCaptureParams));

    createCaptureParams.dwVersion       = NVFBC_CREATE_CAPTURE_SESSION_PARAMS_VER;
    createCaptureParams.eCaptureType    = NVFBC_CAPTURE_TO_HW_ENCODER;
    createCaptureParams.bWithCursor     = NVFBC_FALSE;
    createCaptureParams.frameSize       = frameSize;
    createCaptureParams.bRoundFrameSize = NVFBC_TRUE;
    createCaptureParams.eTrackingType   = trackingType;

    if (trackingType == NVFBC_TRACKING_OUTPUT) {
        createCaptureParams.dwOutputId = outputId;
    }

    fbcStatus = device->pFn.nvFBCCreateCaptureSession(device->fbcHandle, &createCaptureParams);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    /*
     * Configure the HW encoder.
     *
     * Select encoding quality, bitrate, etc.
     *
     * Here, we are configuring a 60 fps capture at a balanced encode / decode
     * time, using a high quality profile.
     */
    memset(&encoderConfig, 0, sizeof(encoderConfig));

    encoderConfig.dwVersion          = NVFBC_HWENC_CONFIG_VER;
    if (codec == NVFBC_HWENC_CODEC_H264) {
        encoderConfig.dwProfile      = 77;
    } else {
        encoderConfig.dwProfile      = 1;
    }
    encoderConfig.dwFrameRateNum     = 60;
    encoderConfig.dwFrameRateDen     = 1;
    encoderConfig.dwAvgBitRate       = 8000000;
    encoderConfig.dwPeakBitRate      = encoderConfig.dwAvgBitRate * 1.5;
    encoderConfig.dwGOPLength        = 100;
    encoderConfig.eRateControl       = NVFBC_HWENC_PARAMS_RC_VBR;
    encoderConfig.ePresetConfig      = NVFBC_HWENC_PRESET_LOW_LATENCY_HP;
    encoderConfig.dwQP               = 26;
    encoderConfig.eInputBufferFormat = NVFBC_BUFFER_FORMAT_NV12;
    encoderConfig.dwVBVBufferSize    = encoderConfig.dwAvgBitRate;
    encoderConfig.dwVBVInitialDelay  = encoderConfig.dwVBVBufferSize;
    encoderConfig.codec              = codec;

    /*
     * Frame headers are included with the frame.  Set this to TRUE for
     * outband header fetching.  Headers can then be fetched using the
     * NvFBCToHwEncGetHeader() API call.
     */
    encoderConfig.bOutBandSPSPPS = NVFBC_FALSE;

    /*
     * Set up the capture session.
     */
    memset(&setupParams, 0, sizeof(setupParams));

    setupParams.dwVersion     = NVFBC_TOHWENC_SETUP_PARAMS_VER;
    setupParams.pEncodeConfig = &encoderConfig;

    fbcStatus = device->pFn.nvFBCToHwEncSetUp(device->fbcHandle, &setupParams);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    // Set pointer to NULL, nvidia will overwrite this with the framebuffer pointer
    device->frame = NULL;

    /*
     * We are now ready to start grabbing frames.
     */
    printf("Frame capture session started.  New frames will be captured when "
           "the display is refreshed or when the mouse cursor moves.\n");
    return 0;
}

int NvidiaCaptureScreen(NvidiaCaptureDevice* device) {
    uint64_t t1, t2;

    NVFBCSTATUS fbcStatus;

    NVFBC_TOHWENC_GRAB_FRAME_PARAMS grabParams;

    NVFBC_FRAME_GRAB_INFO frameInfo;
    NVFBC_HWENC_FRAME_INFO encFrameInfo;

    t1 = NvFBCUtilsGetTimeInMillis();

    memset(&grabParams, 0, sizeof(grabParams));
    memset(&frameInfo, 0, sizeof(frameInfo));
    memset(&encFrameInfo, 0, sizeof(encFrameInfo));

    grabParams.dwVersion = NVFBC_TOHWENC_GRAB_FRAME_PARAMS_VER;

    /*
     * Use blocking calls.
     *
     * The application will wait for new frames.  New frames are generated
     * when the mouse cursor moves or when the screen if refreshed.
     */
    grabParams.dwFlags = NVFBC_TOHWENC_GRAB_FLAGS_NOFLAGS;

    /*
     * This structure will contain information about the captured frame.
     */
    grabParams.pFrameGrabInfo = &frameInfo;

    /*
     * This structure will contain information about the encoding of
     * the captured frame.
     */
    grabParams.pEncFrameInfo = &encFrameInfo;

    /*
     * Specify per-frame encoding configuration.  Here, keep the defaults.
     */
    grabParams.pEncodeParams = NULL;

    /*
     * This pointer is allocated by the NvFBC library and will
     * contain the captured frame.
     *
     * Make sure this pointer stays the same during the capture session
     * to prevent memory leaks.
     */
    grabParams.ppBitStreamBuffer = (void **) &device->frame;

    /*
     * Capture a new frame.
     */
    fbcStatus = device->pFn.nvFBCToHwEncGrabFrame(device->fbcHandle, &grabParams);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    /*
     * Save frame to the output file.
     *
     * Information such as dimension and size in bytes is available from
     * the frameInfo structure.
     */
    
    device->size = encFrameInfo.dwByteSize;
    //fwrite(frame, 1, frameInfo.dwByteSize, fd);

    t2 = NvFBCUtilsGetTimeInMillis();

    device->is_iframe = encFrameInfo.bIsIFrame;
    printf("New %dx%d frame id %u size %u%s%s grabbed and saved in %llu ms\n",
           frameInfo.dwWidth, frameInfo.dwHeight, frameInfo.dwCurrentFrame, device->size, encFrameInfo.bIsIFrame ? " (i-frame)" : "", frameInfo.bIsNewFrame ? " (new frame)" : "", (unsigned long long) (t2 - t1));

    return 0;
}

void DestroyNvidiaCaptureDevice(NvidiaCaptureDevice* device) {
    NVFBCSTATUS fbcStatus;
    NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroyCaptureParams;
    NVFBC_DESTROY_HANDLE_PARAMS destroyHandleParams;

    /*
     * Destroy capture session, tear down resources.
     */
    memset(&destroyCaptureParams, 0, sizeof(destroyCaptureParams));

    destroyCaptureParams.dwVersion = NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER;

    fbcStatus = device->pFn.nvFBCDestroyCaptureSession(device->fbcHandle, &destroyCaptureParams);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return;
    }

    /*
     * Destroy session handle, tear down more resources.
     */
    memset(&destroyHandleParams, 0, sizeof(destroyHandleParams));

    destroyHandleParams.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER;

    fbcStatus = device->pFn.nvFBCDestroyHandle(device->fbcHandle, &destroyHandleParams);
    if (fbcStatus != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return;
    }
}

