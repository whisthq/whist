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

        encoder->frame = NULL;
    }
}

NV_ENC_REGISTERED_PTR register_resource(NvidiaEncoder* encoder, void* p_gpu_texture, int width, int height) {
    NV_ENC_REGISTER_RESOURCE register_params = {0};
    register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
    register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    register_params.width = width;
    register_params.height = height;
    register_params.pitch = width;
    register_params.resourceToRegister = p_gpu_texture;
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

NvidiaEncoder* create_nvidia_encoder(int bitrate, CodecType codec, int out_width, int out_height, void** p_cuda_context) {
    NVENCSTATUS status;

    // Initialize the encoder
    NvidiaEncoder* encoder = malloc(sizeof(NvidiaEncoder));
    memset(encoder, 0, sizeof(*encoder));
    encoder->codec_type = codec;
    encoder->width = out_width;
    encoder->height = out_height;

    // Set initial frame pointer to NULL, nvidia will overwrite this later with the framebuffer
    // pointer
    encoder->frame = NULL;
    encoder->frame_idx = -1;

    // Dynamically load the NvEncodeAPI library
    void* lib_enc = dlopen(LIB_ENCODEAPI_NAME, RTLD_NOW);
    if (lib_enc == NULL) {
        LOG_ERROR("Unable to open '%s' (%s)", LIB_ENCODEAPI_NAME, dlerror());
        free(encoder);
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
        free(encoder);
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
        free(encoder);
        return NULL;
    }

    // Create an encoder session
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encode_session_params = {0};

    encode_session_params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    encode_session_params.apiVersion = NVENCAPI_VERSION;
    encode_session_params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    encode_session_params.device = *p_cuda_context;

    status = encoder->p_enc_fn.nvEncOpenEncodeSessionEx(&encode_session_params,
                                                        &encoder->internal_nvidia_encoder);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to open an encoder session, status = %d", status);
        free(encoder);
        return NULL;
    }

    // Initialize the preset config
    NV_ENC_PRESET_CONFIG preset_config;
    status = initialize_preset_config(encoder, bitrate, encoder->codec_type, &preset_config);
    if (status < 0) {
        LOG_ERROR("custom_preset_config failed");
        free(encoder);
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
    init_params.maxEncodeWidth = MAX_SCREEN_WIDTH;
    init_params.maxEncodeHeight = MAX_SCREEN_HEIGHT;
    init_params.frameRateNum = FPS;
    init_params.frameRateDen = 1;
    init_params.enablePTD = 1;

    // Copy the init params into the encoder for reconfiguration
    encoder->encoder_params = init_params;

    status =
        encoder->p_enc_fn.nvEncInitializeEncoder(encoder->internal_nvidia_encoder, &init_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to initialize the encode session, status = %d", status);
        free(encoder);
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

int nvidia_encoder_frame_intake(NvidiaEncoder* encoder, void* p_gpu_texture,
                                int width, int height) {
    if (width != encoder->width || height != encoder->height) {
        LOG_ERROR(
            "Nvidia Encoder has received a frame_intake of dimensions %dx%d, "
            "but the Nvidia Encoder is only configured for dimensions %dx%d",
            width, height, encoder->width, encoder->height);
        return -1;
    }
    int cache_size =
        sizeof(encoder->registered_resources) / sizeof(encoder->registered_resources[0]);
    // Check the registered resource cache
    for (int i = 0; i < cache_size; i++) {
        InputBufferCacheEntry resource = encoder->registered_resources[i];
        // We include a check against registered_resource NULL for validity
        if (resource.p_gpu_texture == p_gpu_texture &&
            resource.width == width && resource.height == height &&
            resource.registered_resource != NULL) {
            encoder->registered_resource = resource.registered_resource;
            return 0;
        }
    }
    // Unregister the last resource
    if (encoder->registered_resources[cache_size - 1].registered_resource != NULL) {
        unregister_resource(encoder,
                            encoder->registered_resources[cache_size - 1].registered_resource);
        encoder->registered_resources[cache_size - 1].registered_resource = NULL;
    }
    // Move everything up one, overwriting the last resource
    for (int i = cache_size - 1; i > 0; i--) {
        encoder->registered_resources[i] = encoder->registered_resources[i - 1];
    }
    // Invalidate the 0th resource, since it was moved to index 1
    encoder->registered_resources[0].registered_resource = NULL;
    NV_ENC_REGISTERED_PTR registered_resource =
        register_resource(encoder, p_gpu_texture, width, height);
    if (registered_resource == NULL) {
        LOG_ERROR("Failed to register resource!");
        return -1;
    }
    // Overwrite the 0th resource with the newly registered resource
    encoder->registered_resources[0].p_gpu_texture = p_gpu_texture;
    encoder->registered_resources[0].width = width;
    encoder->registered_resources[0].height = height;
    encoder->registered_resources[0].registered_resource = registered_resource;
    encoder->registered_resource = registered_resource;
    return 0;
}

int nvidia_encoder_encode(NvidiaEncoder* encoder) {
    NVENCSTATUS status;

    // Register the frame intake
    NV_ENC_REGISTERED_PTR registered_resource = encoder->registered_resource;
    if (registered_resource == NULL) {
        LOG_ERROR(
            "Failed to register resource! frame_intake must not have been called, or it failed.");
        return -1;
    }

    NV_ENC_MAP_INPUT_RESOURCE map_params = {0};
    map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    map_params.registeredResource = registered_resource;
    // We've now consumed the registered resource for the purpose of mapping the buffer,
    // So we'll not clear the encoder's registered resource so as to not use it twice by accident
    encoder->registered_resource = NULL;
    status = encoder->p_enc_fn.nvEncMapInputResource(encoder->internal_nvidia_encoder, &map_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to map the resource, status = %d\n", status);
        return -1;
    }
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
    enc_params.inputBuffer = map_params.mappedResource;
    enc_params.bufferFmt = encoder->buffer_fmt;
    // frame_idx starts at -1, so first frame has idx 0
    enc_params.frameIdx = ++encoder->frame_idx;
    enc_params.outputBitstream = encoder->output_buffer;
    if (encoder->wants_iframe) {
        enc_params.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR | NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
    }

    // Encode the frame
    status = encoder->p_enc_fn.nvEncEncodePicture(encoder->internal_nvidia_encoder, &enc_params);
    if (status != NV_ENC_SUCCESS) {
        // TODO: Unmap the frame! Otherwise, memory leaks here
        LOG_ERROR("Failed to encode frame, status = %d", status);
        return -1;
    }

    // Unmap the input buffer
    status = encoder->p_enc_fn.nvEncUnmapInputResource(encoder->internal_nvidia_encoder,
                                                       map_params.mappedResource);
    if (status != NV_ENC_SUCCESS) {
        // FATAL is chosen over ERROR here to prevent
        // out-of-control runaway memory usage
        LOG_FATAL("Failed to unmap the resource, memory is leaking! status = %d", status);
    }

    // Lock the bitstream
    NV_ENC_LOCK_BITSTREAM lock_params = {0};

    lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
    lock_params.outputBitstream = encoder->output_buffer;

    status = encoder->p_enc_fn.nvEncLockBitstream(encoder->internal_nvidia_encoder, &lock_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to lock bitstream buffer, status = %d", status);
        return -1;
    }

    encoder->frame_size = lock_params.bitstreamSizeInBytes;
    encoder->frame = lock_params.bitstreamBufferPtr;
    encoder->is_iframe = encoder->wants_iframe || encoder->frame_idx == 0;

    // Untrigger iframe
    encoder->wants_iframe = false;

    return 0;
}

GUID get_codec_guid(CodecType codec) {
    GUID codec_guid = {0};
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

bool nvidia_reconfigure_encoder(NvidiaEncoder* encoder, int width, int height, int bitrate,
                                CodecType codec) {
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

    NVENCSTATUS status;
    // If dimensions or codec type changes,
    // then we need to send an IDR frame
    bool needs_idr_frame =
        width != encoder->width || height != encoder->height || codec != encoder->codec_type;

    // Create reconfigure params
    NV_ENC_RECONFIGURE_PARAMS reconfigure_params = {0};
    reconfigure_params.version = NV_ENC_RECONFIGURE_PARAMS_VER;

    // Copy over init params
    reconfigure_params.reInitEncodeParams = encoder->encoder_params;

    // Check that it's possible to reconfigure into those dimensions
    int max_width = reconfigure_params.reInitEncodeParams.maxEncodeWidth;
    int max_height = reconfigure_params.reInitEncodeParams.maxEncodeHeight;
    if (width > max_width || height > max_height) {
        LOG_ERROR(
            "Trying to reconfigure into dimensions %dx%d,"
            "but the max reconfigure dimensions are %dx%d",
            width, height, max_width, max_height);
    }

    // Initialize preset_config
    NV_ENC_PRESET_CONFIG preset_config;
    if (initialize_preset_config(encoder, bitrate, codec, &preset_config) < 0) {
        LOG_ERROR("Failed to reconfigure encoder!");
        return false;
    }
    reconfigure_params.reInitEncodeParams.encodeConfig = &preset_config.presetCfg;

    // Adjust the init params with new configure,
    // And then copy over these new init params
    reconfigure_params.reInitEncodeParams.encodeWidth = width;
    reconfigure_params.reInitEncodeParams.encodeHeight = height;
    reconfigure_params.reInitEncodeParams.encodeGUID = get_codec_guid(codec);
    NV_ENC_INITIALIZE_PARAMS new_init_params = reconfigure_params.reInitEncodeParams;
    // Reset the encoder stream if the codec has changed,
    // Since the decoder will need an iframe
    if (needs_idr_frame) {
        reconfigure_params.resetEncoder = 1;
        reconfigure_params.forceIDR = 1;
    } else {
        reconfigure_params.resetEncoder = 0;
        reconfigure_params.forceIDR = 0;
    }
    // Set encode_config params, since this is the bitrate and codec stuff we really want to change
    status = encoder->p_enc_fn.nvEncReconfigureEncoder(encoder->internal_nvidia_encoder,
                                                       &reconfigure_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to reconfigure the encoder, status = %d", status);
        return false;
    }

    // Only update the encoder struct here!
    // Since this is after all possible "return false"'s'

    if (needs_idr_frame) {
        // Trigger an i-frame on the next iframe
        encoder->wants_iframe = true;
    }

    encoder->encoder_params = new_init_params;
    encoder->codec_type = codec;
    encoder->width = width;
    encoder->height = height;

    return true;
}

void nvidia_set_iframe(NvidiaEncoder* encoder) { encoder->wants_iframe = true; }

void nvidia_unset_iframe(NvidiaEncoder* encoder) { encoder->wants_iframe = false; }

void destroy_nvidia_encoder(NvidiaEncoder* encoder) {
    LOG_INFO("Destroying nvidia encoder...");

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
    int cache_size =
        sizeof(encoder->registered_resources) / sizeof(encoder->registered_resources[0]);
    // Check the registered resource cache
    for (int i = 0; i < cache_size; i++) {
        if (encoder->registered_resources[i].registered_resource != NULL) {
            unregister_resource(encoder, encoder->registered_resources[i].registered_resource);
            encoder->registered_resources[i].registered_resource = NULL;
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
