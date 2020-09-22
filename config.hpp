#pragma once

#include "taskbarvol.hpp"

#include <stdexcept>
#include <string>

class config {
public:
	config();
	~config();
	static config& get_instance();
	void load_config();
	void save_config();

private:
	HANDLE m_hMutex;
};

class mutex_locker {
public:
	mutex_locker(HANDLE hMutex) : m_hMutex(hMutex) {
		_ASSERT(m_hMutex);
		DWORD ret = ::WaitForSingleObject(m_hMutex, INFINITE);
		if (ret != WAIT_OBJECT_0) {
			throw std::runtime_error("WaitForSingleObject failed");
		}
	}
	~mutex_locker() {
		::ReleaseMutex(m_hMutex);
	}
private:
	HANDLE m_hMutex;
};
