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
    if (device->p_gpu_texture == NULL) {
        LOG_ERROR("Nothing to register! Device GPU texture was NULL");
        return -1;
    }

    NV_ENC_REGISTER_RESOURCE register_params;
    // register the device's resources to the encoder
    register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
    register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    register_params.width = device->width;
    register_params.height = device->height;
    register_params.pitch = device->width;
    register_params.resourceToRegister = device->p_gpu_texture;
    register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

    int status =
        encoder->p_enc_fn.nvEncRegisterResource(encoder->internal_nvidia_encoder, &register_params);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to register texture, status = %d", status);
        return -1;
    }
    encoder->registered_resource = register_params.registeredResource;
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
    if (!encoder->registered_resource) {
        LOG_INFO("Trying to unregister NULL resource - nothing to do!");
        return 0;
    }

    // unregister all resources in encoder->registered_resources
    int status = encoder->p_enc_fn.nvEncUnregisterResource(encoder->internal_nvidia_encoder,
                                                           encoder->registered_resource);
    if (status != NV_ENC_SUCCESS) {
        LOG_ERROR("Failed to unregister resource, status = %d", status);
        return -1;
    }
    encoder->registered_resource = NULL;
    return 0;
}
