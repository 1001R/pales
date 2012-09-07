#include "palesjni.h"
#include <windows.h>


JNIEXPORT void JNICALL Java_net_sf_pales_OS_cancelProcess(JNIEnv *env, jclass class, jstring process_id)
{
	HANDLE event;
	const char *errmsg = NULL;

	const char *ename = (*env)->GetStringUTFChars(env, process_id, NULL);
	event = CreateEvent(NULL, FALSE, FALSE, ename);
	(*env)->ReleaseStringUTFChars(env, process_id, ename);
	if (event == INVALID_HANDLE_VALUE) {
		errmsg = "Cannot create event object";
	}
	else if (!SetEvent(event)) {
		errmsg = "Cannot signal the event";
	}
	if (errmsg != NULL) {
		jclass rte = (*env)->FindClass(env, "java/lang/RuntimeException");
        if (rte == NULL) {
			return;
		}
		(*env)->ThrowNew(env, rte, "Cannot cancel process");
	}
}


JNIEXPORT jstring JNICALL Java_net_sf_pales_OS_execute(JNIEnv *env, jclass clazz, jobjectArray cmdarray)
{
	STARTUPINFO sinfo;
	PROCESS_INFORMATION pinfo;
	int rv;
	int len, i, cmdlen;
	char *cmdline;
	char *s;
	jstring retval = NULL;

	cmdlen = 0;
	len = (*env)->GetArrayLength(env, cmdarray);
	for (i = 0; i < len; i++) {
		jstring s = (*env)->GetObjectArrayElement(env, cmdarray, i);
		cmdlen += (*env)->GetStringUTFLength(env, s) + 2;
	}
	cmdlen += len;
	cmdline = (char*) malloc(cmdlen);
	s = cmdline;
	for (i = 0; i < len; i++) {
		jstring element = (*env)->GetObjectArrayElement(env, cmdarray, i);
		const char *tmp = (*env)->GetStringUTFChars(env, element, NULL);
		if (i > 0) {
			*s++ = ' ';
		}
		*s++ = '"';
		strcpy(s, tmp);
		s += strlen(tmp);
		*s++ = '"';
		(*env)->ReleaseStringUTFChars(env, element, tmp);
	}
	*s = '\0';
	
	ZeroMemory(&sinfo, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);
    ZeroMemory(&pinfo, sizeof(pinfo));

    // Start the child process. 
    rv = CreateProcess( NULL,   // No module name (use command line)
        cmdline,          // Command line
        NULL,             // Process handle not inheritable
        NULL,             // Thread handle not inheritable
        FALSE,            // Set handle inheritance to FALSE
        CREATE_NO_WINDOW, // No creation flags
        NULL,             // Use parent's environment block
        NULL,             // Use parent's starting directory 
        &sinfo,           // Pointer to STARTUPINFO structure
        &pinfo );         // Pointer to PROCESS_INFORMATION structure
	if (rv != 0) {
		retval = (*env)->NewStringUTF(env, cmdline);
		CloseHandle(pinfo.hProcess);
 		CloseHandle(pinfo.hThread);
	}
	free(cmdline);
 	return retval;
}
