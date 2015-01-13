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

JNIEXPORT jlong JNICALL Java_net_sf_pales_ProcessManager_launch(JNIEnv *env, jclass javaClass, jstring execwPath, jstring procid, jstring palesdir,
		jstring workdir, jstring outfile, jstring errfile, jstring executable, jobjectArray argv)
{
	const char *c_procid, *c_palesdir, *c_workdir, *c_outfile = NULL, *c_errfile = NULL, *c_executable;
	char *dbdir = NULL, *s;
	char **c_argv = NULL;
	jlong result = -1;
	int n, i, j;

	c_procid = env->GetStringUTFChars(procid, NULL);
	c_palesdir = env->GetStringUTFChars(palesdir, NULL);
	c_workdir = env->GetStringUTFChars(workdir, NULL);
	c_executable = env->GetStringUTFChars(executable, NULL);
	if (outfile != NULL) {
		c_outfile = env->GetStringUTFChars(outfile, NULL);
	}
	if (errfile != NULL) {
		c_errfile = env->GetStringUTFChars(errfile, NULL);
	}
	if ((dbdir = (char*) malloc(strlen(c_palesdir) + 4)) == NULL) {
		goto cleanup;
	}
	i = j = 0;
	n = env->GetArrayLength(argv) + 1;
#	ifndef WIN32
	n++;
#	endif
	if ((c_argv = (char**) calloc(n, sizeof(void*))) == NULL) {
		goto cleanup;
	}
#	ifndef WIN32
	c_argv[0] = strdup(c_executable);
	i++;
#	endif
	for ( ; i < n - 1; i++) {
		jstring str = (jstring) env->GetObjectArrayElement(argv, j++);
		if (str == NULL) {
			goto cleanup;
		}
		const char *s = env->GetStringUTFChars(str, NULL);
		c_argv[i] = strdup(s);
		env->ReleaseStringUTFChars(str, s);
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
		env->ReleaseStringUTFChars(errfile, c_errfile);
	}
	if (c_outfile != NULL) {
		env->ReleaseStringUTFChars(outfile, c_outfile);
	}
	env->ReleaseStringUTFChars(executable, c_executable);
	env->ReleaseStringUTFChars(workdir, c_workdir);
	env->ReleaseStringUTFChars(palesdir, c_palesdir);
	env->ReleaseStringUTFChars(procid, c_procid);

	if (result == -1) {
		jclass rte = env->FindClass("java/lang/RuntimeException");
		if (rte == NULL) {
			env->FatalError("Can't find class java.lang.RuntimeException");
		}
		env->ThrowNew(rte, "Can't launch process");
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

JNIEXPORT void JNICALL Java_net_sf_pales_OS_cancelProcess(JNIEnv *env, jclass javaClass, jstring process_id)
{
    int r;
    const char *errmsg = NULL;
    const char *procid;
    jclass rte;

    procid = env->GetStringUTFChars(process_id, NULL);
    sem_t *sem = open_cancel_semaphore(procid);
    env->ReleaseStringUTFChars(process_id, procid);
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
    rte = env->FindClass("java/lang/RuntimeException");
    if (rte == NULL) {
	env->FatalError("Can't find class java.lang.RuntimeException");
    }
    env->ThrowNew(rte, errmsg);
}

JNIEXPORT jboolean JNICALL Java_net_sf_pales_OS_isProcessRunning(JNIEnv *env, jclass javaClass, jstring process_id) {
    jboolean retval = 0;
    const char *procid = env->GetStringUTFChars(process_id, NULL);
    sem_t *sem = open_cancel_semaphore(procid);
    env->ReleaseStringUTFChars(process_id, procid);
    if (sem != SEM_FAILED) {
	retval = 1;
	sem_close(sem);
    }
    return retval;
}
