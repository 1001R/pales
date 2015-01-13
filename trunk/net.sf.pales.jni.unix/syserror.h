#pragma once
#include <stdexcept>
#include <sstream>
#include <cerrno>
#include <string>
#include <cstring>


using namespace std;

class SystemError : public runtime_error {
    static const size_t bufsize = 256;
    static char m_errbuf[bufsize];
    static string createErrorMessage(const char* syscall, const char* file, unsigned int line, int error) {
	ostringstream stream;
	stream << file << ':' << line << " - " << syscall << " failed: " << strerror(error);
	return stream.str();
    }
public:
    SystemError(const char* syscall, const char* file, unsigned int line, int error = errno) 
	: runtime_error(createErrorMessage(syscall, file, line, error))
    {

    }
};

