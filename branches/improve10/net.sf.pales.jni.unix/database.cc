#include "database.h"
#include "file.h"
#include <cstdarg>
#include <sstream>


Database::Database(const char* dirPath, const char* procid) 
:   m_dirPath(dirPath),
    m_processId(procid),
    m_dirDescriptor(::open(dirPath, O_DIRECTORY))
{
    if (m_dirPath.back() == PATHSEP) {
	m_dirPath.pop_back();
    }
    if (m_dirDescriptor == -1) {
	throw SystemError("open", __FILE__, __LINE__);
    }
}

Database::~Database() {
    if (m_dirDescriptor != -1) {
	::close(m_dirDescriptor);
    }
}

void Database::close() {
    if (m_dirDescriptor == -1) {
	return;
    }
    if (::fsync(m_dirDescriptor) == -1) {
	throw SystemError("fsync", __FILE__, __LINE__);
    }
    if (::close(m_dirDescriptor) == -1) {
	throw SystemError("close", __FILE__, __LINE__);
    }
    m_dirDescriptor = -1;
}

string Database::createFileName(State state) const
{
    ostringstream stream;
    stream << m_dirPath << PATHSEP << '0' << m_processId << '.';
    switch (state) {
    case RUNNING:
	stream << 'R';
	break;
    case FINISHED:
	stream << 'F';
	break;
    case CANCELLED:
	stream << 'C';
	break;
    case ERROR:
	stream << 'E';
	break;
    }
    return stream.str();
}

void Database::createEmptyFile(const string& filePath) {
    File f(filePath, 0600, O_EXCL | O_WRONLY);
    f.sync();
    f.close();
    sync();
}

void Database::update(State state, ...) 
{

    string filePath = createFileName(state);

    int pos = filePath.rfind(PATHSEP);

    filePath.replace(pos + 1, 1, 1, '1');
    if (state == RUNNING) {
	va_list args;
	pid_t pid;

	va_start(args, state);
	pid = va_arg(args, pid_t);
	va_end(args);

	File f(filePath, 0600, O_EXCL | O_WRONLY);
	f.write(pid);
	f.sync();
    }
    else if (state == FINISHED) {
	va_list args;
	int exitCode;

	va_start(args, state);
	exitCode = va_arg(args, int);
	va_end(args);

	File f(filePath, 0600, O_EXCL | O_WRONLY);
	f.write(exitCode);
	f.sync();
    }
    else if (state == ERROR) {
	va_list args;
	const char *format;
	va_start(args, state);
	format = va_arg(args, const char*);
	File f(filePath, 0600, O_APPEND);
	f.write(format, args);
	va_end(args);
    }
    filePath.replace(pos + 1, 1, 1, '0');

    createEmptyFile(filePath);

    if (state == FINISHED || state == CANCELLED || state == ERROR) {
	filePath.back() = 'X';
	createEmptyFile(filePath);
    }
    sync();
}

void Database::sync() {
    if (fsync(m_dirDescriptor) == -1) {
	throw SystemError("fsync", __FILE__, __LINE__);
    }
}

