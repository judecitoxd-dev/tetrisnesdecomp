#include "android_bridge.h"

#ifdef __ANDROID__
#include <SDL_system.h>
#include <jni.h>

bool tetris_android_request_rom(void) {
    JNIEnv *environment = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass activity_class;
    jmethodID method;
    if (!environment || !activity) return false;
    activity_class = (*environment)->GetObjectClass(environment, activity);
    if (!activity_class) {
        (*environment)->DeleteLocalRef(environment, activity);
        return false;
    }
    method = (*environment)->GetMethodID(environment, activity_class,
                                         "requestRomSelection", "()V");
    if (!method) {
        (*environment)->DeleteLocalRef(environment, activity_class);
        (*environment)->DeleteLocalRef(environment, activity);
        return false;
    }
    (*environment)->CallVoidMethod(environment, activity, method);
    if ((*environment)->ExceptionCheck(environment)) {
        (*environment)->ExceptionClear(environment);
        (*environment)->DeleteLocalRef(environment, activity_class);
        (*environment)->DeleteLocalRef(environment, activity);
        return false;
    }
    (*environment)->DeleteLocalRef(environment, activity_class);
    (*environment)->DeleteLocalRef(environment, activity);
    return true;
}
#else
bool tetris_android_request_rom(void) {
    return false;
}
#endif
