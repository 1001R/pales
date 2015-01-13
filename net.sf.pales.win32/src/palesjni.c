#include "palesjni.h"
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include "win32.h"
#else
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include "unix.h"
#define PATHSEP '/'
#endif

static int pales_exec(const wchar_t *execw, const wchar_t *procid, const wchar_t *dbdir, const wchar_t *workdir, const wchar_t *outfile, const wchar_t *errfile, const wchar_t *executable, wchar_t **argv) {
	wchar_t *cmdline, *s;
	size_t cmdlineLen, arglen, argc;
	int rv;
	STARTUPINFO sinfo;
	PROCESS_INFORMATION pinfo;
	bool quoteExecw = false, quoteProcessId = false, quoteDbDir = false, quoteWorkDir = false, quoteExecutable = false, quoteOutFile = false, quoteErrFile = false;
	bool *quoteArgument;
	
	argc = 0;
	while (argv[argc] != NULL) {
		argc++;
	}
	quoteArgument = _alloca(sizeof(bool)* argc);
	cmdlineLen = 0;
	quoteExecw = ArgumentMustBeQuoted(execw, &arglen);
	cmdlineLen += arglen;
	quoteProcessId = ArgumentMustBeQuoted(procid, &arglen);
	cmdlineLen += arglen + 4;
	quoteDbDir = ArgumentMustBeQuoted(dbdir, &arglen);
	cmdlineLen += arglen + 4;
	quoteWorkDir = ArgumentMustBeQuoted(workdir, &arglen);
	cmdlineLen += arglen + 4;
	if (outfile != NULL) {
		quoteOutFile = ArgumentMustBeQuoted(outfile, &arglen);
		cmdlineLen += arglen + 4;
	}
	if (errfile != NULL) {
		quoteErrFile = ArgumentMustBeQuoted(errfile, &arglen);
		cmdlineLen += arglen + 4;
	}
	quoteExecutable = ArgumentMustBeQuoted(executable, &arglen);
	cmdlineLen += arglen + 1;

	for (size_t i = 0; i < argc; i++) {
		quoteArgument[i] = ArgumentMustBeQuoted(argv[i], &arglen);
		cmdlineLen += arglen + 1;
	}
	if ((s = cmdline = malloc(sizeof(wchar_t) * (cmdlineLen + 1))) == NULL) {
		return -1;
	}
	s = quoteExecw ? QuoteArgument(s, execw) : wcpcpy(s, execw);
	s = wcpcpy(s, L" -i ");
	s = quoteProcessId ? QuoteArgument(s, procid) : wcpcpy(s, procid);
	s = wcpcpy(s, L" -d ");
	s = quoteDbDir ? QuoteArgument(s, dbdir) : wcpcpy(s, dbdir);
	s = wcpcpy(s, L" -w ");
	s = quoteWorkDir ? QuoteArgument(s, workdir) : wcpcpy(s, workdir);
	if (outfile != NULL) {
		s = wcpcpy(s, L" -o ");
		s = quoteOutFile ? QuoteArgument(s, outfile) : wcpcpy(s, outfile);
	}
	if (errfile != NULL) {
		s = wcpcpy(s, L" -e ");
		s = quoteErrFile ? QuoteArgument(s, errfile) : wcpcpy(s, errfile);
	}
	*s++ = L' ';
	s = quoteExecutable ? QuoteArgument(s, executable) : wcpcpy(s, executable);
	for (size_t i = 0; i < argc; i++) {
		*s++ = L' ';
		s = quoteArgument[i] ? QuoteArgument(s, argv[i]) : wcpcpy(s, argv[i]);
	}
	
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

JNIEXPORT jlong JNICALL Java_net_sf_pales_ProcessManager_launch(JNIEnv *env, jclass class, jstring execwPath, jstring procid, jstring palesdir,
		jstring workdir, jstring outfile, jstring errfile, jstring executable, jobjectArray argv)
{
	const wchar_t *c_procid = NULL, *c_palesdir = NULL, *c_workdir = NULL, *c_outfile = NULL, *c_errfile = NULL, *c_executable = NULL;
	wchar_t *dbdir = NULL, *s;
#	ifdef WIN32
	const wchar_t *c_execw = NULL;
#	endif
	wchar_t **c_argv = NULL;
	jlong result = -1;
	int n, i, j;

	c_procid = (*env)->GetStringChars(env, procid, NULL);
	c_palesdir = (*env)->GetStringChars(env, palesdir, NULL);
	c_workdir = (*env)->GetStringChars(env, workdir, NULL);
	c_executable = (*env)->GetStringChars(env, executable, NULL);
#	ifdef WIN32
	c_execw = (*env)->GetStringChars(env, execwPath, NULL);
#	endif
	if (outfile != NULL) {
		c_outfile = (*env)->GetStringChars(env, outfile, NULL);
	}
	if (errfile != NULL) {
		c_errfile = (*env)->GetStringChars(env, errfile, NULL);
	}
	if ((dbdir = malloc(sizeof(wchar_t) * (wcslen(c_palesdir) + 4))) == NULL) {
		goto cleanup;
	}
	i = j = 0;
	n = (*env)->GetArrayLength(env, argv) + 1;
#	ifndef WIN32
	n++;
#	endif
	if ((c_argv = calloc(n, sizeof(void*))) == NULL) {
		goto cleanup;
	}
#	ifndef WIN32
	c_argv[0] = strdup(c_executable);
	i++;
#	endif
	for ( ; i < n - 1; i++) {
		jstring str = (*env)->GetObjectArrayElement(env, argv, j++);
		const wchar_t *s;
		if (str == NULL) {
			goto cleanup;
		}
		s = (*env)->GetStringChars(env, str, NULL);
		if (s == NULL) {
			goto cleanup;
		}
		c_argv[i] = _wcsdup(s);
		(*env)->ReleaseStringChars(env, str, s);
		if (c_argv[i] == NULL) {
			goto cleanup;
		}
	}
	s = wcpcpy(dbdir, c_palesdir);
	*s++ = PATHSEP;
	s = wcpcpy(s, L"db");

#	ifdef WIN32
	result = pales_exec(c_execw, c_procid, dbdir, c_workdir, c_outfile, c_errfile, c_executable, c_argv);
#	else
	result = process_run(c_procid, dbdir, c_workdir, c_outfile, c_errfile, c_executable, c_argv);
#	endif
cleanup:
	if (c_argv != NULL) {
		for (wchar_t **arg = c_argv; *arg != NULL; arg++) {
			free(*arg);
		}
		free(c_argv);
	}
	if (dbdir != NULL) {
		free(dbdir);
	}
	if (c_errfile != NULL) {
		(*env)->ReleaseStringChars(env, errfile, c_errfile);
	}
	if (c_outfile != NULL) {
		(*env)->ReleaseStringChars(env, outfile, c_outfile);
	}
	if (c_executable != NULL) {
		(*env)->ReleaseStringChars(env, executable, c_executable);
	}
	if (c_workdir != NULL) {
		(*env)->ReleaseStringChars(env, workdir, c_workdir);
	}
	if (c_palesdir != NULL) {
		(*env)->ReleaseStringChars(env, palesdir, c_palesdir);
	}
	if (c_procid != NULL) {
		(*env)->ReleaseStringChars(env, procid, c_procid);
	}
	if (c_execw != NULL) {
		(*env)->ReleaseStringChars(env, execwPath, c_execw);
	}

	if (result == -1) {
		jclass rte = (*env)->FindClass(env, "java/lang/RuntimeException");
		if (rte == NULL) {
			(*env)->FatalError(env, "Can't find class java.lang.RuntimeException");
		}
		(*env)->ThrowNew(env, rte, "Can't launch process");
	}
	return result;
}

JNIEXPORT void JNICALL Java_net_sf_pales_OS_cancelProcess(JNIEnv *env, jclass class, jstring process_id)
{
	bool success = false;
#	ifdef WIN32
	HANDLE event;
	const wchar_t *ename = (*env)->GetStringChars(env, process_id, NULL);
	
	event = CreateEvent(NULL, FALSE, FALSE, ename);
	(*env)->ReleaseStringChars(env, process_id, ename);
	success = event != NULL && SetEvent(event);
#	else
	const char *semaphoreName = (*env)->GetStringUTFChars(env, palesProcessId, NULL);
	sem_t *semaphore = sem_open(semaphoreName, 0);
	if (semaphore != SEM_FAILED) {
		success = sem_post(semaphore) == 0;
		sem_close(semaphore);
	}
	(*env)->ReleaseStringUTFChars(env, palesProcessId, semaphoreName);
#	endif
	if (!success) {
		jclass rte = (*env)->FindClass(env, "java/lang/RuntimeException");
        if (rte == NULL) {
			(*env)->FatalError(env, "Can't find class java.lang.RuntimeException");
		}
		(*env)->ThrowNew(env, rte, "Can't cancel process");
	}
}

JNIEXPORT jboolean JNICALL Java_net_sf_pales_OS_isProcessRunning(JNIEnv *env, jclass javaClass, jstring palesProcessId)
{
	bool isRunning = false;
#	ifdef WIN32
	const wchar_t *eventName;
	HANDLE eventHandle;
#   else
	const char *semaphoreName;
	sem_t *semaphore = NULL;
#   endif
	
#	ifdef WIN32
	eventName = (*env)->GetStringChars(env, palesProcessId, NULL);
	eventHandle = OpenEvent(EVENT_MODIFY_STATE, FALSE, eventName);
	if (eventHandle != NULL) {
		isRunning = true;
		CloseHandle(eventHandle);
	}
	(*env)->ReleaseStringChars(env, palesProcessId, eventName);
#	else
	semaphoreName = (*env)->GetStringUTFChars(env, palesProcessId, NULL);
	semaphore = sem_open(semaphoreName, 0);
	if (semaphore != SEM_FAILED) {
		isRunning = true;
		sem_close(semaphore);
	}
	(*env)->ReleaseStringUTFChars(env, palesProcessId, semaphoreName);
#	endif
	return isRunning;
}
