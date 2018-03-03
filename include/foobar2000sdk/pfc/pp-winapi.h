#if !defined(PP_WINAPI_H_INCLUDED) && defined(_WIN32)
#define PP_WINAPI_H_INCLUDED

#ifdef WINAPI_FAMILY_PARTITION

#if ! WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#ifndef CreateEvent // SPECIAL HACK: disable this stuff if somehow these functions are already defined

inline HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName) {
	DWORD flags = 0;
	if (bManualReset) flags |= CREATE_EVENT_MANUAL_RESET;
	if (bInitialState) flags |= CREATE_EVENT_INITIAL_SET;
	DWORD rights = SYNCHRONIZE | EVENT_MODIFY_STATE;
	return CreateEventEx(lpEventAttributes, lpName, flags, rights);
}

#define CreateEvent CreateEventW

inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
	return WaitForSingleObjectEx(hHandle, dwMilliseconds, FALSE);
}

inline DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) {
	return WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, FALSE);
}

inline void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
	InitializeCriticalSectionEx(lpCriticalSection, 0, 0);
}

#endif // #ifndef CreateEvent


#ifndef CreateMutex

inline HANDLE CreateMutexW(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName) {
	DWORD rights = MUTEX_MODIFY_STATE | SYNCHRONIZE;
	DWORD flags = 0;
	if (bInitialOwner) flags |= CREATE_MUTEX_INITIAL_OWNER;
	return CreateMutexExW(lpMutexAttributes, lpName, flags, rights);
}

#define CreateMutex CreateMutexW

#endif // CreateMutex


#ifndef FindFirstFile

inline HANDLE FindFirstFileW(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) {
	return FindFirstFileEx(lpFileName, FindExInfoStandard, lpFindFileData, FindExSearchNameMatch, NULL, 0);
}

#define FindFirstFile FindFirstFileW

#endif // #ifndef FindFirstFile

// No reliable way to detect if GetFileSizeEx is present?? Give ours another name
inline BOOL GetFileSizeEx_Fallback(HANDLE hFile, PLARGE_INTEGER lpFileSize) {
	FILE_STANDARD_INFO info;
	if (!GetFileInformationByHandleEx(hFile, FileStandardInfo, &info, sizeof(info))) return FALSE;
	*lpFileSize = info.EndOfFile;
	return TRUE;
}

#define PP_GetFileSizeEx_Fallback_Present

#ifndef CreateFile

inline HANDLE CreateFileW(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
	CREATEFILE2_EXTENDED_PARAMETERS arg = {};
	arg.dwSize = sizeof(arg);
	arg.hTemplateFile = hTemplateFile;
	arg.lpSecurityAttributes = lpSecurityAttributes;
	arg.dwFileAttributes = dwFlagsAndAttributes & 0x0000FFFF;
	arg.dwFileFlags = dwFlagsAndAttributes & 0xFFFF0000;
	return CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, &arg);
}

#define CreateFile CreateFileW

#endif // #ifndef CreateFile

#ifndef GetFileAttributes

inline DWORD GetFileAttributesW(const wchar_t * path) {
	WIN32_FILE_ATTRIBUTE_DATA data = {};
	if (!GetFileAttributesEx(path, GetFileExInfoStandard, &data)) return 0xFFFFFFFF;
	return data.dwFileAttributes;
}

#define GetFileAttributes GetFileAttributesW

#endif // #ifndef GetFileAttributes

#endif // #if ! WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#endif // #ifdef WINAPI_FAMILY_PARTITION

#ifndef PP_GetFileSizeEx_Fallback_Present
#define GetFileSizeEx_Fallback GetFileSizeEx
#endif

#endif // !defined(PP_WINAPI_H_INCLUDED) && defined(_WIN32)
