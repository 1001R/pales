#pragma once
#include "syserror.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstdarg>
#include <memory>

using namespace std;

class File {
    int m_descriptor;
    string m_filePath;
    string m_dirPath;

    void computeDirectoryPath() {
	size_t pos = m_filePath.rfind('/');
	if (pos == string::npos) {
	    m_dirPath = ".";
	}
	else {
	    m_dirPath = m_filePath.substr(0, pos);
	}
    }
public:
    File(const string& filepath, mode_t mode, int flags = 0) : m_descriptor(::open(filepath.c_str(), flags | O_CREAT , mode)), m_filePath(filepath) {
	if (m_descriptor == -1) {
	    throw SystemError("open", __FILE__, __LINE__);
	}
	//computeDirectoryPath();
    }

    File(const string& filepath, int flags = 0) : m_descriptor(::open(filepath.c_str(), flags | O_WRONLY & ~O_CREAT)) {
	if (m_descriptor == -1) {
	    throw SystemError("open", __FILE__, __LINE__);
	}
    }

    ~File() {
	if (m_descriptor != -1) {
	    ::close(m_descriptor);
	}
    }

    void write(const char* buf, size_t len) {
	if (::write(m_descriptor, buf, len) != len) {
	    throw SystemError("write", __FILE__, __LINE__);
	}
    }

    void write(unsigned int i) {
	string s = to_string(i);
	write(s.data(), s.length());
    }

    void write(const char* format, va_list args) {
	va_list argsCopy;
	va_copy(argsCopy, args);
	int bufsz = vsnprintf(nullptr, 0, format, args) + 1;
	unique_ptr<char> buf(new char[bufsz]);
	vsnprintf(buf.get(), bufsz, format, argsCopy);
	va_end(argsCopy);
	write(buf.get(), bufsz - 1);
    }

    void sync() {
	if (fsync(m_descriptor) != 0) {
	    throw SystemError("fsync", __FILE__, __LINE__);
	}
    }

    void close() {
	if (::close(m_descriptor) == -1) {
	    throw SystemError("close", __FILE__, __LINE__);
	}
	m_descriptor = -1;
    }
};
