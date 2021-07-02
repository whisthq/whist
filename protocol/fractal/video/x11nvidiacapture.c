#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <getopt.h>

#include "x11capture.h"
#include "x11nvidiacapture.h"
#include <GL/glx.h>

// NOTE: Using Nvidia Capture SDK 8.0.4
// Please bump this comment if a newer Nvidia Capture SDK is going to be used

#define APP_VERSION 4

#define PRINT_STATUS false

#define LIB_NVFBC_NAME "libnvidia-fbc.so.1"
#define LIB_ENCODEAPI_NAME "libnvidia-encode.so.1"

/**
 * @brief                          Creates an OpenGL context for use in NvFBC.
 *                                 NvFBC needs an OpenGL context to work with,
 *                                 since it will capture frames into an OpenGL Texture.
 *
 * @param glx_ctx                  Pointer to the glx context to fill in
 *
 * @param glx_fb_config            Pointer to the framebuffer config to fill in
 *
 * @returns                        NVFBC_TRUE in case of success, NVFBC_FALSE otherwise.
 */
static NVFBC_BOOL gl_init(GLXContext* glx_ctx, GLXFBConfig* glx_fb_config) {
    int attribs[] = {GLX_DRAWABLE_TYPE,
                     GLX_PIXMAP_BIT | GLX_WINDOW_BIT,
                     GLX_BIND_TO_TEXTURE_RGBA_EXT,
                     1,
                     GLX_BIND_TO_TEXTURE_TARGETS_EXT,
                     GLX_TEXTURE_2D_BIT_EXT,
                     None};

    // I don't really know what's going on in this function,
    // ideally Nvidia should've put this in their NvFBCUtils.c file,
    // but they didn't so I just copied this function from their examples.
    // All that matters is that this opens the X11 Display,
    // and makes an OpenGL Context out of it

    // Get the X11 Display that the OpenGL Context will refer to
    Display* dpy = XOpenDisplay(NULL);
    if (dpy == None) {
        fprintf(stderr, "Unable to open display\n");
        return NVFBC_FALSE;
    }

    int n;
    GLXFBConfig* fb_configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), attribs, &n);
    if (!fb_configs) {
        fprintf(stderr, "Unable to find FB configs\n");
        return NVFBC_FALSE;
    }

    *glx_ctx = glXCreateNewContext(dpy, fb_configs[0], GLX_RGBA_TYPE, None, True);
    if (*glx_ctx == None) {
        fprintf(stderr, "Unable to create GL context\n");
        return NVFBC_FALSE;
    }

    Pixmap pixmap =
        XCreatePixmap(dpy, XDefaultRootWindow(dpy), 1, 1, DisplayPlanes(dpy, XDefaultScreen(dpy)));
    if (pixmap == None) {
        fprintf(stderr, "Unable to create pixmap\n");
        return NVFBC_FALSE;
    }

    GLXPixmap glx_pixmap = glXCreatePixmap(dpy, fb_configs[0], pixmap, NULL);
    if (glx_pixmap == None) {
        fprintf(stderr, "Unable to create GLX pixmap\n");
        return NVFBC_FALSE;
    }

    Bool res = glXMakeCurrent(dpy, glx_pixmap, *glx_ctx);
    if (!res) {
        fprintf(stderr, "Unable to make context current\n");
        return NVFBC_FALSE;
    }

    *glx_fb_config = fb_configs[0];

    XFree(fb_configs);

    return NVFBC_TRUE;
}

int create_nvidia_encoder(NvidiaCaptureDevice* device, int bitrate, CodecType requested_codec,
                          NVFBC_TOGL_SETUP_PARAMS* p_setup_params) {
    NVENCSTATUS status;

    /*
     * Dynamically load the NvEncodeAPI library.
     */
    void* lib_enc = dlopen(LIB_ENCODEAPI_NAME, RTLD_NOW);
    if (lib_enc == NULL) {
        LOG_ERROR("Unable to open '%s' (%s)", LIB_ENCODEAPI_NAME, dlerror());
        return -1;
    }

    /*
     * Resolve the 'NvEncodeAPICreateInstance' symbol that will allow us to get
     * the API function pointers.
     */
    typedef NVENCSTATUS(NVENCAPI * NVENCODEAPICREATEINSTANCEPROC)(NV_ENCODE_API_FUNCTION_LIST*);
    NVENCODEAPICREATEINSTANCEPROC nv_encode_api_create_instance_ptr =
        (NVENCODEAPICREATEINSTANCEPROC)dlsym(lib_enc, "NvEncodeAPICreateInstance");
    if (nv_encode_api_create_instance_ptr == NULL) {
        LOG_ERROR("Unable to resolve symbol 'NvEncodeAPICreateInstance'");
        return -1;
    }

    /*
     * Create an NvEncodeAPI instance.
     *
     * API function pointers are accessible through pEncFn.
     */
    memset(&device->p_enc_fn, 0, sizeof(device->p_enc_fn));

    device->p_enc_fn.version = NV_ENCODE_API_FUNCTION_LIST_VER;

    status = nv_encode_api_create_instance_ptr(&device->p_enc_fn);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Unable to create NvEncodeAPI instance (status: %d)", status);
        return -1;
    }

    /*
     * Create an encoder session
     */
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encode_session_params = {0};

    encode_session_params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    encode_session_params.apiVersion = NVENCAPI_VERSION;
    encode_session_params.deviceType = NV_ENC_DEVICE_TYPE_OPENGL;

    status = device->p_enc_fn.nvEncOpenEncodeSessionEx(&encode_session_params, &device->encoder);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to open an encoder session, status = %d", status);
        return -1;
    }

    /*
     * Validate the codec requested
     */
    GUID codec_guid;
    if (requested_codec == CODEC_TYPE_H265) {
        device->codec_type = CODEC_TYPE_H265;
        codec_guid = NV_ENC_CODEC_HEVC_GUID;
    } else {
        device->codec_type = CODEC_TYPE_H264;
        codec_guid = NV_ENC_CODEC_H264_GUID;
    }
    // status = validateEncodeGUID(encoder, codec_guid);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to validate codec GUID");
        return -1;
    }

    NV_ENC_PRESET_CONFIG preset_config;
    memset(&preset_config, 0, sizeof(preset_config));

    preset_config.version = NV_ENC_PRESET_CONFIG_VER;
    preset_config.presetCfg.version = NV_ENC_CONFIG_VER;
    status = device->p_enc_fn.nvEncGetEncodePresetConfig(
        device->encoder, codec_guid, NV_ENC_PRESET_LOW_LATENCY_HP_GUID, &preset_config);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR(
            "Failed to obtain preset settings, "
            "status = %d",
            status);
        return -1;
    }

    // Set iframe length to basically be infinite
    preset_config.presetCfg.gopLength = 999999;
    // Set bitrate
    preset_config.presetCfg.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
    preset_config.presetCfg.rcParams.averageBitRate = bitrate;
    // Specify the size of the clientside buffer, 1 * bitrate is recommended
    preset_config.presetCfg.rcParams.vbvBufferSize = bitrate;

    /*
     * Initialize the encode session
     */
    NV_ENC_INITIALIZE_PARAMS init_params;
    memset(&init_params, 0, sizeof(init_params));
    init_params.version = NV_ENC_INITIALIZE_PARAMS_VER;
    init_params.encodeGUID = codec_guid;
    init_params.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
    init_params.encodeConfig = &preset_config.presetCfg;
    init_params.encodeWidth = device->width;
    init_params.encodeHeight = device->height;
    init_params.frameRateNum = FPS;
    init_params.frameRateDen = 1;
    init_params.enablePTD = 1;

    status = device->p_enc_fn.nvEncInitializeEncoder(device->encoder, &init_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to initialize the encode session, status = %d", status);
        return -1;
    }

    /*
     * Register the textures received from NvFBC for use with NvEncodeAPI
     */
    for (int i = 0; i < NVFBC_TOGL_TEXTURES_MAX; i++) {
        NV_ENC_REGISTER_RESOURCE register_params;
        NV_ENC_INPUT_RESOURCE_OPENGL_TEX tex_params;

        if (!p_setup_params->dwTextures[i]) {
            break;
        }

        memset(&register_params, 0, sizeof(register_params));

        tex_params.texture = p_setup_params->dwTextures[i];
        tex_params.target = p_setup_params->dwTexTarget;

        register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
        register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_OPENGL_TEX;
        register_params.width = device->width;
        register_params.height = device->height;
        register_params.pitch = device->width;
        register_params.resourceToRegister = &tex_params;
        register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

        status = device->p_enc_fn.nvEncRegisterResource(device->encoder, &register_params);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to register texture, status = %d", status);
            return -1;
        }

        device->registered_resources[i] = register_params.registeredResource;
    }

    /*
     * Create a bitstream buffer to hold the output
     */
    NV_ENC_CREATE_BITSTREAM_BUFFER bitstream_buffer_params = {0};
    bitstream_buffer_params.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;

    status = device->p_enc_fn.nvEncCreateBitstreamBuffer(device->encoder, &bitstream_buffer_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to create a bitstream buffer, status = %d", status);
        return -1;
    }

    device->output_buffer = bitstream_buffer_params.bitstreamBuffer;
    return 0;
}

int create_nvidia_capture_device(NvidiaCaptureDevice* device, int bitrate,
                                 CodecType requested_codec) {
    // 0-initialize everything in the NvidiaCaptureDevice
    memset(device, 0, sizeof(NvidiaCaptureDevice));

    char output_name[NVFBC_OUTPUT_NAME_LEN];
    uint32_t output_id = 0;

    NVFBCSTATUS fbc_status;

    NvFBCUtilsPrintVersions(APP_VERSION);

    /*
     * Dynamically load the NvFBC library.
     */
    void* lib_nvfbc = dlopen(LIB_NVFBC_NAME, RTLD_NOW);
    if (lib_nvfbc == NULL) {
        LOG_ERROR("Unable to open '%s' (%s)", LIB_NVFBC_NAME, dlerror());
        return -1;
    }

    /*
     * Resolve the 'NvFBCCreateInstance' symbol that will allow us to get
     * the API function pointers.
     */
    PNVFBCCREATEINSTANCE nv_fbc_create_instance_ptr =
        (PNVFBCCREATEINSTANCE)dlsym(lib_nvfbc, "NvFBCCreateInstance");
    if (nv_fbc_create_instance_ptr == NULL) {
        LOG_ERROR("Unable to resolve symbol 'NvFBCCreateInstance'");
        return -1;
    }

    /*
     * Initialize OpenGL.
     */
    GLXContext glx_ctx;
    GLXFBConfig glx_fb_config;
    NVFBC_BOOL fbc_bool = gl_init(&glx_ctx, &glx_fb_config);
    if (fbc_bool != NVFBC_TRUE) {
        LOG_ERROR("Failed to initialized OpenGL!");
        return -1;
    }

    /*
     * Create an NvFBC instance.
     *
     * API function pointers are accessible through p_fbc_fn.
     */
    memset(&device->p_fbc_fn, 0, sizeof(device->p_fbc_fn));
    device->p_fbc_fn.dwVersion = NVFBC_VERSION;

    fbc_status = nv_fbc_create_instance_ptr(&device->p_fbc_fn);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("Unable to create NvFBC instance (status: %d)", fbc_status);
        return -1;
    }

    /*
     * Create a session handle that is used to identify the client.
     */
    NVFBC_CREATE_HANDLE_PARAMS create_handle_params = {0};
    create_handle_params.dwVersion = NVFBC_CREATE_HANDLE_PARAMS_VER;
    create_handle_params.bExternallyManagedContext = NVFBC_TRUE;
    create_handle_params.glxCtx = glx_ctx;
    create_handle_params.glxFBConfig = glx_fb_config;

    fbc_status = device->p_fbc_fn.nvFBCCreateHandle(&device->fbc_handle, &create_handle_params);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

    /*
     * Get information about the state of the display driver.
     *
     * This call is optional but helps the application decide what it should
     * do.
     */
    NVFBC_GET_STATUS_PARAMS status_params = {0};
    status_params.dwVersion = NVFBC_GET_STATUS_PARAMS_VER;

    fbc_status = device->p_fbc_fn.nvFBCGetStatus(device->fbc_handle, &status_params);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

#if PRINT_STATUS
    NvFBCUtilsPrintStatus(&status_params);
#endif

    if (status_params.bCanCreateNow == NVFBC_FALSE) {
        LOG_ERROR("It is not possible to create a capture session on this system.");
        return -1;
    }

    // Get width and height
    device->width = status_params.screenSize.w;
    device->height = status_params.screenSize.h;

    if (device->width % 4 != 0) {
        LOG_ERROR("Device width must be a multiple of 4!");
        return -1;
    }

    /*
     * Create a capture session.
     */
    LOG_INFO("Creating a capture session of NVidia compressed frames.");

    NVFBC_CREATE_CAPTURE_SESSION_PARAMS create_capture_params = {0};
    NVFBC_SIZE frame_size = {0, 0};
    create_capture_params.dwVersion = NVFBC_CREATE_CAPTURE_SESSION_PARAMS_VER;
    create_capture_params.eCaptureType = NVFBC_CAPTURE_TO_GL;
    create_capture_params.bWithCursor = NVFBC_FALSE;
    create_capture_params.frameSize = frame_size;
    create_capture_params.bRoundFrameSize = NVFBC_TRUE;
    create_capture_params.eTrackingType = NVFBC_TRACKING_DEFAULT;
    create_capture_params.bDisableAutoModesetRecovery = NVFBC_FALSE;

    fbc_status =
        device->p_fbc_fn.nvFBCCreateCaptureSession(device->fbc_handle, &create_capture_params);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }
    /*
     * Set up the capture session.
     */
    NVFBC_TOGL_SETUP_PARAMS setup_params = {0};
    setup_params.dwVersion = NVFBC_TOGL_SETUP_PARAMS_VER;
    setup_params.eBufferFormat = NVFBC_BUFFER_FORMAT_NV12;

    fbc_status = device->p_fbc_fn.nvFBCToGLSetUp(device->fbc_handle, &setup_params);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

    if (create_nvidia_encoder(device, bitrate, requested_codec, &setup_params) != 0) {
        LOG_ERROR("Failed to create nvidia encoder!");
        return -1;
    }

    // Set initial frame pointer to NULL, nvidia will overwrite this with the framebuffer pointer
    device->frame = NULL;
    device->frame_idx = -1;

    /*
     * We are now ready to start grabbing frames.
     */
    LOG_INFO(
        "Nvidia Frame capture session started. New frames will be captured when "
        "the display is refreshed or when the mouse cursor moves.");
    return 0;
}

#define SHOW_DEBUG_FRAMES false

void try_free_frame(NvidiaCaptureDevice* device) {
    NVENCSTATUS enc_status;

    // If there was a previous frame, we should free it first
    if (device->frame != NULL) {
        /*
         * Unlock the bitstream
         */
        enc_status = device->p_enc_fn.nvEncUnlockBitstream(device->encoder, device->output_buffer);
        if (enc_status != NV_ENC_SUCCESS) {
            // If LOG_ERROR is ever desired here in the future,
            // make sure to still UnmapInputResource before returning -1
            LOG_FATAL("Failed to unlock bitstream buffer, status = %d", enc_status);
        }
        /*
         * Unmap the input buffer
         */
        enc_status =
            device->p_enc_fn.nvEncUnmapInputResource(device->encoder, device->input_buffer);
        if (enc_status != NV_ENC_SUCCESS) {
            // FATAL is chosen over ERROR here to prevent
            // out-of-control runaway memory usage
            LOG_FATAL("Failed to unmap the resource, memory is leaking! status = %d", enc_status);
        }
        device->frame = NULL;
    }
}

int nvidia_capture_screen(NvidiaCaptureDevice* device) {
    NVFBCSTATUS fbc_status;
    NVENCSTATUS enc_status;

    bool force_iframe = false;

#if SHOW_DEBUG_FRAMES
    uint64_t t1, t2;
    t1 = NvFBCUtilsGetTimeInMillis();
#endif

    NVFBC_TOGL_GRAB_FRAME_PARAMS grab_params = {0};
    NVFBC_FRAME_GRAB_INFO frame_info;

    grab_params.dwVersion = NVFBC_TOGL_GRAB_FRAME_PARAMS_VER;

    /*
     * Use blocking calls.
     *
     * The application will wait for new frames.  New frames are generated
     * when the mouse cursor moves or when the screen if refreshed.
     */
    grab_params.dwFlags = NVFBC_TOGL_GRAB_FLAGS_NOWAIT;

    /*
     * This structure will contain information about the captured frame.
     */
    grab_params.pFrameGrabInfo = &frame_info;

    /*
     * This structure will contain information about the encoding of
     * the captured frame.
     */
    // grab_params.pEncFrameInfo = &enc_frame_info;

    /*
     * Specify per-frame encoding configuration.  Here, keep the defaults.
     */
    // grab_params.pEncodeParams = NULL;

    /*
     * This pointer is allocated by the NvFBC library and will
     * contain the captured frame.
     *
     * Make sure this pointer stays the same during the capture session
     * to prevent memory leaks.
     */
    // grab_params.ppBitStreamBuffer = (void**)&device->frame;

    int* tracked_packets = flush_tracked_packets();
    memcpy(device->metrics_ids, tracked_packets, sizeof(device->metrics_ids));
    free(tracked_packets);

    /*
     * Capture a new frame.
     */
    fbc_status = device->p_fbc_fn.nvFBCToGLGrabFrame(device->fbc_handle, &grab_params);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

    // TODO: Make this work by factoring out encoder
    // If the frame isn't new, just return 0
    // if (!frame_info.bIsNewFrame) {
    //    return 0;
    //}

    // If the frame is new, then free the old frame and capture+encode the new frame

    // Try to free the device frame
    try_free_frame(device);

    // Set the device to use the newly captured width/height
    device->width = frame_info.dwWidth;
    device->height = frame_info.dwHeight;

    /*
     * Map the frame for use by the encoder.
     */
    NV_ENC_MAP_INPUT_RESOURCE map_params = {0};
    map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    map_params.registeredResource = device->registered_resources[grab_params.dwTextureIndex];
    enc_status = device->p_enc_fn.nvEncMapInputResource(device->encoder, &map_params);
    if (enc_status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to map the resource, status = %d\n", enc_status);
        return -1;
    }
    device->input_buffer = map_params.mappedResource;

    /*
     * Pre-fill frame encoding information
     */
    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    enc_params.inputWidth = device->width;
    enc_params.inputHeight = device->height;
    enc_params.inputPitch = device->width;
    enc_params.inputBuffer = device->input_buffer;
    enc_params.bufferFmt = map_params.mappedBufferFmt;
    // frame_idx starts at -1, so first frame has idx 0
    enc_params.frameIdx = ++device->frame_idx;
    enc_params.outputBitstream = device->output_buffer;
    if (force_iframe) {
        enc_params.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
    }

    /*
     * Encode the frame.
     */
    enc_status = device->p_enc_fn.nvEncEncodePicture(device->encoder, &enc_params);
    if (enc_status != NV_ENC_SUCCESS) {
        // TODO: Unmap the frame! Otherwise, memory leaks here
        LOG_ERROR("Failed to encode frame, status = %d", enc_status);
        return -1;
    }

    /*
     * Get the bitstream and dump to file.
     */
    NV_ENC_LOCK_BITSTREAM lock_params = {0};

    lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
    lock_params.outputBitstream = device->output_buffer;

    enc_status = device->p_enc_fn.nvEncLockBitstream(device->encoder, &lock_params);
    if (enc_status != NV_ENC_SUCCESS) {
        // TODO: Unmap the frame! Otherwise, memory leaks here
        LOG_ERROR("Failed to lock bitstream buffer, status = %d", enc_status);
        return -1;
    }

    device->size = lock_params.bitstreamSizeInBytes;
    device->frame = lock_params.bitstreamBufferPtr;
    device->is_iframe = force_iframe || device->frame_idx == 0;

#if SHOW_DEBUG_FRAMES
    t2 = NvFBCUtilsGetTimeInMillis();

    LOG_INFO("New %dx%d frame id %u size %u%s%s grabbed and saved in %llu ms", frameInfo.dwWidth,
             frameInfo.dwHeight, frameInfo.dwCurrentFrame, device->size,
             encFrameInfo.bIsIFrame ? " (i-frame)" : "",
             frameInfo.bIsNewFrame ? " (new frame)" : "", (unsigned long long)(t2 - t1));
#endif

    return frame_info.dwMissedFrames + 1;
}

void destroy_nvidia_encoder(NvidiaCaptureDevice* device) {
    NVENCSTATUS enc_status;

    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;

    enc_status = device->p_enc_fn.nvEncEncodePicture(device->encoder, &enc_params);
    if (enc_status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to flush the encoder, status = %d", enc_status);
    }

    if (device->output_buffer) {
        enc_status =
            device->p_enc_fn.nvEncDestroyBitstreamBuffer(device->encoder, device->output_buffer);
        if (enc_status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to destroy buffer, status = %d", enc_status);
        }
        device->output_buffer = NULL;
    }

    /*
     * Unregister all the resources that we had registered earlier with the
     * encoder.
     */
    for (int i = 0; i < NVFBC_TOGL_TEXTURES_MAX; i++) {
        if (device->registered_resources[i]) {
            enc_status = device->p_enc_fn.nvEncUnregisterResource(device->encoder,
                                                                  device->registered_resources[i]);
            if (enc_status != NV_ENC_SUCCESS) {
                LOG_ERROR("Failed to unregister resource, status = %d", enc_status);
            }
            device->registered_resources[i] = NULL;
        }
    }

    /*
     * Destroy the encode session
     */
    enc_status = device->p_enc_fn.nvEncDestroyEncoder(device->encoder);
    if (enc_status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to destroy encoder, status = %d", enc_status);
    }
}

void destroy_nvidia_capture_device(NvidiaCaptureDevice* device) {
    NVFBCSTATUS fbc_status;

    // Try to free a device frame
    try_free_frame(device);

    // Destroy the encoder first
    destroy_nvidia_encoder(device);

    /*
     * Destroy capture session, tear down resources.
     */
    NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroy_capture_params = {0};
    destroy_capture_params.dwVersion = NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER;

    fbc_status =
        device->p_fbc_fn.nvFBCDestroyCaptureSession(device->fbc_handle, &destroy_capture_params);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return;
    }

    /*
     * Destroy session handle, tear down more resources.
     */
    NVFBC_DESTROY_HANDLE_PARAMS destroy_handle_params = {0};
    destroy_handle_params.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER;

    fbc_status = device->p_fbc_fn.nvFBCDestroyHandle(device->fbc_handle, &destroy_handle_params);
    if (fbc_status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return;
    }
}
