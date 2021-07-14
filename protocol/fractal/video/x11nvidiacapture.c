#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>

#include "x11capture.h"
#include "x11nvidiacapture.h"
#include <GL/glx.h>

// NOTE: Using Nvidia Capture SDK 8.0.4
// Please bump this comment if a newer Nvidia Capture SDK is going to be used

#define APP_VERSION 4

#define PRINT_STATUS false

#define LIB_NVFBC_NAME "libnvidia-fbc.so.1"

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

int create_nvidia_capture_device(NvidiaCaptureDevice* device) {
    // 0-initialize everything in the NvidiaCaptureDevice
    memset(device, 0, sizeof(NvidiaCaptureDevice));

    char output_name[NVFBC_OUTPUT_NAME_LEN];
    uint32_t output_id = 0;

    NVFBCSTATUS status;

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

    status = nv_fbc_create_instance_ptr(&device->p_fbc_fn);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("Unable to create NvFBC instance (status: %d)", status);
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

    status = device->p_fbc_fn.nvFBCCreateHandle(&device->fbc_handle, &create_handle_params);
    if (status != NVFBC_SUCCESS) {
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

    status = device->p_fbc_fn.nvFBCGetStatus(device->fbc_handle, &status_params);
    if (status != NVFBC_SUCCESS) {
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

    status = device->p_fbc_fn.nvFBCCreateCaptureSession(device->fbc_handle, &create_capture_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

    /*
     * Set up the capture session.
     */
    memset(&device->togl_setup_params, 0, sizeof(device->togl_setup_params));
    device->togl_setup_params.dwVersion = NVFBC_TOGL_SETUP_PARAMS_VER;
    device->togl_setup_params.eBufferFormat = NVFBC_BUFFER_FORMAT_NV12;

    status = device->p_fbc_fn.nvFBCToGLSetUp(device->fbc_handle, &device->togl_setup_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

    /*
     * We are now ready to start grabbing frames.
     */
    LOG_INFO(
        "Nvidia Frame capture session started. New frames will be captured when "
        "the display is refreshed or when the mouse cursor moves.");

    return 0;
}

#define SHOW_DEBUG_FRAMES false

int nvidia_capture_screen(NvidiaCaptureDevice* device) {
    NVFBCSTATUS status;

#if SHOW_DEBUG_FRAMES
    uint64_t t1, t2;
    t1 = NvFBCUtilsGetTimeInMillis();
#endif

    NVFBC_TOGL_GRAB_FRAME_PARAMS grab_params = {0};
    NVFBC_FRAME_GRAB_INFO frame_info = {0};

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

    /*
     * Capture a new frame.
     */
    status = device->p_fbc_fn.nvFBCToGLGrabFrame(device->fbc_handle, &grab_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

    device->dw_texture_index = grab_params.dwTextureIndex;

    // If the frame isn't new, just return 0
    if (!frame_info.bIsNewFrame) {
        return 0;
    }

    // Set the device to use the newly captured width/height
    device->width = frame_info.dwWidth;
    device->height = frame_info.dwHeight;

#if SHOW_DEBUG_FRAMES
    t2 = NvFBCUtilsGetTimeInMillis();

    LOG_INFO("New %dx%d frame or size %u grabbed in %llu ms", device->width, device->height, 0,
             (unsigned long long)(t2 - t1));
#endif

    // Return with the number of accumulated frames
    return frame_info.dwMissedFrames + 1;
}

void destroy_nvidia_capture_device(NvidiaCaptureDevice* device) {
    NVFBCSTATUS status;

    /*
     * Destroy capture session, tear down resources.
     */
    NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroy_capture_params = {0};
    destroy_capture_params.dwVersion = NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER;

    status =
        device->p_fbc_fn.nvFBCDestroyCaptureSession(device->fbc_handle, &destroy_capture_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return;
    }

    /*
     * Destroy session handle, tear down more resources.
     */
    NVFBC_DESTROY_HANDLE_PARAMS destroy_handle_params = {0};
    destroy_handle_params.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER;

    status = device->p_fbc_fn.nvFBCDestroyHandle(device->fbc_handle, &destroy_handle_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return;
    }
}
