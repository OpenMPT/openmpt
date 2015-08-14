#if !defined(PP_WINAPI_H_INCLUDED) && defined(_WIN32)
#define PP_WINAPI_H_INCLUDED

#ifdef WINAPI_FAMILY_PARTITION

#if ! WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

inline HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName) {
	DWORD flags = 0;
	if (bManualReset) flags |= CREATE_EVENT_MANUAL_RESET;
	if (bInitialState) flags |= CREATE_EVENT_INITIAL_SET;
	DWORD rights = SYNCHRONIZE | EVENT_MODIFY_STATE;
	return CreateEventEx(lpEventAttributes, lpName, flags, rights);
}

inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
	return WaitForSingleObjectEx(hHandle, dwMilliseconds, FALSE);
}

inline DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) {
	return WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, FALSE);
}

inline void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
	InitializeCriticalSectionEx(lpCriticalSection, 0, 0);
}

inline HANDLE FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) {
	return FindFirstFileEx(lpFileName, FindExInfoStandard, lpFindFileData, FindExSearchNameMatch, NULL, 0);
}

inline BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize) {
	FILE_STANDARD_INFO info;
	if (!GetFileInformationByHandleEx(hFile, FileStandardInfo, &info, sizeof(info))) return FALSE;
	*lpFileSize = info.EndOfFile;
	return TRUE;
}

inline HANDLE CreateFileW(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
	CREATEFILE2_EXTENDED_PARAMETERS arg = {};
	arg.dwSize = sizeof(arg);
	arg.hTemplateFile = hTemplateFile;
	arg.lpSecurityAttributes = lpSecurityAttributes;
	arg.dwFileAttributes = dwFlagsAndAttributes & 0x0000FFFF;
	arg.dwFileFlags = dwFlagsAndAttributes & 0xFFFF0000;
	return CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, &arg);
}

inline DWORD GetFileAttributesW(const wchar_t * path) {
	WIN32_FILE_ATTRIBUTE_DATA data = {};
	if (!GetFileAttributesEx(path, GetFileExInfoStandard, &data)) return 0xFFFFFFFF;
	return data.dwFileAttributes;
}

#define GetFileAttributes GetFileAttributesW

#endif // #if ! WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#endif // #ifdef WINAPI_FAMILY_PARTITION

#endif // !defined(PP_WINAPI_H_INCLUDED) && defined(_WIN32)