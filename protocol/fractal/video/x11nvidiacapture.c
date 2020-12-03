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

int create_nvidia_capture_device(NvidiaCaptureDevice* device, int bitrate,
                                 CodecType requested_codec) {
    NVFBC_SIZE frame_size = {0, 0};
    NVFBC_BOOL print_status_only = NVFBC_FALSE;

    NVFBC_TRACKING_TYPE tracking_type = NVFBC_TRACKING_DEFAULT;
    char output_name[NVFBC_OUTPUT_NAME_LEN];
    uint32_t output_id = 0;

    void* lib_nvfbc = NULL;
    PNVFBCCREATEINSTANCE nv_fbc_create_instance_ptr = NULL;

    NVFBCSTATUS fbc_status;

    NVFBC_CREATE_HANDLE_PARAMS create_handle_params;
    NVFBC_GET_STATUS_PARAMS status_params;
    NVFBC_CREATE_CAPTURE_SESSION_PARAMS create_capture_params;

    NVFBC_HWENC_CONFIG encoder_config;
    NVFBC_TOHWENC_SETUP_PARAMS setup_params;

    NVFBC_HWENC_CODEC codec;
    if (requested_codec == CODEC_TYPE_H265) {
        device->codec_type = CODEC_TYPE_H265;
        codec = NVFBC_HWENC_CODEC_HEVC;
    } else {
        device->codec_type = CODEC_TYPE_H264;
        codec = NVFBC_HWENC_CODEC_H264;
    }

    NvFBCUtilsPrintVersions(APP_VERSION);

    /*
     * Dynamically load the NvFBC library.
     */
    lib_nvfbc = dlopen(LIB_NVFBC_NAME, RTLD_NOW);
    if (lib_nvfbc == NULL) {
        fprintf(stderr, "Unable to open '%s' (%s)\n", LIB_NVFBC_NAME, dlerror());
        return -1;
    }

    /*
     * Resolve the 'NvFBCCreateInstance' symbol that will allow us to get
     * the API function pointers.
     */
    nv_fbc_create_instance_ptr = (PNVFBCCREATEINSTANCE)dlsym(lib_nvfbc, "NvFBCCreateInstance");
    if (nv_fbc_create_instance_ptr == NULL) {
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

    fbc_status = nv_fbc_create_instance_ptr(&device->pFn);
    if (fbc_status != NVFBC_SUCCESS) {
        fprintf(stderr, "Unable to create NvFBC instance (status: %d)\n", fbc_status);
        return -1;
    }

    /*
     * Create a session handle that is used to identify the client.
     */
    memset(&create_handle_params, 0, sizeof(create_handle_params));

    create_handle_params.dwVersion = NVFBC_CREATE_HANDLE_PARAMS_VER;

    fbc_status = device->pFn.nvFBCCreateHandle(&device->fbcHandle, &create_handle_params);
    if (fbc_status != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    /*
     * Get information about the state of the display driver.
     *
     * This call is optional but helps the application decide what it should
     * do.
     */
    memset(&status_params, 0, sizeof(status_params));

    status_params.dwVersion = NVFBC_GET_STATUS_PARAMS_VER;

    fbc_status = device->pFn.nvFBCGetStatus(device->fbcHandle, &status_params);
    if (fbc_status != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    if (print_status_only) {
        NvFBCUtilsPrintStatus(&status_params);
        return EXIT_SUCCESS;
    }

    if (status_params.bCanCreateNow == NVFBC_FALSE) {
        fprintf(stderr,
                "It is not possible to create a capture session "
                "on this system.\n");
        return -1;
    }

    if (tracking_type == NVFBC_TRACKING_OUTPUT) {
        if (!status_params.bXRandRAvailable) {
            fprintf(stderr, "The XRandR extension is not available.\n");
            fprintf(stderr, "It is therefore not possible to track an RandR output.\n");
            return -1;
        }

        output_id =
            NvFBCUtilsGetOutputId(status_params.outputs, status_params.dwOutputNum, output_name);
        if (output_id == 0) {
            fprintf(stderr, "RandR output '%s' not found.\n", output_name);
            return -1;
        }
    }

    /*
     * Create a capture session.
     */
    printf("Creating a capture session of HW compressed frames.\n");

    memset(&create_capture_params, 0, sizeof(create_capture_params));

    create_capture_params.dwVersion = NVFBC_CREATE_CAPTURE_SESSION_PARAMS_VER;
    create_capture_params.eCaptureType = NVFBC_CAPTURE_TO_HW_ENCODER;
    create_capture_params.bWithCursor = NVFBC_FALSE;
    create_capture_params.frameSize = frame_size;
    create_capture_params.bRoundFrameSize = NVFBC_TRUE;
    create_capture_params.eTrackingType = tracking_type;

    if (tracking_type == NVFBC_TRACKING_OUTPUT) {
        create_capture_params.dwOutputId = output_id;
    }

    fbc_status = device->pFn.nvFBCCreateCaptureSession(device->fbcHandle, &create_capture_params);
    if (fbc_status != NVFBC_SUCCESS) {
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
    memset(&encoder_config, 0, sizeof(encoder_config));

    encoder_config.dwVersion = NVFBC_HWENC_CONFIG_VER;
    if (codec == NVFBC_HWENC_CODEC_H264) {
        encoder_config.dwProfile = 77;
    } else {
        encoder_config.dwProfile = 1;
    }
    encoder_config.dwFrameRateNum = FPS;
    encoder_config.dwFrameRateDen = 1;
    encoder_config.dwAvgBitRate = bitrate;
    encoder_config.dwPeakBitRate = encoder_config.dwAvgBitRate * 1.5;
    encoder_config.dwGOPLength = 99999;
    encoder_config.eRateControl = NVFBC_HWENC_PARAMS_RC_CBR;
    encoder_config.ePresetConfig = NVFBC_HWENC_PRESET_LOW_LATENCY_HP;
    encoder_config.dwQP = 18;
    encoder_config.eInputBufferFormat = NVFBC_BUFFER_FORMAT_NV12;
    encoder_config.dwVBVBufferSize = encoder_config.dwAvgBitRate;
    encoder_config.dwVBVInitialDelay = encoder_config.dwVBVBufferSize;
    encoder_config.codec = codec;

    /*
     * Frame headers are included with the frame.  Set this to TRUE for
     * outband header fetching.  Headers can then be fetched using the
     * NvFBCToHwEncGetHeader() API call.
     */
    encoder_config.bOutBandSPSPPS = NVFBC_FALSE;

    /*
     * Set up the capture session.
     */
    memset(&setup_params, 0, sizeof(setup_params));

    setup_params.dwVersion = NVFBC_TOHWENC_SETUP_PARAMS_VER;
    setup_params.pEncodeConfig = &encoder_config;

    fbc_status = device->pFn.nvFBCToHwEncSetUp(device->fbcHandle, &setup_params);
    if (fbc_status != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    // Set pointer to NULL, nvidia will overwrite this with the framebuffer pointer
    device->frame = NULL;

    /*
     * We are now ready to start grabbing frames.
     */
    printf(
        "Frame capture session started.  New frames will be captured when "
        "the display is refreshed or when the mouse cursor moves.\n");
    return 0;
}

#define SHOW_DEBUG_FRAMES false

int nvidia_capture_screen(NvidiaCaptureDevice* device) {
#if SHOW_DEBUG_FRAMES
    uint64_t t1, t2;
#endif

    NVFBCSTATUS fbc_status;

    NVFBC_TOHWENC_GRAB_FRAME_PARAMS grab_params;

    NVFBC_FRAME_GRAB_INFO frame_info;
    NVFBC_HWENC_FRAME_INFO enc_frame_info;

#if SHOW_DEBUG_FRAMES
    t1 = NvFBCUtilsGetTimeInMillis();
#endif

    memset(&grab_params, 0, sizeof(grab_params));
    memset(&frame_info, 0, sizeof(frame_info));
    memset(&enc_frame_info, 0, sizeof(enc_frame_info));

    grab_params.dwVersion = NVFBC_TOHWENC_GRAB_FRAME_PARAMS_VER;

    /*
     * Use blocking calls.
     *
     * The application will wait for new frames.  New frames are generated
     * when the mouse cursor moves or when the screen if refreshed.
     */
    grab_params.dwFlags = NVFBC_TOHWENC_GRAB_FLAGS_NOWAIT;

    /*
     * This structure will contain information about the captured frame.
     */
    grab_params.pFrameGrabInfo = &frame_info;

    /*
     * This structure will contain information about the encoding of
     * the captured frame.
     */
    grab_params.pEncFrameInfo = &enc_frame_info;

    /*
     * Specify per-frame encoding configuration.  Here, keep the defaults.
     */
    grab_params.pEncodeParams = NULL;

    /*
     * This pointer is allocated by the NvFBC library and will
     * contain the captured frame.
     *
     * Make sure this pointer stays the same during the capture session
     * to prevent memory leaks.
     */
    grab_params.ppBitStreamBuffer = (void**)&device->frame;

    /*
     * Capture a new frame.
     */
    fbc_status = device->pFn.nvFBCToHwEncGrabFrame(device->fbcHandle, &grab_params);
    if (fbc_status != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return -1;
    }

    /*
     * Save frame to the output file.
     *
     * Information such as dimension and size in bytes is available from
     * the frameInfo structure.
     */

    device->size = enc_frame_info.dwByteSize;
    // fwrite(frame, 1, frameInfo.dwByteSize, fd);

    device->is_iframe = enc_frame_info.bIsIFrame;

#if SHOW_DEBUG_FRAMES
    t2 = NvFBCUtilsGetTimeInMillis();

    printf("New %dx%d frame id %u size %u%s%s grabbed and saved in %llu ms\n", frameInfo.dwWidth,
           frameInfo.dwHeight, frameInfo.dwCurrentFrame, device->size,
           encFrameInfo.bIsIFrame ? " (i-frame)" : "", frameInfo.bIsNewFrame ? " (new frame)" : "",
           (unsigned long long)(t2 - t1));
#endif

    device->width = frame_info.dwWidth;
    device->height = frame_info.dwHeight;

    return 1;
}

void destroy_nvidia_capture_device(NvidiaCaptureDevice* device) {
    NVFBCSTATUS fbc_status;
    NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroy_capture_params;
    NVFBC_DESTROY_HANDLE_PARAMS destroy_handle_params;

    /*
     * Destroy capture session, tear down resources.
     */
    memset(&destroy_capture_params, 0, sizeof(destroy_capture_params));

    destroy_capture_params.dwVersion = NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER;

    fbc_status = device->pFn.nvFBCDestroyCaptureSession(device->fbcHandle, &destroy_capture_params);
    if (fbc_status != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return;
    }

    /*
     * Destroy session handle, tear down more resources.
     */
    memset(&destroy_handle_params, 0, sizeof(destroy_handle_params));

    destroy_handle_params.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER;

    fbc_status = device->pFn.nvFBCDestroyHandle(device->fbcHandle, &destroy_handle_params);
    if (fbc_status != NVFBC_SUCCESS) {
        fprintf(stderr, "%s\n", device->pFn.nvFBCGetLastErrorStr(device->fbcHandle));
        return;
    }
}
