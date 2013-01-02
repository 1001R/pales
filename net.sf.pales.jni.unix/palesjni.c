#include "palesjni.h"
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>

#define PATHSEP '\\'

static char *stpcpy(char *restrict to, const char *restrict from)
{
	for (; (*to = *from); ++from, ++to);
	return(to);
}

#else
#include <signal.h>
#include "unix.h"
#define PATHSEP '/'

#endif


#ifdef WIN32
static char *stpcpy_quote(char *to, const char *from) {
	bool quote = false;
	for (const char *s = from; *s; s++) {
		if (isspace(*s)) {
			quote = true;
			break;
		}
	}
	if (quote) {
		*to++ = '"';
	}
	to = stpcpy(to, from);
	if (quote) {
		*to++ = '"';
	}
	return to;
}

static int pales_exec(const char *execw, const char *procid, const char *dbdir, const char *workdir, const char *outfile, const char *errfile, const char *executable, char **argv) {
	char *cmdline, *s;
	int len, rv;
	STARTUPINFO sinfo;
	PROCESS_INFORMATION pinfo;

	len = 21 + strlen(execw) + strlen(procid) + strlen(dbdir) + strlen(workdir) + strlen(executable);
	if (outfile != NULL) {
		len += strlen(outfile) + 6;
	}
	if (errfile != NULL) {
		len += strlen(errfile) + 6;
	}
	for (char **arg = argv; *arg != NULL; arg++) {
		len += strlen(*arg) + 3;
	}
	if ((s = cmdline = malloc(len + 1)) == NULL) {
		return -1;
	}
	s = stpcpy_quote(s, execw);
	s = stpcpy(s, " -i ");
	s = stpcpy(s, procid);
	s = stpcpy(s, " -d ");
	s = stpcpy_quote(s, dbdir);
	s = stpcpy(s, " -w ");
	s = stpcpy_quote(s, workdir);
	if (outfile != NULL) {
		s = stpcpy(s, " -o ");
		s = stpcpy_quote(s, outfile);
	}
	if (errfile != NULL) {
		s = stpcpy(s, " -e ");
		s = stpcpy_quote(s, errfile);
	}
	*s++ = ' ';
	s = stpcpy_quote(s, executable);
	for (char **arg = argv; *arg != NULL; arg++) {
		*s++ = ' ';
		s = stpcpy_quote(s, *arg);
	}
	*s = '\0';
	memset(&sinfo, 0, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);
    memset(&pinfo, 0, sizeof(pinfo));
    rv = CreateProcess(NULL,   // No module name (use command line)
        cmdline,          // Command line
        NULL,             // Process handle not inheritable
        NULL,             // Thread handle not inheritable
        FALSE,            // Set handle inheritance to FALSE
        CREATE_NO_WINDOW, // No creation flags
        NULL,             // Use parent's environment block
        NULL,             // Use parent's starting directory
        &sinfo,           // Pointer to STARTUPINFO structure
        &pinfo);         // Pointer to PROCESS_INFORMATION structure
	if (rv != 0) {
		CloseHandle(pinfo.hProcess);
 		CloseHandle(pinfo.hThread);
	}
	free(cmdline);
	return rv ? 0 : -1;
}
#else

#include "unix.h"

#endif

JNIEXPORT jlong JNICALL Java_net_sf_pales_ProcessManager_launch(JNIEnv *env, jclass class, jstring procid, jstring palesdir,
		jstring workdir, jstring outfile, jstring errfile, jstring executable, jobjectArray argv)
{
	const char *c_procid, *c_palesdir, *c_workdir, *c_outfile = NULL, *c_errfile = NULL, *c_executable;
	char *dbdir = NULL, *s;
#	ifdef WIN32
	char *execw = NULL;
#	endif
	char **c_argv = NULL;
	jlong result = -1;
	int n, i;

	c_procid = (*env)->GetStringUTFChars(env, procid, NULL);
	c_palesdir = (*env)->GetStringUTFChars(env, palesdir, NULL);
	c_workdir = (*env)->GetStringUTFChars(env, workdir, NULL);
	c_executable = (*env)->GetStringUTFChars(env, executable, NULL);
	if (outfile != NULL) {
		c_outfile = (*env)->GetStringUTFChars(env, outfile, NULL);
	}
	if (errfile != NULL) {
		c_errfile = (*env)->GetStringUTFChars(env, errfile, NULL);
	}
#	ifdef WIN32
	if ((execw = malloc(strlen(c_palesdir) + 15)) == NULL) {
		goto cleanup;
	}
#	endif
	if ((dbdir = malloc(strlen(c_palesdir) + 4)) == NULL) {
		goto cleanup;
	}
	i = 0;
	n = (*env)->GetArrayLength(env, argv) + 1;
#	ifndef WIN32
	n++;
#	endif
	if ((c_argv = calloc(n, sizeof(void*))) == NULL) {
		goto cleanup;
	}
#	ifndef WIN32
	s = strrchr(c_executable, PATHSEP);
	if (s == NULL) {
		c_argv[0] = strdup(c_executable);
	}
	else {
		c_argv[0] = strdup(s + 1);
	}
	i++;
#	endif
	for ( ; i < n; i++) {
		jstring str = (*env)->GetObjectArrayElement(env, argv, i);
		if (str == NULL) {
			goto cleanup;
		}
		const char *s = (*env)->GetStringUTFChars(env, str, NULL);
		if ((c_argv[i] = strdup(s)) == NULL) {
			goto cleanup;
		}
	}
#	ifdef WIN32
	s = stpcpy(execw, c_palesdir);
	s = stpcpy(s, "\\bin\\execw.exe");
#	endif
	s = stpcpy(dbdir, c_palesdir);
	*s++ = PATHSEP;
	s = stpcpy(s, "db");

#	ifdef WIN32
	result = pales_exec(execw, c_procid, dbdir, c_workdir, c_outfile, c_errfile, c_executable, c_argv);
#	else
	result = process_run(c_procid, dbdir, c_workdir, c_outfile, c_errfile, c_executable, c_argv);
#	endif
cleanup:
	if (c_argv != NULL) {
		for (char **arg = c_argv; *arg != NULL; arg++) {
			free(*arg);
		}
		free(c_argv);
	}
#	ifdef WIN32
	if (execw != NULL) {
		free(execw);
	}
#	endif
	if (dbdir != NULL) {
		free(dbdir);
	}
	if (c_errfile != NULL) {
		(*env)->ReleaseStringUTFChars(env, errfile, c_errfile);
	}
	if (c_outfile != NULL) {
		(*env)->ReleaseStringUTFChars(env, outfile, c_outfile);
	}
	(*env)->ReleaseStringUTFChars(env, executable, c_executable);
	(*env)->ReleaseStringUTFChars(env, workdir, c_workdir);
	(*env)->ReleaseStringUTFChars(env, palesdir, c_palesdir);
	(*env)->ReleaseStringUTFChars(env, procid, c_procid);

	if (result == -1) {
		jclass rte = (*env)->FindClass(env, "java/lang/RuntimeException");
		if (rte == NULL) {
			(*env)->FatalError(env, "Can't find class java.lang.RuntimeException");
		}
		(*env)->ThrowNew(env, rte, "Can't launch process");
	}
	return result;
}

JNIEXPORT void JNICALL Java_net_sf_pales_OS_cancelProcess(JNIEnv *env, jclass class, jstring process_id, jlong pid)
{
	bool success = false;
#	ifdef WIN32
	HANDLE event;

	const char *ename = (*env)->GetStringUTFChars(env, process_id, NULL);
	event = CreateEvent(NULL, FALSE, FALSE, ename);
	(*env)->ReleaseStringUTFChars(env, process_id, ename);
	if (event == INVALID_HANDLE_VALUE) {
		errmsg = "Cannot create event object";
	}
	else {
		success = SetEvent(event);
	}
#	else
	success = kill(pid, SIGTERM) == 0;
#	endif
	if (!success) {
		jclass rte = (*env)->FindClass(env, "java/lang/RuntimeException");
        if (rte == NULL) {
			(*env)->FatalError(env, "Can't find class java.lang.RuntimeException");
		}
		(*env)->ThrowNew(env, rte, "Can't cancel process");
	}
}
