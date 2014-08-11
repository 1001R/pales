#include <jni.h>

#ifndef _Included_net_sf_pales_OS
#define _Included_net_sf_pales_OS
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_net_sf_pales_OS_cancelProcess(JNIEnv *, jclass, jstring, jlong);

JNIEXPORT jboolean JNICALL Java_net_sf_pales_OS_isProcessRunning(JNIEnv *env, jclass javaClass, jstring processId);

JNIEXPORT jlong JNICALL Java_net_sf_pales_ProcessManager_launch(JNIEnv *, jclass, jstring, jstring, jstring, jstring, jstring, jstring, jstring, jobjectArray);

#ifdef __cplusplus
}
#endif
#endif
