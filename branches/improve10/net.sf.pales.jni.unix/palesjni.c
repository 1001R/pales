#include "palesjni.h"
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include "unix.h"
#define PATHSEP '/'


#include "unix.h"

JNIEXPORT jlong JNICALL Java_net_sf_pales_ProcessManager_launch(JNIEnv *env, jclass class, jstring execwPath, jstring procid, jstring palesdir,
		jstring workdir, jstring outfile, jstring errfile, jstring executable, jobjectArray argv)
{
	const char *c_procid, *c_palesdir, *c_workdir, *c_outfile = NULL, *c_errfile = NULL, *c_executable;
	char *dbdir = NULL, *s;
	char **c_argv = NULL;
	jlong result = -1;
	int n, i, j;

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
	if ((dbdir = malloc(strlen(c_palesdir) + 4)) == NULL) {
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
		if (str == NULL) {
			goto cleanup;
		}
		const char *s = (*env)->GetStringUTFChars(env, str, NULL);
		c_argv[i] = strdup(s);
		(*env)->ReleaseStringUTFChars(env, str, s);
		if (c_argv[i] == NULL) {
			goto cleanup;
		}
	}
	s = stpcpy(dbdir, c_palesdir);
	*s++ = PATHSEP;
	s = stpcpy(s, "db");

	result = process_run(c_procid, dbdir, c_workdir, c_outfile, c_errfile, c_executable, c_argv);
cleanup:
	if (c_argv != NULL) {
		for (char **arg = c_argv; *arg != NULL; arg++) {
			free(*arg);
		}
		free(c_argv);
	}
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

static sem_t *open_cancel_semaphore(const char *procid) {
    char *semname, *s;
    int len;
    sem_t *sem;

    len = strlen(procid) + 8;
    if ((s = semname = (char*) malloc(len)) == NULL) {
	return SEM_FAILED;
    }
    s = stpcpy(s, "/PALES:");
    s = stpcpy(s, procid);
    sem = sem_open(semname, 0);
    free(semname);
    return sem;
}

JNIEXPORT void JNICALL Java_net_sf_pales_OS_cancelProcess(JNIEnv *env, jclass class, jstring process_id)
{
    int r;
    const char *errmsg = NULL;
    const char *procid;
    jclass rte;

    procid = (*env)->GetStringUTFChars(env, process_id, NULL);
    sem_t *sem = open_cancel_semaphore(procid);
    (*env)->ReleaseStringUTFChars(env, process_id, procid);
    if (sem == SEM_FAILED) {
	return;
    }
    r = sem_post(sem);
    sem_close(sem);
    if (r != 0) {
	errmsg = "Failed to unlock cancellation semaphore";
	goto fail;
    }
    return;
fail:
    rte = (*env)->FindClass(env, "java/lang/RuntimeException");
    if (rte == NULL) {
	(*env)->FatalError(env, "Can't find class java.lang.RuntimeException");
    }
    (*env)->ThrowNew(env, rte, errmsg);
}

JNIEXPORT jboolean JNICALL Java_net_sf_pales_OS_isProcessRunning(JNIEnv *env, jclass class, jstring process_id) {
    jboolean retval = 0;
    const char *procid = (*env)->GetStringUTFChars(env, process_id, NULL);
    sem_t *sem = open_cancel_semaphore(procid);
    (*env)->ReleaseStringUTFChars(env, process_id, procid);
    if (sem != SEM_FAILED) {
	retval = 1;
	sem_close(sem);
    }
    return retval;
}
