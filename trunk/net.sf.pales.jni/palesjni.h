#include <jni.h>

#ifndef _Included_net_sf_pales_OS
#define _Included_net_sf_pales_OS
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL _Java_net_sf_pales_OS_cancelProcess(JNIEnv *, jclass, jstring, jlong);
JNIEXPORT jlong JNICALL _Java_net_sf_pales_ProcessManager_launch(JNIEnv *, jclass, jstring, jstring, jstring, jstring, jstring, jstring, jobjectArray);

#ifdef __cplusplus
}
#endif
#endif