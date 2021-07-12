#include "nvidiatransfercapture.h"

int nvidia_start_transfer_context(NvidiaCaptureDevice* device, NvidiaEncoder* encoder) {
    /*
        Register the device's GL capture textures with the encoder. Call this whenever we have an
        updated/recreated device/encoder.

        Arguments:
            device (NvidiaCaptureDevice*): device with screen capturing resources
            encoder (NvidiaEncoder*): encoder which will be encoding the screen captures

        Returns:
            (int): 0 on success, -1 on failure
        */
    // register the device's resources to the encoder
    for (int i = 0; i < NVFBC_TOGL_TEXTURES_MAX; i++) {
        NV_ENC_REGISTER_RESOURCE register_params;
        NV_ENC_INPUT_RESOURCE_OPENGL_TEX tex_params;

        // the device stores the textures and targets in togl_setup_params.
        if (!device->togl_setup_params.dwTextures[i]) {
            break;
        }
        memset(&register_params, 0, sizeof(register_params));

        tex_params.texture = device->togl_setup_params.dwTextures[i];
        tex_params.target = device->togl_setup_params.dwTexTarget;

        register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
        register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_OPENGL_TEX;
        register_params.width = device->width;
        register_params.height = device->height;
        register_params.pitch = device->width;
        register_params.resourceToRegister = &tex_params;
        register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

        int status = encoder->p_enc_fn.nvEncRegisterResource(encoder->internal_nvidia_encoder,
                                                             &register_params);
        if (status != NV_ENC_SUCCESS) {
            LOG_ERROR("Failed to register texture, status = %d", status);
            return -1;
        }

        encoder->registered_resources[i] = register_params.registeredResource;
    }
    return 0;
}

int nvidia_close_transfer_context(NvidiaEncoder* encoder) {
    /*
        Unregister the encoder's registered resources. This function must be called if either the
        capture device or encoder changes resolution or is destroyed.

        Arguments:
            encoder (NvidiaEncoder*): encoder with registered resources which are no longer
       associated to a capture device

        Returns:
            (int): 0 on success, -1 on failure
        */
    // unregister all resources in encoder->registered_resources
    for (int i = 0; i < NVFBC_TOGL_TEXTURES_MAX; i++) {
        if (encoder->registered_resources[i]) {
            int status = encoder->p_enc_fn.nvEncUnregisterResource(
                encoder->internal_nvidia_encoder, encoder->registered_resources[i]);
            if (status != NV_ENC_SUCCESS) {
                LOG_ERROR("Failed to unregister resource, status = %d", status);
            }
            encoder->registered_resources[i] = NULL;
        }
    }
}
