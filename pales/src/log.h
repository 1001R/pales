#ifndef LOG_H
#define LOG_H

void log_init(const wchar_t *dbDirPath, const wchar_t *processId);

int log_message(const wchar_t *message, ...);


#endif LOG_H