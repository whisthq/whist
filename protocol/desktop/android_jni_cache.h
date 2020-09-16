#include <jni.h>

void cacheFinders(JavaVM* vm);

jclass findClass(const char* name);

JNIEnv* getEnv();