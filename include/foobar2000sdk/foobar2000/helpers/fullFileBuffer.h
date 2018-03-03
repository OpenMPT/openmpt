#pragma once

class fullFileBuffer {
public:
	fullFileBuffer() {
		//hMutex = CreateMutex(NULL, FALSE, pfc::stringcvt::string_os_from_utf8(pfc::string_formatter() << "{3ABC4D98-2510-431C-A720-26BEB45F0278}-" << (uint32_t) GetCurrentProcessId()));
	}
	~fullFileBuffer() {
		//CloseHandle(hMutex);
	}
	file::ptr open(const char * path, abort_callback & abort, file::ptr hint, t_filesize sizeMax = 1024 * 1024 * 256);

private:
	fullFileBuffer(const fullFileBuffer&);
	void operator=(const fullFileBuffer&);

	//HANDLE hMutex;
};
