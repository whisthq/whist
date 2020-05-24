
# Uncomment this if you're using STL in your project
# You can find more information here:
# https://developer.android.com/ndk/guides/cpp-support
APP_STL := c++_shared

APP_CFLAGS += -Ofast
#APP_ABI := armeabi armeabi-v7a x86
APP_ABI := x86_64
#armeabi-v7a arm64-v8a x86 x86_64

# Min runtime API level
APP_PLATFORM=android-16
APP_CPPFLAGS += -fexceptions
