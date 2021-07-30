#include "nvidia_encode.h"
#include "cudacontext.h"

#include <dlfcn.h>
#include <string.h>

#define LIB_ENCODEAPI_NAME "libnvidia-encode.so.1"

int initialize_preset_config(NvidiaEncoder* encoder, int bitrate, CodecType codec,
                             NV_ENC_PRESET_CONFIG* p_preset_config);
GUID get_codec_guid(CodecType codec);
int register_resource(NvidiaEncoder* encoder, RegisteredResource* resource_to_register);
void unregister_resource(NvidiaEncoder* encoder, RegisteredResource registered_resource);

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

NvidiaEncoder* create_nvidia_encoder(int bitrate, CodecType codec, int out_width, int out_height,
                                     CUcontext cuda_context) {
    LOG_INFO("Creating encoder of bitrate %d, codec %d, width %d, height %d, cuda_context %x",
             bitrate, codec, out_width, out_height, cuda_context);
    NVENCSTATUS status;

    // Initialize the encoder
    NvidiaEncoder* encoder = malloc(sizeof(NvidiaEncoder));
    memset(encoder, 0, sizeof(*encoder));
    encoder->codec_type = codec;
    encoder->width = out_width;
    encoder->height = out_height;
    encoder->cuda_context = cuda_context;
    encoder->bitrate = bitrate;

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
    encode_session_params.device = cuda_context;

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

int nvidia_encoder_frame_intake(NvidiaEncoder* encoder, RegisteredResource resource_to_register) {
    if (resource_to_register.width != encoder->width ||
        resource_to_register.height != encoder->height) {
        // reconfigure the encoder
        if (!nvidia_reconfigure_encoder(encoder, resource_to_register.width,
                                        resource_to_register.height, encoder->bitrate,
                                        encoder->codec_type)) {
            LOG_ERROR("Reconfigure failed!");
            return -1;
        }
    }
    if (!resource_to_register.texture_pointer) {
        LOG_ERROR("NULL texture passed into encoder! Doing nothing.");
        return -1;
    }
    bool cache_hit = false;
    // check the cache for a registered resource
    for (int i = 0; i < RESOURCE_CACHE_SIZE; i++) {
        RegisteredResource cached_resource = encoder->resource_cache[i];
        if (cached_resource.texture_pointer == resource_to_register.texture_pointer &&
            cached_resource.width == resource_to_register.width &&
            cached_resource.height == resource_to_register.height &&
            cached_resource.device_type == resource_to_register.device_type) {
            encoder->registered_resource = encoder->resource_cache[i];
            LOG_DEBUG("Using cached resource at entry %d", i);
            cache_hit = true;
            break;
        }
    }
    if (!cache_hit) {
        // no cache hit: we register the resource
        if (encoder->resource_cache[RESOURCE_CACHE_SIZE - 1].texture_pointer != NULL) {
            unregister_resource(encoder, encoder->resource_cache[RESOURCE_CACHE_SIZE - 1]);
        }
        for (int i = RESOURCE_CACHE_SIZE - 1; i > 0; i--) {
            encoder->resource_cache[i] = encoder->resource_cache[i - 1];
        }
        int result = register_resource(encoder, &resource_to_register);
        if (result < 0) {
            LOG_ERROR("Failed to register resource!");
            return -1;
        }
        encoder->resource_cache[0] = resource_to_register;
        encoder->registered_resource = encoder->resource_cache[0];
    }
    LOG_DEBUG("Registered resource data: texture %x, width %d, height %d, pitch %d, device %s",
              encoder->registered_resource.texture_pointer, encoder->registered_resource.width,
              encoder->registered_resource.height, encoder->registered_resource.pitch,
              encoder->registered_resource.device_type == NVIDIA_DEVICE ? "Nvidia" : "X11");
    // TODO: if X11, memcpy?
    if (encoder->registered_resource.device_type == X11_DEVICE) {
        NV_ENC_LOCK_INPUT_BUFFER lock_params = {0};
        lock_params.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
        lock_params.inputBuffer = encoder->registered_resource.handle;
        NVENCSTATUS status =
            encoder->p_enc_fn.nvEncLockInputBuffer(encoder->internal_nvidia_encoder, &lock_params);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to lock input buffer, error %d", status);
            return -1;
        }
        encoder->pitch = lock_params.pitch;
       // encoder->pitch = encoder->registered_resource.pitch;
        LOG_DEBUG("width: %d, pitch: %d (lock pitch %d)", encoder->registered_resource.width, encoder->pitch, lock_params.pitch);
        // memcpy input data
        for(int i = 0; i < encoder->registered_resource.height; i++) {
            memcpy(
                lock_params.bufferDataPtr + i * lock_params.pitch,
                encoder->registered_resource.texture_pointer + i * encoder->registered_resource.pitch,
                encoder->registered_resource.pitch);
        }
        status = encoder->p_enc_fn.nvEncUnlockInputBuffer(encoder->internal_nvidia_encoder,
                                                          lock_params.inputBuffer);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to unlock input buffer, error %d", status);
            return -1;
        }
    } else {
        encoder->pitch = encoder->registered_resource.pitch;
    }
    return 0;
}

int register_resource(NvidiaEncoder* encoder, RegisteredResource* resource_to_register) {
    switch (resource_to_register->device_type) {
        case NVIDIA_DEVICE: {
            LOG_DEBUG("On nvidia");
            if (resource_to_register->texture_pointer == NULL) {
                LOG_ERROR("Tried to register NULL resource, exiting");
                return -1;
            }
            NV_ENC_REGISTER_RESOURCE register_params = {0};
            // register the device's resources to the encoder
            register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
            register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
            register_params.width = resource_to_register->width;
            register_params.height = resource_to_register->height;
            register_params.pitch = resource_to_register->pitch;
            register_params.resourceToRegister = resource_to_register->texture_pointer;
            register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

            int status = encoder->p_enc_fn.nvEncRegisterResource(encoder->internal_nvidia_encoder,
                                                                 &register_params);
            if (status != NV_ENC_SUCCESS) {
                LOG_ERROR("Failed to register texture, status = %d", status);
                return -1;
            }
            resource_to_register->handle = register_params.registeredResource;
            return 0;
        }
        case X11_DEVICE: {
            LOG_DEBUG("On X11");
            NV_ENC_CREATE_INPUT_BUFFER input_params = {0};
            input_params.version = NV_ENC_CREATE_INPUT_BUFFER_VER;
            input_params.width = resource_to_register->width;
            input_params.height = resource_to_register->height;
            input_params.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
            // input_params.pSysMemBuffer = resource_to_register->texture_pointer;
            int status = encoder->p_enc_fn.nvEncCreateInputBuffer(encoder->internal_nvidia_encoder,
                                                                  &input_params);
            if (status != NV_ENC_SUCCESS) {
                LOG_ERROR("Failed to create input buffer, status = %d", status);
                return -1;
            }
            encoder->buffer_fmt = input_params.bufferFmt;
            resource_to_register->handle = input_params.inputBuffer;
            return 0;
        }
        default:
            LOG_ERROR("Unknown device type: %d");
            return -1;
    }
}

void unregister_resource(NvidiaEncoder* encoder, RegisteredResource registered_resource) {
    if (!registered_resource.handle) {
        LOG_INFO("Trying to unregister NULL resource - nothing to do!");
        return;
    }
    if (registered_resource.device_type == NVIDIA_DEVICE) {
        int status = encoder->p_enc_fn.nvEncUnregisterResource(encoder->internal_nvidia_encoder,
                                                               registered_resource.handle);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to unregister resource, status = %d", status);
        }
    } else {
        int status = encoder->p_enc_fn.nvEncDestroyInputBuffer(encoder->internal_nvidia_encoder,
                                                               registered_resource.handle);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to destroy input buffer, status = %d", status);
        }
    }
}

int nvidia_encoder_encode(NvidiaEncoder* encoder) {
    NVENCSTATUS status;

    // Register the frame intake
    RegisteredResource registered_resource = encoder->registered_resource;
    if (registered_resource.handle == NULL) {
        LOG_ERROR("Frame intake failed! No resource to map and encode");
        return -1;
    }
    NV_ENC_MAP_INPUT_RESOURCE map_params = {0};
    if (registered_resource.device_type == NVIDIA_DEVICE) {
        map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
        map_params.registeredResource = registered_resource.handle;
        status =
            encoder->p_enc_fn.nvEncMapInputResource(encoder->internal_nvidia_encoder, &map_params);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to map the resource, status = %d\n", status);
            return -1;
        }
        encoder->buffer_fmt = map_params.mappedBufferFmt;
    }

    // Try to free the encoder's previous frame
    try_free_frame(encoder);

    // Fill in the frame encoding information
    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    enc_params.inputWidth = encoder->width;
    enc_params.inputHeight = encoder->height;
    enc_params.inputPitch = encoder->width;
    if (registered_resource.device_type == NVIDIA_DEVICE) {
        enc_params.inputBuffer = map_params.mappedResource;
    } else {
        enc_params.inputBuffer = registered_resource.handle;
    }
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

    if (registered_resource.device_type == NVIDIA_DEVICE) {
        // Unmap the input buffer
        status = encoder->p_enc_fn.nvEncUnmapInputResource(encoder->internal_nvidia_encoder,
                                                           map_params.mappedResource);
        if (status != NV_ENC_SUCCESS) {
            // FATAL is chosen over ERROR here to prevent
            // out-of-control runaway memory usage
            LOG_FATAL("Failed to unmap the resource, memory is leaking! status = %d", status);
        }
    }
    encoder->registered_resource.handle = NULL;

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
    p_preset_config->presetCfg.gopLength = NVENC_INFINITE_GOPLENGTH;
    p_preset_config->presetCfg.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
    p_preset_config->presetCfg.rcParams.maxBitRate = 4 * bitrate;
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

    LOG_INFO("Reconfiguring encoder to width %d, height %d, bitrate %d, codec %d", width, height,
             bitrate, codec);
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

void nvidia_set_iframe(NvidiaEncoder* encoder) {
    if (encoder == NULL) {
        LOG_ERROR("nvidia_set_iframe received NULL encoder!");
        return;
    }
    encoder->wants_iframe = true;
}

void destroy_nvidia_encoder(NvidiaEncoder* encoder) {
    // Try to free the encoder's frame
    LOG_INFO("Trying to free frame...");
    try_free_frame(encoder);
    LOG_INFO("Freed frame!");

    NVENCSTATUS status;

    NV_ENC_PIC_PARAMS enc_params = {0};
    enc_params.version = NV_ENC_PIC_PARAMS_VER;
    enc_params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;

    LOG_INFO("Sending EOS to encoder...");
    status = encoder->p_enc_fn.nvEncEncodePicture(encoder->internal_nvidia_encoder, &enc_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to flush the encoder, status = %d", status);
    }
    LOG_INFO("Sent EOS!");

    if (encoder->output_buffer) {
        LOG_INFO("Destroying bitstream buffer...");
        status = encoder->p_enc_fn.nvEncDestroyBitstreamBuffer(encoder->internal_nvidia_encoder,
                                                               encoder->output_buffer);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to destroy buffer, status = %d", status);
        }
        encoder->output_buffer = NULL;
        LOG_INFO("Destroyed bitstream buffer!");
    }

    // Destroy the encode session
    LOG_INFO("Destroying nvenc device...");
    status = encoder->p_enc_fn.nvEncDestroyEncoder(encoder->internal_nvidia_encoder);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to destroy encoder, status = %d", status);
    }
    LOG_INFO("Destroyed nvenc device!");

    // Free the encoder struct itself
    free(encoder);
}
