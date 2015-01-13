#ifndef PROCESS_H
#define PROCESS_H

#include <windows.h>
#include <stdint.h>

enum procstat {
	running,
	finished,
	cancelled,
	error
};

typedef enum procstat procstat_t;

typedef struct process {
	const wchar_t *id;
	HANDLE handle;
	HANDLE job;
	DWORD pid;
	procstat_t status;
	DWORD exitcode;
} process_t;



typedef struct launch_request {
	const wchar_t *dbpath;
	const wchar_t *workdir;
	const wchar_t *errpath;
	const wchar_t *outpath;
	const wchar_t *id;
} launch_request_t;


int launch_request_new(const wchar_t *id,
	const wchar_t *dbpath,
	const wchar_t *workdir,
	const wchar_t *outpath,
	const wchar_t *errpath,
	launch_request_t **request);
void launch_request_free(launch_request_t *request);

// int process_new(const wchar_t *id, size_t idlen, process_t **process);
int process_launch(const launch_request_t *request, int argc, wchar_t **argv, process_t **process);
int process_cancel(process_t *process);
int process_wait(process_t *process);
void process_free(process_t *p);


#endif
