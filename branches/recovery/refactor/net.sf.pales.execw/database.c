#include "database.h"
#include "process.h"
#include "error.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <assert.h>

#define SEPARATOR L'-'
#define PATHSEP L'\\'


int db_open(database_t *db, const wchar_t *path, error_t *err)
{
	DWORD attr;

	FILE *f = _wfopen(L"c:\\temp\\log.txt", L"w+, ccs=UTF-8");
	fwprintf(f, L"%s\n", path);
	fclose(f);

	memset(db, 0, sizeof(*db));
	assert (path != NULL);
	attr = GetFileAttributes(path);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		error_set_message_fmt(err, L"Database directory does not exist: %s", path);
		return -1;
	}
 	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
 		error_set_message_fmt(err, L"Invalid database directory: %s", path);
		return -1;
	}
	db->dbdir = path;
	return 0;
}

int db_close(database_t *db)
{
	return 0;
}

static int process_encode(const database_t *db, const process_t *proc, const wchar_t *suffix, wchar_t *path)
{
	wchar_t pid[32];
	wchar_t *s;
	size_t pathlen, dirlen, idlen, pidlen = 0, sfxlen = 0;
	bool addsep = false;

	if (proc->status == running) {
		snwprintf(pid, sizeof(pid) / sizeof(wchar_t), L"%ld", proc->pid);
		pidlen = wcslen(pid);
	}
	
	pathlen = dirlen = wcslen(db->dbdir);
	if (db->dbdir[dirlen - 1] != L'\\') {
		addsep = true;
		pathlen++;
	}

	idlen = wcslen(proc->id);
	pathlen += idlen;

	if (suffix != NULL && suffix[0] != L'\0') {
		sfxlen = wcslen(suffix);
		pathlen += sfxlen;
	}

	if (proc->status == running) {
		pathlen += pidlen + 1;
	}
	pathlen += 3;

	if (path == NULL) {
		return pathlen;
	}

	s = path;

	wcsncpy(s, db->dbdir, dirlen);
	s += dirlen;

	if (addsep) {
		*s++ = PATHSEP;
	}

	wcsncpy(s, proc->id, idlen);
	s += idlen;

	*s++ = SEPARATOR;

	switch (proc->status) {
	case running:
		*s++ = L'R';
		*s++ = SEPARATOR;
		wcsncpy(s, pid, pidlen);
		s += pidlen;
		break;
	case finished:
		*s++ = L'F';
		break;
	case cancelled:
		*s++ = L'C';
		break;
	case error:
		*s++ = L'E';
		break;
	}
	if (suffix != NULL && suffix[0] != L'\0') {
		wcsncpy(s, suffix, sfxlen);
		s += sfxlen;
	}
	*s = L'\0';
	return pathlen;
}


void db_error(const database_t *db, const process_t *proc, const error_t *error) {
	size_t pathlen;
	wchar_t *path;
	FILE *f;

	pathlen = process_encode(db, proc, L".msg", NULL);
	path = alloca(pathlen);
	process_encode(db, proc, L".msg", path);

	f = _wfopen(path, L"w, ccs=UTF-8");
	if (f != NULL) {
		fwprintf(f, L"Message: %s\n", error->message);
		if (error->type == ERR_APICALL) {
			fwprintf(f, L"API function: %s\n", error->apicall);
			fwprintf(f, L"Error code: %d\n", error->errnum);
		}
		fclose(f);
	}

	path[pathlen - 4] = L'\0';
	f = CreateFile(path, 0, FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f != INVALID_HANDLE_VALUE) {
		CloseHandle(f);
	}
}


/*
static process_t *process_decode(database_t *db, const char *path)
{
	char *s;
	process_t *proc;

	if (strncmp(db->path, path, strlen(db->path)) == 0) {
		path += strlen(db->path);
	}
	if (*path == PATHSEP) {
		path++;
	}
	s = strchr(path, SEPARATOR);
	if (s == NULL) {
		return NULL;
	}

	if (process_new(path, s - path, &proc) != 0) {
		return NULL;
	}
	s++;
	switch (*s) {
	case 'R':
		proc->status = running;
		break;
	case 'F':
		break;
	case 'C':
		break;
	}
	return proc;
}

static char *path_join(const char *directory, const char *filename)
{
	char *s;
	char *result;
	int len;

	len = strlen(directory);
	while (len >=0 && directory[len - 1] == PATHSEP) {
		len--;
	}
	result = s = (char *)malloc(len + 1 + strlen(filename) + 1);
	if (result != NULL) {
		strncpy(s, directory, len);
		s += len;
		*s++ = PATHSEP;
		strcpy(s, filename);
		s += strlen(filename);
		*s++ = '\0';
	}
	return result;
}
*/

static bool create_empty_file(const wchar_t *path, error_t *err)
{
	HANDLE h;
	int success = true;
	/*
	wchar_t *path, *s;
	size_t len, dirlen, fnlen;
	bool addpathsep = false;
	wchar_t *errbuf;

	len = dirlen = wcslen(directory);
	if (directory[dirlen - 1] != PATHSEP) {
		addpathsep = true;
		len++;
	}
	fnlen = wcslen(filename);
	len += fnlen;
	if ((path = (wchar_t *) malloc((len + 1) * sizeof(wchar_t))) == NULL) {
		if (errmsg != NULL) {
			*errmsg = ERRMSG_OOM;
		}
		return -1;
	}
	s = path;
	wcsncpy(s, directory, dirlen);
	s += dirlen;
	if (addpathsep) {
		*s++ = PATHSEP;
	}
	wcsncpy(s, filename, fnlen);
	s[fnlen] = L'\0';
	*/
	h = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
//	free(path);
	if (h == INVALID_HANDLE_VALUE) {
		error_apicall_fail(err, L"CreateFile");
		success = false;
	}
	else {
		CloseHandle(h);
	}
	return success;
}

bool db_update(database_t *db, process_t *proc, error_t *err)
{
	size_t pathlen;
	wchar_t *path;

	pathlen = process_encode(db, proc, NULL, NULL);
	path = alloca(pathlen * sizeof(wchar_t));
	process_encode(db, proc, NULL, path);

	return create_empty_file(path, err);
}


