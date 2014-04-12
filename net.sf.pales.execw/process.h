#ifndef PROCESS_H
#define PROCESS_H

#include <windows.h>
#include <stdint.h>

enum procstat {
	running,
	finished,
	cancelled
};

typedef enum procstat procstat_t;

typedef struct process {
	const char *id;
	HANDLE handle;
	HANDLE job;
	DWORD pid;
	procstat_t status;
	DWORD exitcode;
} process_t;



typedef struct launch_request {
	const char *dbpath;
	const char *workdir;
	const char *errpath;
	const char *outpath;
	const char *id;
} launch_request_t;


int launch_request_new(const char *id,
	const char *dbpath,
	const char *workdir,
	const char *outpath,
	const char *errpath,
	launch_request_t **request);
void launch_request_free(launch_request_t *request);

int process_new(const char *id, size_t idlen, process_t **process);
int process_launch(const launch_request_t *request, int argc, char **argv, process_t **process);
int process_cancel(process_t *process);
int process_wait(process_t *process);
void process_free(process_t *p);


#endif
