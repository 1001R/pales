#include <jni.h>

#ifndef _Included_net_sf_pales_OS
#define _Included_net_sf_pales_OS
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_net_sf_pales_OS_cancelProcess(JNIEnv *, jclass, jstring);
JNIEXPORT jstring JNICALL Java_net_sf_pales_OS_execute(JNIEnv *env, jclass, jobjectArray);

#ifdef __cplusplus
}
#endif
#endif
