#include "nvidia_encode.h"

void try_free_frame(NvidiaEncoder* encoder) {
    NVENCSTATUS enc_status;

    // If there was a previous frame, we should free it first
    if (encoder->frame != NULL) {
        /*
         * Unlock the bitstream
         */
        enc_status = encoder->p_enc_fn.nvEncUnlockBitstream(encoder->internal_nvidia_encoder, encoder->output_buffer);
        if (enc_status != NV_ENC_SUCCESS) {
            // If LOG_ERROR is ever desired here in the future,
            // make sure to still UnmapInputResource before returning -1
            LOG_FATAL("Failed to unlock bitstream buffer, status = %d", enc_status);
        }
        /*
         * Unmap the input buffer
         */
        enc_status =
            encoder->p_enc_fn.nvEncUnmapInputResource(encoder->internal_nvidia_encoder, encoder->input_buffer);
        if (enc_status != NV_ENC_SUCCESS) {
            // FATAL is chosen over ERROR here to prevent
            // out-of-control runaway memory usage
            LOG_FATAL("Failed to unmap the resource, memory is leaking! status = %d", enc_status);
        }
        encoder->frame = NULL;
    }
}

int create_nvidia_encoder(NvidiaCaptureDevice* device, int bitrate, CodecType requested_codec,
                          NVFBC_TOGL_SETUP_PARAMS* p_setup_params, int out_width, int out_height) {
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
    memset(&encoder->p_enc_fn, 0, sizeof(encoder->p_enc_fn));

    encoder->p_enc_fn.version = NV_ENCODE_API_FUNCTION_LIST_VER;

    status = nv_encode_api_create_instance_ptr(&encoder->p_enc_fn);
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

    status = encoder->p_enc_fn.nvEncOpenEncodeSessionEx(&encode_session_params, &encoder->internal_nvidia_encoder);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to open an encoder session, status = %d", status);
        return -1;
    }

    /*
     * Validate the codec requested
     */
    GUID codec_guid;
    if (requested_codec == CODEC_TYPE_H265) {
        encoder->codec_type = CODEC_TYPE_H265;
        codec_guid = NV_ENC_CODEC_HEVC_GUID;
    } else {
        encoder->codec_type = CODEC_TYPE_H264;
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
    status = encoder->p_enc_fn.nvEncGetEncodePresetConfig(
        encoder->internal_nvidia_encoder, codec_guid, NV_ENC_PRESET_LOW_LATENCY_HP_GUID, &preset_config);
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
    init_params.encodeWidth = out_width;
    init_params.encodeHeight = out_height;
    init_params.frameRateNum = FPS;
    init_params.frameRateDen = 1;
    init_params.enablePTD = 1;

    status = encoder->p_enc_fn.nvEncInitializeEncoder(encoder->internal_nvidia_encoder, &init_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to initialize the encode session, status = %d", status);
        return -1;
    }

    /*
     * Register the textures received from NvFBC for use with NvEncodeAPI
     */
    int in_width = out_height;
    int in_height = out_height;
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
        register_params.width = in_width;
        register_params.height = out_height;
        register_params.pitch = in_width;
        register_params.resourceToRegister = &tex_params;
        register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

        status = encoder->p_enc_fn.nvEncRegisterResource(encoder->internal_nvidia_encoder, &register_params);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to register texture, status = %d", status);
            return -1;
        }

        encoder->registered_resources[i] = register_params.registeredResource;
    }

    /*
     * Create a bitstream buffer to hold the output
     */
    NV_ENC_CREATE_BITSTREAM_BUFFER bitstream_buffer_params = {0};
    bitstream_buffer_params.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;

    status = encoder->p_enc_fn.nvEncCreateBitstreamBuffer(encoder->internal_nvidia_encoder, &bitstream_buffer_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to create a bitstream buffer, status = %d", status);
        return -1;
    }

    encoder->output_buffer = bitstream_buffer_params.bitstreamBuffer;
    return 0;
}

void nvidia_encoder_frame_intake(NvidiaEncoder* encoder, void* input_buffer, int width, int height) {
    encoder->width = width;
    encoder->height = height;
}

void nvidia_encoder_encode(NvidiaEncoder* encoder) {
    // Try to free the encoder's previous frame
    try_free_frame(encoder);

    /*
     * Map the frame for use by the encoder.
     */
    NV_ENC_MAP_INPUT_RESOURCE map_params = {0};
    map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    map_params.registeredResource = encoder->registered_resources[grab_params.dwTextureIndex];
    enc_status = encoder->p_enc_fn.nvEncMapInputResource(encoder->internal_nvidia_encoder, &map_params);
    if (enc_status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to map the resource, status = %d\n", enc_status);
        return -1;
    }
    encoder->input_buffer = map_params.mappedResource;

    /*
     * Pre-fill frame encoding information
     */
    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    enc_params.inputWidth = encoder->width;
    enc_params.inputHeight = encoder->height;
    enc_params.inputPitch = encoder->width;
    enc_params.inputBuffer = encoder->input_buffer;
    enc_params.bufferFmt = map_params.mappedBufferFmt;
    // frame_idx starts at -1, so first frame has idx 0
    enc_params.frameIdx = ++encoder->frame_idx;
    enc_params.outputBitstream = encoder->output_buffer;
    if (force_iframe) {
        enc_params.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
    }

    /*
     * Encode the frame.
     */
    enc_status = encoder->p_enc_fn.nvEncEncodePicture(encoder->internal_nvidia_encoder, &enc_params);
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
    lock_params.outputBitstream = encoder->output_buffer;

    enc_status = encoder->p_enc_fn.nvEncLockBitstream(encoder->internal_nvidia_encoder, &lock_params);
    if (enc_status != NV_ENC_SUCCESS) {
        // TODO: Unmap the frame! Otherwise, memory leaks here
        LOG_ERROR("Failed to lock bitstream buffer, status = %d", enc_status);
        return -1;
    }

    encoder->size = lock_params.bitstreamSizeInBytes;
    encoder->frame = lock_params.bitstreamBufferPtr;
    encoder->is_iframe = force_iframe || encoder->frame_idx == 0;
}

void destroy_nvidia_encoder(NvidiaEncode* encoder) {
    // Try to free the encoder's frame
    try_free_frame(encoder);

    NVENCSTATUS enc_status;

    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;

    enc_status = encoder->p_enc_fn.nvEncEncodePicture(encoder->internal_nvidia_encoder, &enc_params);
    if (enc_status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to flush the encoder, status = %d", enc_status);
    }

    if (encoder->output_buffer) {
        enc_status =
            encoder->p_enc_fn.nvEncDestroyBitstreamBuffer(encoder->internal_nvidia_encoder, encoder->output_buffer);
        if (enc_status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to destroy buffer, status = %d", enc_status);
        }
        encoder->output_buffer = NULL;
    }

    /*
     * Unregister all the resources that we had registered earlier with the
     * encoder.
     */
    for (int i = 0; i < NVFBC_TOGL_TEXTURES_MAX; i++) {
        if (encoder->registered_resources[i]) {
            enc_status = encoder->p_enc_fn.nvEncUnregisterResource(encoder->internal_nvidia_encoder,
                                                                  encoder->registered_resources[i]);
            if (enc_status != NV_ENC_SUCCESS) {
                LOG_ERROR("Failed to unregister resource, status = %d", enc_status);
            }
            encoder->registered_resources[i] = NULL;
        }
    }

    /*
     * Destroy the encode session
     */
    enc_status = encoder->p_enc_fn.nvEncDestroyEncoder(encoder->internal_nvidia_encoder);
    if (enc_status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to destroy encoder, status = %d", enc_status);
    }
}