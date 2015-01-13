#pragma once
#include "syserror.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

class Semaphore {
    sem_t* m_semaphore;
    const string m_name;
    bool m_deleteOnClose;
public:
    Semaphore(const string& name, int flags, mode_t mode = 0, int initialValue = 0, bool deleteOnClose = false) : m_name(name), m_deleteOnClose(deleteOnClose) {
	m_semaphore = sem_open(name.c_str(), flags | O_CREAT, mode, initialValue);
	if (m_semaphore == SEM_FAILED) {
	    throw SystemError("sem_open", __FILE__, __LINE__);
	}
    }

    Semaphore(const string& name, int flags) : m_deleteOnClose(false) {
	m_semaphore = sem_open(name.c_str(), flags & ~O_CREAT);
	if (m_semaphore == SEM_FAILED) {
	    throw SystemError("sem_open", __FILE__, __LINE__);
	}
    }

    ~Semaphore() {
	sem_close(m_semaphore);
	if (m_deleteOnClose) {
	    sem_unlink(m_name.c_str());
	}
    }

    void post() {
	if (sem_post(m_semaphore) != 0) {
	    throw SystemError("sem_close", __FILE__, __LINE__);
	}
    }

    void wait() {
	if (sem_wait(m_semaphore) != 0) {
	    throw SystemError("sem_wait", __FILE__, __LINE__);
	}
    }

    operator sem_t* () {
	return m_semaphore;
    }
};
