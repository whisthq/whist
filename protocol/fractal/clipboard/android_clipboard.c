/*
 * Clipboard for android, empty for now
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "clipboard.h"
#include "../utils/logging.h"
#include <stddef.h>
#include <jni.h>
#include <memory.h>

extern JavaVM* javaVM;

#define MAX_CLIPBOARD_SIZE 10000000

static char cb_buf[MAX_CLIPBOARD_SIZE];

void initClipboard() {
  JNIEnv *env;
  (*javaVM)->GetEnv(javaVM, (void**)&env, JNI_VERSION_1_6);
  jclass clazz = (*env)->FindClass(env, "org/fractal/app/Fractal");
  jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "initClipboard", "()V");
  (*env)->CallStaticVoidMethod(env, clazz, mid);
}

bool hasClipboardUpdated() {
  JNIEnv *env;
  (*javaVM)->GetEnv(javaVM, (void**)&env, JNI_VERSION_1_6);
  jclass clazz = (*env)->FindClass(env, "org/fractal/app/Fractal");
  jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "hasClipboardUpdated", "()Z");
  jboolean b = (*env)->CallStaticBooleanMethod(env, clazz, mid);
  return b;
}

ClipboardData* GetClipboard() {
  JNIEnv *env;
  (*javaVM)->GetEnv(javaVM, (void**)&env, JNI_VERSION_1_6);
  jclass clazz = (*env)->FindClass(env, "org/fractal/app/Fractal");

  // Get size
  jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "GetClipboardSize", "()I");
  int size = (*env)->CallStaticIntMethod(env, clazz, mid);

  // Get Content
  mid = (*env)->GetStaticMethodID(env, clazz, "GetClipboard", "()Ljava/lang/String;");
  jstring clip = (*env)->CallStaticObjectMethod(env, clazz, mid);

  // Get Type
  mid = (*env)->GetStaticMethodID(env, clazz, "GetClipboardType", "()I");
  int type = (*env)->CallStaticIntMethod(env, clazz, mid);

  ClipboardData* cb = (ClipboardData*)cb_buf;
  cb->type = (ClipboardType) type;
  cb->size = size;

    const char *java_string = (*env)->GetStringUTFChars(env, clip, 0);
    memcpy(cb->data, java_string, (size_t) size + 1);
    (*env)->ReleaseStringUTFChars(env, clip, java_string);
    LOG_ERROR("Type : %d", type);
    LOG_ERROR("Size : %d", size);
    LOG_ERROR("Buff : %s", cb->data);
  return cb;
}

void SetClipboard(ClipboardData* cb) {
  JNIEnv *env;
  (*javaVM)->GetEnv(javaVM, (void**)&env, JNI_VERSION_1_6);
  jclass clazz = (*env)->FindClass(env, "org/fractal/app/Fractal");
  jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "SetClipboard", "(IILjava/lang/String;)V");
  (*env)->CallStaticVoidMethod(env, clazz, mid, cb->size, (int)cb->type, (jstring) cb->data);
}
