#include "nvidia_encode.h"

#include <dlfcn.h>
#include <string.h>

#define LIB_ENCODEAPI_NAME "libnvidia-encode.so.1"

int initialize_preset_config(NvidiaEncoder* encoder, int bitrate, CodecType codec,
                             NV_ENC_PRESET_CONFIG* p_preset_config);
GUID get_codec_guid(CodecType codec);

void try_free_frame(NvidiaEncoder* encoder) {
    NVENCSTATUS status;

    // If there was a previous frame, we should free it first
    if (encoder->frame != NULL) {
        // Unlock the bitstream
        status = encoder->p_enc_fn.nvEncUnlockBitstream(encoder->internal_nvidia_encoder,
                                                        encoder->output_buffer);
        if (status != NV_ENC_SUCCESS) {
            // If LOG_ERROR is ever desired here in the future,
            // make sure to still UnmapInputResource before returning -1
            LOG_FATAL("Failed to unlock bitstream buffer, status = %d", status);
        }

        // Unmap the input buffer
        status = encoder->p_enc_fn.nvEncUnmapInputResource(encoder->internal_nvidia_encoder,
                                                           encoder->input_buffer);
        if (status != NV_ENC_SUCCESS) {
            // FATAL is chosen over ERROR here to prevent
            // out-of-control runaway memory usage
            LOG_FATAL("Failed to unmap the resource, memory is leaking! status = %d", status);
        }
        encoder->frame = NULL;
    }
}

NV_ENC_REGISTERED_PTR register_resource(NvidiaEncoder* encoder, uint32_t dw_texture,
                                        uint32_t dw_tex_target, int width, int height) {
    NV_ENC_INPUT_RESOURCE_OPENGL_TEX tex_params = {0};
    tex_params.texture = dw_texture;
    tex_params.target = dw_tex_target;

    NV_ENC_REGISTER_RESOURCE register_params = {0};
    register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
    register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_OPENGL_TEX;
    register_params.width = width;
    register_params.height = height;
    register_params.pitch = width;
    register_params.resourceToRegister = &tex_params;
    register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

    NVENCSTATUS status =
        encoder->p_enc_fn.nvEncRegisterResource(encoder->internal_nvidia_encoder, &register_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to register texture, status = %d", status);
        return NULL;
    }

    return register_params.registeredResource;
}

void unregister_resource(NvidiaEncoder* encoder, NV_ENC_REGISTERED_PTR registered_resource) {
    NVENCSTATUS status = encoder->p_enc_fn.nvEncUnregisterResource(encoder->internal_nvidia_encoder,
                                                                   registered_resource);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to unregister resource, status = %d", status);
    }
}

NvidiaEncoder* create_nvidia_encoder(int bitrate, CodecType codec, int out_width, int out_height) {
    NVENCSTATUS status;

    // Initialize the encoder
    NvidiaEncoder* encoder = malloc(sizeof(NvidiaEncoder));
    memset(encoder, 0, sizeof(*encoder));
    encoder->codec_type = codec;

    // Set initial frame pointer to NULL, nvidia will overwrite this later with the framebuffer
    // pointer
    encoder->frame = NULL;
    encoder->frame_idx = -1;

    // Dynamically load the NvEncodeAPI library
    void* lib_enc = dlopen(LIB_ENCODEAPI_NAME, RTLD_NOW);
    if (lib_enc == NULL) {
        LOG_ERROR("Unable to open '%s' (%s)", LIB_ENCODEAPI_NAME, dlerror());
        return NULL;
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
        return NULL;
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
        return NULL;
    }

    // Create an encoder session
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encode_session_params = {0};

    encode_session_params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    encode_session_params.apiVersion = NVENCAPI_VERSION;
    encode_session_params.deviceType = NV_ENC_DEVICE_TYPE_OPENGL;

    status = encoder->p_enc_fn.nvEncOpenEncodeSessionEx(&encode_session_params,
                                                        &encoder->internal_nvidia_encoder);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to open an encoder session, status = %d", status);
        return NULL;
    }

    // Initialize the preset config
    NV_ENC_PRESET_CONFIG preset_config;
    status = initialize_preset_config(encoder, bitrate, encoder->codec_type, &preset_config);
    if (status < 0) {
        LOG_ERROR("custom_preset_config failed");
        return NULL;
    }

    // Initialize the encode session
    NV_ENC_INITIALIZE_PARAMS init_params = {0};
    init_params.version = NV_ENC_INITIALIZE_PARAMS_VER;
    init_params.encodeGUID = get_codec_guid(encoder->codec_type);
    init_params.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
    init_params.encodeConfig = &preset_config.presetCfg;
    init_params.encodeWidth = out_width;
    init_params.encodeHeight = out_height;
    init_params.frameRateNum = FPS;
    init_params.frameRateDen = 1;
    init_params.enablePTD = 1;

    // Copy the init params into the encoder for reconfiguration
    encoder->encoder_params = init_params;

    status =
        encoder->p_enc_fn.nvEncInitializeEncoder(encoder->internal_nvidia_encoder, &init_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to initialize the encode session, status = %d", status);
        return NULL;
    }

    // Create a bitstream buffer to hold the output
    NV_ENC_CREATE_BITSTREAM_BUFFER bitstream_buffer_params = {0};
    bitstream_buffer_params.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;

    status = encoder->p_enc_fn.nvEncCreateBitstreamBuffer(encoder->internal_nvidia_encoder,
                                                          &bitstream_buffer_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to create a bitstream buffer, status = %d", status);
        return NULL;
    }

    encoder->output_buffer = bitstream_buffer_params.bitstreamBuffer;
    return encoder;
}

int nvidia_encoder_frame_intake(NvidiaEncoder* encoder, uint32_t dw_texture, uint32_t dw_tex_target,
                                int width, int height) {
    encoder->dw_texture = dw_texture;
    encoder->dw_tex_target = dw_tex_target;
    encoder->width = width;
    encoder->height = height;
    return 0;
}

int nvidia_encoder_encode(NvidiaEncoder* encoder) {
    bool force_iframe = false;
    NVENCSTATUS status;

    // Register the frame intake
    NV_ENC_REGISTERED_PTR registered_resource = register_resource(
        encoder, encoder->dw_texture, encoder->dw_tex_target, encoder->width, encoder->height);
    if (registered_resource == NULL) {
        LOG_ERROR("Failed to register resource!");
        return -1;
    }

    NV_ENC_MAP_INPUT_RESOURCE map_params = {0};
    map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    map_params.registeredResource = registered_resource;
    status = encoder->p_enc_fn.nvEncMapInputResource(encoder->internal_nvidia_encoder, &map_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to map the resource, status = %d\n", status);
        unregister_resource(encoder, registered_resource);
        return -1;
    }
    LOG_ERROR("SUCCESSE!!!!!!!!!!!!!!!!!!!!!!!!!!");
    encoder->input_buffer = map_params.mappedResource;
    encoder->buffer_fmt = map_params.mappedBufferFmt;

    // Try to free the encoder's previous frame
    try_free_frame(encoder);

    // Fill in the frame encoding information
    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    enc_params.inputWidth = encoder->width;
    enc_params.inputHeight = encoder->height;
    enc_params.inputPitch = encoder->width;
    enc_params.inputBuffer = encoder->input_buffer;
    enc_params.bufferFmt = encoder->buffer_fmt;
    // frame_idx starts at -1, so first frame has idx 0
    enc_params.frameIdx = ++encoder->frame_idx;
    enc_params.outputBitstream = encoder->output_buffer;
    if (force_iframe) {
        enc_params.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
    }

    // Encode the frame
    status = encoder->p_enc_fn.nvEncEncodePicture(encoder->internal_nvidia_encoder, &enc_params);
    if (status != NV_ENC_SUCCESS) {
        // TODO: Unmap the frame! Otherwise, memory leaks here
        LOG_ERROR("Failed to encode frame, status = %d", status);
        return -1;
    }

    // Unregister the frame intake
    unregister_resource(encoder, registered_resource);

    // Lock the bitstream
    NV_ENC_LOCK_BITSTREAM lock_params = {0};

    lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
    lock_params.outputBitstream = encoder->output_buffer;

    status = encoder->p_enc_fn.nvEncLockBitstream(encoder->internal_nvidia_encoder, &lock_params);
    if (status != NV_ENC_SUCCESS) {
        // TODO: Unmap the frame! Otherwise, memory leaks here
        LOG_ERROR("Failed to lock bitstream buffer, status = %d", status);
        return -1;
    }

    encoder->frame_size = lock_params.bitstreamSizeInBytes;
    encoder->frame = lock_params.bitstreamBufferPtr;
    encoder->is_iframe = force_iframe || encoder->frame_idx == 0;

    return 0;
}

GUID get_codec_guid(CodecType codec) {
    GUID codec_guid;
    if (codec == CODEC_TYPE_H265) {
        codec_guid = NV_ENC_CODEC_HEVC_GUID;
    } else if (codec == CODEC_TYPE_H264) {
        codec_guid = NV_ENC_CODEC_H264_GUID;
    } else {
        LOG_FATAL("Unexpected CodecType: %d", (int)codec);
    }

    // Validate the codec requested
    // status = validateEncodeGUID(encoder, codec_guid);
    // if (status != NV_ENC_SUCCESS) {
    //     LOG_ERROR("Failed to validate codec GUID");
    //     return NULL;
    // }

    return codec_guid;
}

int initialize_preset_config(NvidiaEncoder* encoder, int bitrate, CodecType codec,
                             NV_ENC_PRESET_CONFIG* p_preset_config) {
    memset(p_preset_config, 0, sizeof(*p_preset_config));

    p_preset_config->version = NV_ENC_PRESET_CONFIG_VER;
    p_preset_config->presetCfg.version = NV_ENC_CONFIG_VER;
    NVENCSTATUS status = encoder->p_enc_fn.nvEncGetEncodePresetConfig(
        encoder->internal_nvidia_encoder, get_codec_guid(codec), NV_ENC_PRESET_LOW_LATENCY_HP_GUID,
        p_preset_config);
    // Check for bad status
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to obtain preset settings, status = %d", status);
        return -1;
    }
    p_preset_config->presetCfg.gopLength = 999999;
    p_preset_config->presetCfg.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
    p_preset_config->presetCfg.rcParams.averageBitRate = bitrate;
    p_preset_config->presetCfg.rcParams.vbvBufferSize = bitrate;

    return 0;
}

int reconfigure_nvidia_encoder(NvidiaEncoder* encoder, int bitrate, CodecType codec) {
    /*
        Using Nvidia's Reconfigure API, update encoder bitrate and codec without
       destruction/recreation.

        Arguments:
            encoder (NvidiaEncoder*): encoder to update
            codec (CodecType): new codec
            bitrate (int): new bitrate

        Returns:
            (int): 0 on success, -1 on failure
    */

    // Create reconfigure params
    NV_ENC_RECONFIGURE_PARAMS reconfigure_params;
    memset(&reconfigure_params, 0, sizeof(reconfigure_params));
    reconfigure_params.version = NV_ENC_RECONFIGURE_PARAMS_VER;

    // Copy over init params
    reconfigure_params.reInitEncodeParams = encoder->encoder_params;
    encoder->codec_type = codec;
    encoder->encoder_params.encodeGUID = get_codec_guid(codec);

    // Initialize preset_config
    NV_ENC_PRESET_CONFIG preset_config;
    if (initialize_preset_config(encoder, bitrate, codec, &preset_config) < 0) {
        LOG_ERROR("Failed to reconfigure encoder!");
        return -1;
    }
    reconfigure_params.reInitEncodeParams.encodeConfig = &preset_config.presetCfg;

    // Copy over new init params
    memcpy(&encoder->encoder_params, &reconfigure_params.reInitEncodeParams,
           sizeof(encoder->encoder_params));
    // Not sure if we need this, but just in case
    reconfigure_params.resetEncoder = 0;
    reconfigure_params.forceIDR = 0;
    // Set encode_config params, since this is the bitrate and codec stuff we really want to change
    NVENCSTATUS status = encoder->p_enc_fn.nvEncReconfigureEncoder(encoder->internal_nvidia_encoder,
                                                                   &reconfigure_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to reconfigure the encoder, status = %d", status);
        return -1;
    }
    return 0;
}

void destroy_nvidia_encoder(NvidiaEncoder* encoder) {
    // Try to free the encoder's frame
    try_free_frame(encoder);

    NVENCSTATUS status;

    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;

    status = encoder->p_enc_fn.nvEncEncodePicture(encoder->internal_nvidia_encoder, &enc_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to flush the encoder, status = %d", status);
    }

    if (encoder->output_buffer) {
        status = encoder->p_enc_fn.nvEncDestroyBitstreamBuffer(encoder->internal_nvidia_encoder,
                                                               encoder->output_buffer);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to destroy buffer, status = %d", status);
        }
        encoder->output_buffer = NULL;
    }

    // Unregister all the resources that we had registered earlier
    for (int i = 0; i < NVFBC_TOGL_TEXTURES_MAX; i++) {
        if (encoder->registered_resources[i]) {
            status = encoder->p_enc_fn.nvEncUnregisterResource(encoder->internal_nvidia_encoder,
                                                               encoder->registered_resources[i]);
            if (status != NV_ENC_SUCCESS) {
                LOG_ERROR("Failed to unregister resource, status = %d", status);
            }
            encoder->registered_resources[i] = NULL;
        }
    }

    // Destroy the encode session
    status = encoder->p_enc_fn.nvEncDestroyEncoder(encoder->internal_nvidia_encoder);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to destroy encoder, status = %d", status);
    }

    // Free the encoder struct itself
    free(encoder);
}
