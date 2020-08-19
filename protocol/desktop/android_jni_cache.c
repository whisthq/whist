#include "android_jni_cache.h"

#import <android/log.h>

JavaVM* cachedJvm = NULL;
static jobject cachedClassLoader;
static jmethodID cachedFindClassMethod;

// cache Java vm to allow Fractal class access from other threads
void cacheFinders(JavaVM *vm) {
    cachedJvm = vm;
    JNIEnv *env = getEnv();
    jclass fractalClass = (*env)->FindClass(env, "org/fractal/app/Fractal");
    jclass fractalObjectClass = (*env)->GetObjectClass(env, fractalClass);
    jclass classLoaderClass = (*env)->FindClass(env, "java/lang/ClassLoader");
    jmethodID getClassLoaderMethod = (*env)->GetMethodID(env, fractalObjectClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    cachedClassLoader = (jobject)(*env)->NewGlobalRef(env, (*env)->CallObjectMethod(env, fractalClass, getClassLoaderMethod));
    cachedFindClassMethod = (*env)->GetMethodID(env, classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
}

void uncacheFinders() {
    JNIEnv *env = getEnv();
    (*env)->DeleteGlobalRef(env, cachedClassLoader);
}

jclass findClass(const char* name) {
    JNIEnv* env = getEnv();
    return (jclass)((*env)->CallObjectMethod(env, cachedClassLoader, cachedFindClassMethod, (*env)->NewStringUTF(env, name)));
}

JNIEnv* getEnv() {
    JNIEnv *env;
    int status = (*cachedJvm)->GetEnv(cachedJvm, (void**)&env, JNI_VERSION_1_6);
    if(status < 0) {
        status = (*cachedJvm)->AttachCurrentThread(cachedJvm, &env, NULL);
        if(status < 0) {
            return NULL;
        }
    }

    return env;
}