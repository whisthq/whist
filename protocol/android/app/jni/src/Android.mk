LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# prepare libavcodec
include $(CLEAR_VARS)
LOCAL_MODULE    := libavcodec
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libavcodec.so
include $(PREBUILT_SHARED_LIBRARY)

# prepare libavdevice
include $(CLEAR_VARS)
LOCAL_MODULE    := libavdevice
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libavdevice.so
include $(PREBUILT_SHARED_LIBRARY)

# prepare libavformat
include $(CLEAR_VARS)
LOCAL_MODULE    := libavformat
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libavformat.so
include $(PREBUILT_SHARED_LIBRARY)

# prepare libavfilter
include $(CLEAR_VARS)
LOCAL_MODULE    := libavfilter
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libavfilter.so
include $(PREBUILT_SHARED_LIBRARY)

# prepare libavutil
include $(CLEAR_VARS)
LOCAL_MODULE    := libavutil
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libavutil.so
include $(PREBUILT_SHARED_LIBRARY)

# prepare libswscale
include $(CLEAR_VARS)
LOCAL_MODULE    := libswscale
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libswscale.so
include $(PREBUILT_SHARED_LIBRARY)

# prepare libswresample
include $(CLEAR_VARS)
LOCAL_MODULE    := libswresample
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libswresample.so
include $(PREBUILT_SHARED_LIBRARY)

# prepare libssl
include $(CLEAR_VARS)
LOCAL_MODULE    := libssl
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libssl.a
include $(PREBUILT_STATIC_LIBRARY)

# prepare libcrypto
include $(CLEAR_VARS)
LOCAL_MODULE    := libcrypto
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../lib/android/$(TARGET_ARCH_ABI)/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)


LOCAL_MODULE := main

LIBS_PATH := ..

SDL_PATH := SDL2
FFMPEG_PATH := ffmpeg

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(LIBS_PATH)/$(SDL_PATH)/include \
                    $(LOCAL_PATH)/../../../../include/$(FFMPEG_PATH) \
                    $(LOCAL_PATH)/../../../../include \
                    $(LOCAL_PATH)/../../..

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/../../../../desktop/*.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../desktop/**/*.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/audio/audiodecode.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/clipboard/clipboard_synchronizer.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/clipboard/android_clipboard.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/core/fractal.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/cursor/peercursor.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/network/network.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/utils/aes.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/utils/clock.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/utils/json.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/utils/logging.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/utils/png.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/utils/sdlscreeninfo.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/utils/sysinfo.c) \
				   $(wildcard $(LOCAL_PATH)/../../../../fractal/video/videodecode.c) \
# LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../desktop/main.c \
#                   $(LOCAL_PATH)/../../../../desktop/video.c \
#                   $(LOCAL_PATH)/../../../../desktop/sdl_utils.c \
#                   $(LOCAL_PATH)/../../../../desktop/audio.c \
#                   $(LOCAL_PATH)/../../../../desktop/network.c \
# 				  $(LOCAL_PATH)/../../../../desktop/server_message_handler.c \
# 				  $(LOCAL_PATH)/../../../../desktop/desktop_utils.c \
# 				  $(LOCAL_PATH)/../../../../desktop/sdl_event_handler.c \
#                   $(LOCAL_PATH)/../../../../fractal/audio/audiodecode.c \
#                   $(LOCAL_PATH)/../../../../fractal/core/fractal.c \
#                   $(LOCAL_PATH)/../../../../fractal/cursor/peercursor.c \
#                   $(LOCAL_PATH)/../../../../fractal/utils/aes.c\
#                   $(LOCAL_PATH)/../../../../fractal/utils/clock.c\
#                   $(LOCAL_PATH)/../../../../fractal/utils/logging.c \
#                   $(LOCAL_PATH)/../../../../fractal/utils/png.c \
#                   $(LOCAL_PATH)/../../../../fractal/utils/sdlscreeninfo.c \
#                   $(LOCAL_PATH)/../../../../fractal/network/network.c \
#                   $(LOCAL_PATH)/../../../../fractal/clipboard/clipboard_synchronizer.c \
#                   $(LOCAL_PATH)/../../../../fractal/clipboard/android_clipboard.c \
#                   $(LOCAL_PATH)/../../../../fractal/video/videodecode.c


LOCAL_STATIC_LIBRARIES := libssl libcrypto
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_main libavcodec libavdevice libavformat libavfilter libavutil libswscale libswresample

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz -ldl

include $(BUILD_SHARED_LIBRARY)
