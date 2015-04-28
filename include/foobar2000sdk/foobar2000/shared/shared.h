#ifndef _SHARED_DLL__SHARED_H_
#define _SHARED_DLL__SHARED_H_

#include "../../pfc/pfc.h"

// Global flag - whether it's OK to leak static objects as they'll be released anyway by process death
#define FB2K_LEAK_STATIC_OBJECTS PFC_LEAK_STATIC_OBJECTS 

#include <signal.h>

#ifndef WIN32
#error N/A
#endif

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <ddeml.h>
#include <commctrl.h>
#include <uxtheme.h>
//#include <tmschema.h>
#include <vssym32.h>

#ifndef NOTHROW
#ifdef _MSC_VER
#define NOTHROW __declspec(nothrow)
#else
#define NOTHROW
#endif
#endif

#define SHARED_API /*NOTHROW*/ __stdcall

#ifndef SHARED_EXPORTS
#define SHARED_EXPORT __declspec(dllimport) SHARED_API
#else
#define SHARED_EXPORT __declspec(dllexport) SHARED_API
#endif

extern "C" {

//SHARED_EXPORT BOOL IsUnicode();
#ifdef UNICODE
#define IsUnicode() 1
#else
#define IsUnicode() 0
#endif

LRESULT SHARED_EXPORT uSendMessageText(HWND wnd,UINT msg,WPARAM wp,const char * text);
LRESULT SHARED_EXPORT uSendDlgItemMessageText(HWND wnd,UINT id,UINT msg,WPARAM wp,const char * text);
BOOL SHARED_EXPORT uGetWindowText(HWND wnd,pfc::string_base & out);
BOOL SHARED_EXPORT uSetWindowText(HWND wnd,const char * p_text);
BOOL SHARED_EXPORT uSetWindowTextEx(HWND wnd,const char * p_text,unsigned p_text_length);
BOOL SHARED_EXPORT uGetDlgItemText(HWND wnd,UINT id,pfc::string_base & out);
BOOL SHARED_EXPORT uSetDlgItemText(HWND wnd,UINT id,const char * p_text);
BOOL SHARED_EXPORT uSetDlgItemTextEx(HWND wnd,UINT id,const char * p_text,unsigned p_text_length);
BOOL SHARED_EXPORT uBrowseForFolder(HWND parent,const char * title,pfc::string_base & out);
BOOL SHARED_EXPORT uBrowseForFolderWithFile(HWND parent,const char * title,pfc::string_base & out,const char * p_file_to_find);
int SHARED_EXPORT uMessageBox(HWND wnd,const char * text,const char * caption,UINT type);
void SHARED_EXPORT uOutputDebugString(const char * msg);
BOOL SHARED_EXPORT uAppendMenu(HMENU menu,UINT flags,UINT_PTR id,const char * content);
BOOL SHARED_EXPORT uInsertMenu(HMENU menu,UINT position,UINT flags,UINT_PTR id,const char * content);
int SHARED_EXPORT uStringCompare(const char * elem1, const char * elem2);
int SHARED_EXPORT uCharCompare(t_uint32 p_char1,t_uint32 p_char2);
int SHARED_EXPORT uStringCompare_ConvertNumbers(const char * elem1,const char * elem2);
HINSTANCE SHARED_EXPORT uLoadLibrary(const char * name);
HANDLE SHARED_EXPORT uCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,BOOL bInitialState, const char * lpName);
HANDLE SHARED_EXPORT GetInfiniteWaitEvent();
DWORD SHARED_EXPORT uGetModuleFileName(HMODULE hMod,pfc::string_base & out);
BOOL SHARED_EXPORT uSetClipboardString(const char * ptr);
BOOL SHARED_EXPORT uGetClipboardString(pfc::string_base & out);
BOOL SHARED_EXPORT uSetClipboardRawData(UINT format,const void * ptr,t_size size);//does not empty the clipboard
BOOL SHARED_EXPORT uGetClassName(HWND wnd,pfc::string_base & out);
t_size SHARED_EXPORT uCharLength(const char * src);
BOOL SHARED_EXPORT uDragQueryFile(HDROP hDrop,UINT idx,pfc::string_base & out);
UINT SHARED_EXPORT uDragQueryFileCount(HDROP hDrop);
BOOL SHARED_EXPORT uGetTextExtentPoint32(HDC dc,const char * text,UINT cb,LPSIZE size);//note, cb is number of bytes, not actual unicode characters in the string (read: plain strlen() will do)
BOOL SHARED_EXPORT uExtTextOut(HDC dc,int x,int y,UINT flags,const RECT * rect,const char * text,UINT cb,const int * lpdx);
BOOL SHARED_EXPORT uTextOutColors(HDC dc,const char * src,UINT len,int x,int y,const RECT * clip,BOOL is_selected,DWORD default_color);
BOOL SHARED_EXPORT uTextOutColorsTabbed(HDC dc,const char * src,UINT src_len,const RECT * item,int border,const RECT * clip,BOOL selected,DWORD default_color,BOOL use_columns);
UINT SHARED_EXPORT uGetTextHeight(HDC dc);
UINT SHARED_EXPORT uGetFontHeight(HFONT font);
BOOL SHARED_EXPORT uChooseColor(DWORD * p_color,HWND parent,DWORD * p_custom_colors);
HCURSOR SHARED_EXPORT uLoadCursor(HINSTANCE hIns,const char * name);
HICON SHARED_EXPORT uLoadIcon(HINSTANCE hIns,const char * name);
HMENU SHARED_EXPORT uLoadMenu(HINSTANCE hIns,const char * name);
BOOL SHARED_EXPORT uGetEnvironmentVariable(const char * name,pfc::string_base & out);
HMODULE SHARED_EXPORT uGetModuleHandle(const char * name);
UINT SHARED_EXPORT uRegisterWindowMessage(const char * name);
BOOL SHARED_EXPORT uMoveFile(const char * src,const char * dst);
BOOL SHARED_EXPORT uDeleteFile(const char * fn);
DWORD SHARED_EXPORT uGetFileAttributes(const char * fn);
BOOL SHARED_EXPORT uFileExists(const char * fn);
BOOL SHARED_EXPORT uRemoveDirectory(const char * fn);
HANDLE SHARED_EXPORT uCreateFile(const char * p_path,DWORD p_access,DWORD p_sharemode,LPSECURITY_ATTRIBUTES p_security_attributes,DWORD p_createmode,DWORD p_flags,HANDLE p_template);
HANDLE SHARED_EXPORT uCreateFileMapping(HANDLE hFile,LPSECURITY_ATTRIBUTES lpFileMappingAttributes,DWORD flProtect,DWORD dwMaximumSizeHigh,DWORD dwMaximumSizeLow,const char * lpName);
BOOL SHARED_EXPORT uCreateDirectory(const char * fn,LPSECURITY_ATTRIBUTES blah);
HANDLE SHARED_EXPORT uCreateMutex(LPSECURITY_ATTRIBUTES blah,BOOL bInitialOwner,const char * name);
BOOL SHARED_EXPORT uGetLongPathName(const char * name,pfc::string_base & out);//may just fail to work on old windows versions, present on win98/win2k+
BOOL SHARED_EXPORT uGetFullPathName(const char * name,pfc::string_base & out);
BOOL SHARED_EXPORT uSearchPath(const char * path, const char * filename, const char * extension, pfc::string_base & p_out);
BOOL SHARED_EXPORT uFixPathCaps(const char * path,pfc::string_base & p_out);
//BOOL SHARED_EXPORT uFixPathCapsQuick(const char * path,pfc::string_base & p_out);
void SHARED_EXPORT uGetCommandLine(pfc::string_base & out);
BOOL SHARED_EXPORT uGetTempPath(pfc::string_base & out);
BOOL SHARED_EXPORT uGetTempFileName(const char * path_name,const char * prefix,UINT unique,pfc::string_base & out);
BOOL SHARED_EXPORT uGetOpenFileName(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,pfc::string_base & p_filename,BOOL b_save);
//note: uGetOpenFileName extension mask uses | as separator, not null
HANDLE SHARED_EXPORT uLoadImage(HINSTANCE hIns,const char * name,UINT type,int x,int y,UINT flags);
UINT SHARED_EXPORT uRegisterClipboardFormat(const char * name);
BOOL SHARED_EXPORT uGetClipboardFormatName(UINT format,pfc::string_base & out);
BOOL SHARED_EXPORT uFormatSystemErrorMessage(pfc::string_base & p_out,DWORD p_code);

HANDLE SHARED_EXPORT uSortStringCreate(const char * src);
int SHARED_EXPORT uSortStringCompare(HANDLE string1,HANDLE string2);
int SHARED_EXPORT uSortStringCompareEx(HANDLE string1,HANDLE string2,DWORD flags);//flags - see win32 CompareString
int SHARED_EXPORT uSortPathCompare(HANDLE string1,HANDLE string2);
void SHARED_EXPORT uSortStringFree(HANDLE string);

// New in 1.1.12
HANDLE SHARED_EXPORT CreateFileAbortable(    __in     LPCWSTR lpFileName,
    __in     DWORD dwDesiredAccess,
    __in     DWORD dwShareMode,
    __in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    __in     DWORD dwCreationDisposition,
    __in     DWORD dwFlagsAndAttributes,
	// Template file handle NOT supported.
	__in_opt HANDLE hAborter
	);


int SHARED_EXPORT uCompareString(DWORD flags,const char * str1,unsigned len1,const char * str2,unsigned len2);

class NOVTABLE uGetOpenFileNameMultiResult : public pfc::list_base_const_t<const char*>
{
public:
	inline t_size GetCount() {return get_count();}
	inline const char * GetFileName(t_size index) {return get_item(index);}
	virtual ~uGetOpenFileNameMultiResult() {}
};

typedef uGetOpenFileNameMultiResult * puGetOpenFileNameMultiResult;

puGetOpenFileNameMultiResult SHARED_EXPORT uGetOpenFileNameMulti(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory);

// new in fb2k 1.1.1
puGetOpenFileNameMultiResult SHARED_EXPORT uBrowseForFolderEx(HWND parent,const char * title, const char * initPath);
puGetOpenFileNameMultiResult SHARED_EXPORT uEvalKnownFolder(const GUID& id);


class NOVTABLE uFindFile
{
protected:
	uFindFile() {}
public:
	virtual BOOL FindNext()=0;
	virtual const char * GetFileName()=0;
	virtual t_uint64 GetFileSize()=0;
	virtual DWORD GetAttributes()=0;
	virtual FILETIME GetCreationTime()=0;
	virtual FILETIME GetLastAccessTime()=0;
	virtual FILETIME GetLastWriteTime()=0;
	virtual ~uFindFile() {};
	inline bool IsDirectory() {return (GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) ? true : false;}
};

typedef uFindFile * puFindFile;

puFindFile SHARED_EXPORT uFindFirstFile(const char * path);

HINSTANCE SHARED_EXPORT uShellExecute(HWND wnd,const char * oper,const char * file,const char * params,const char * dir,int cmd);
HWND SHARED_EXPORT uCreateStatusWindow(LONG style,const char * text,HWND parent,UINT id);

BOOL SHARED_EXPORT uShellNotifyIcon(DWORD dwMessage,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip);
BOOL SHARED_EXPORT uShellNotifyIconEx(DWORD dwMessage,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip,const char * balloon_title,const char * balloon_msg);

HWND SHARED_EXPORT uCreateWindowEx(DWORD dwExStyle,const char * lpClassName,const char * lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);

BOOL SHARED_EXPORT uGetSystemDirectory(pfc::string_base & out); 
BOOL SHARED_EXPORT uGetWindowsDirectory(pfc::string_base & out); 
BOOL SHARED_EXPORT uSetCurrentDirectory(const char * path);
BOOL SHARED_EXPORT uGetCurrentDirectory(pfc::string_base & out);
BOOL SHARED_EXPORT uExpandEnvironmentStrings(const char * src,pfc::string_base & out);
BOOL SHARED_EXPORT uGetUserName(pfc::string_base & out);
BOOL SHARED_EXPORT uGetShortPathName(const char * src,pfc::string_base & out);

HSZ SHARED_EXPORT uDdeCreateStringHandle(DWORD ins,const char * src);
BOOL SHARED_EXPORT uDdeQueryString(DWORD ins,HSZ hsz,pfc::string_base & out);
UINT SHARED_EXPORT uDdeInitialize(LPDWORD pidInst,PFNCALLBACK pfnCallback,DWORD afCmd,DWORD ulRes);
BOOL SHARED_EXPORT uDdeAccessData_Text(HDDEDATA data,pfc::string_base & out);

HIMAGELIST SHARED_EXPORT uImageList_LoadImage(HINSTANCE hi, const char * lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);

#define uDdeFreeStringHandle DdeFreeStringHandle
#define uDdeCmpStringHandles DdeCmpStringHandles
#define uDdeKeepStringHandle DdeKeepStringHandle
#define uDdeUninitialize DdeUninitialize
#define uDdeNameService DdeNameService
#define uDdeFreeDataHandle DdeFreeDataHandle


typedef TVINSERTSTRUCTA uTVINSERTSTRUCT;

HTREEITEM SHARED_EXPORT uTreeView_InsertItem(HWND wnd,const uTVINSERTSTRUCT * param);
LPARAM SHARED_EXPORT uTreeView_GetUserData(HWND wnd,HTREEITEM item);
bool SHARED_EXPORT uTreeView_GetText(HWND wnd,HTREEITEM item,pfc::string_base & out);

#define uSetWindowsHookEx SetWindowsHookEx
#define uUnhookWindowsHookEx UnhookWindowsHookEx
#define uCallNextHookEx CallNextHookEx

typedef TCITEMA uTCITEM;
int SHARED_EXPORT uTabCtrl_InsertItem(HWND wnd,t_size idx,const uTCITEM * item);
int SHARED_EXPORT uTabCtrl_SetItem(HWND wnd,t_size idx,const uTCITEM * item);

int SHARED_EXPORT uGetKeyNameText(LONG lparam,pfc::string_base & out);

void SHARED_EXPORT uFixAmpersandChars(const char * src,pfc::string_base & out);//for notification area icon
void SHARED_EXPORT uFixAmpersandChars_v2(const char * src,pfc::string_base & out);//for other controls

//deprecated
t_size SHARED_EXPORT uPrintCrashInfo(LPEXCEPTION_POINTERS param,const char * extrainfo,char * out);
enum {uPrintCrashInfo_max_length = 1024};

void SHARED_EXPORT uPrintCrashInfo_Init(const char * name);//called only by the exe on startup
void SHARED_EXPORT uPrintCrashInfo_SetComponentList(const char * p_info);//called only by the exe on startup
void SHARED_EXPORT uPrintCrashInfo_AddEnvironmentInfo(const char * p_info);//called only by the exe on startup
void SHARED_EXPORT uPrintCrashInfo_SetDumpPath(const char * name);//called only by the exe on startup

void SHARED_EXPORT uDumpCrashInfo(LPEXCEPTION_POINTERS param);



void SHARED_EXPORT uPrintCrashInfo_OnEvent(const char * message, t_size length);

BOOL SHARED_EXPORT uListBox_GetText(HWND listbox,UINT index,pfc::string_base & out);

void SHARED_EXPORT uPrintfV(pfc::string_base & out,const char * fmt,va_list arglist);
static inline void uPrintf(pfc::string_base & out,const char * fmt,...) {va_list list;va_start(list,fmt);uPrintfV(out,fmt,list);va_end(list);}


class NOVTABLE uResource
{
public:
	virtual const void * GetPointer() = 0;
	virtual unsigned GetSize() = 0;
	virtual ~uResource() {}
};

typedef uResource* puResource;

puResource SHARED_EXPORT uLoadResource(HMODULE hMod,const char * name,const char * type,WORD wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );
puResource SHARED_EXPORT LoadResourceEx(HMODULE hMod,const TCHAR * name,const TCHAR * type,WORD wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );
HRSRC SHARED_EXPORT uFindResource(HMODULE hMod,const char * name,const char * type,WORD wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );

BOOL SHARED_EXPORT uLoadString(HINSTANCE ins,UINT id,pfc::string_base & out);

UINT SHARED_EXPORT uCharLower(UINT c);
UINT SHARED_EXPORT uCharUpper(UINT c);

BOOL SHARED_EXPORT uGetMenuString(HMENU menu,UINT id,pfc::string_base & out,UINT flag);
BOOL SHARED_EXPORT uModifyMenu(HMENU menu,UINT id,UINT flags,UINT newitem,const char * data);
UINT SHARED_EXPORT uGetMenuItemType(HMENU menu,UINT position);


// New in 1.3.4
// Load a system library safely - forcibly look in system directories, not elsewhere.
HMODULE SHARED_EXPORT LoadSystemLibrary(const TCHAR * name);
}//extern "C"

static inline void uAddDebugEvent(const char * msg) {uPrintCrashInfo_OnEvent(msg, strlen(msg));}
static inline void uAddDebugEvent(pfc::string_formatter const & msg) {uPrintCrashInfo_OnEvent(msg, msg.length());}

inline char * uCharNext(char * src) {return src+uCharLength(src);}
inline const char * uCharNext(const char * src) {return src+uCharLength(src);}


class string_utf8_from_window
{
public:
	string_utf8_from_window(HWND wnd)
	{
		uGetWindowText(wnd,m_data);
	}
	string_utf8_from_window(HWND wnd,UINT id)
	{
		uGetDlgItemText(wnd,id,m_data);
	}
	inline operator const char * () const {return m_data.get_ptr();}
	inline t_size length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
private:
	pfc::string8 m_data;
};

static pfc::string uGetWindowText(HWND wnd) {
	pfc::string8 temp;
	if (!uGetWindowText(wnd,temp)) return "";
	return temp.toString();
}
static pfc::string uGetDlgItemText(HWND wnd,UINT id) {
	pfc::string8 temp;
	if (!uGetDlgItemText(wnd,id,temp)) return "";
	return temp.toString();
}

#define uMAKEINTRESOURCE(x) ((const char*)LOWORD(x))

//other

#define uIsDialogMessage IsDialogMessage
#define uGetMessage GetMessage
#define uPeekMessage PeekMessage
#define uDispatchMessage DispatchMessage

#define uCallWindowProc CallWindowProc
#define uDefWindowProc DefWindowProc
#define uGetWindowLong GetWindowLong
#define uSetWindowLong SetWindowLong

#define uEndDialog EndDialog
#define uDestroyWindow DestroyWindow
#define uGetDlgItem GetDlgItem
#define uEnableWindow EnableWindow
#define uGetDlgItemInt GetDlgItemInt
#define uSetDlgItemInt SetDlgItemInt

#define uSendMessage SendMessage
#define uSendDlgItemMessage SendDlgItemMessage
#define uSendMessageTimeout SendMessageTimeout
#define uSendNotifyMessage SendNotifyMessage
#define uSendMessageCallback SendMessageCallback
#define uPostMessage PostMessage
#define uPostThreadMessage PostThreadMessage


class uStringPrintf
{
public:
	inline explicit uStringPrintf(const char * fmt,...)
	{
		va_list list;
		va_start(list,fmt);
		uPrintfV(m_data,fmt,list);
		va_end(list);
	}
	inline operator const char * () const {return m_data.get_ptr();}
	inline t_size length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
	const char * toString() const {return get_ptr();}
private:
	pfc::string8_fastalloc m_data;
};
#pragma deprecated(uStringPrintf, uPrintf, uPrintfV)

inline LRESULT uButton_SetCheck(HWND wnd,UINT id,bool state) {return uSendDlgItemMessage(wnd,id,BM_SETCHECK,state ? BST_CHECKED : BST_UNCHECKED,0); }
inline bool uButton_GetCheck(HWND wnd,UINT id) {return uSendDlgItemMessage(wnd,id,BM_GETCHECK,0,0) == BST_CHECKED;}



extern "C" {
int SHARED_EXPORT stricmp_utf8(const char * p1,const char * p2) throw();
int SHARED_EXPORT stricmp_utf8_ex(const char * p1,t_size len1,const char * p2,t_size len2) throw();
int SHARED_EXPORT stricmp_utf8_stringtoblock(const char * p1,const char * p2,t_size p2_bytes) throw();
int SHARED_EXPORT stricmp_utf8_partial(const char * p1,const char * p2,t_size num = ~0) throw();
int SHARED_EXPORT stricmp_utf8_max(const char * p1,const char * p2,t_size p1_bytes) throw();
t_size SHARED_EXPORT uReplaceStringAdd(pfc::string_base & out,const char * src,t_size src_len,const char * s1,t_size len1,const char * s2,t_size len2,bool casesens);
t_size SHARED_EXPORT uReplaceCharAdd(pfc::string_base & out,const char * src,t_size src_len,unsigned c1,unsigned c2,bool casesens);
//all lengths in uReplaceString functions are optional, set to -1 if parameters is a simple null-terminated string
void SHARED_EXPORT uAddStringLower(pfc::string_base & out,const char * src,t_size len = ~0);
void SHARED_EXPORT uAddStringUpper(pfc::string_base & out,const char * src,t_size len = ~0);
}

class comparator_stricmp_utf8 {
public:
	static int compare(const char * p_string1,const char * p_string2) throw() {return stricmp_utf8(p_string1,p_string2);}
};

inline void uStringLower(pfc::string_base & out,const char * src,t_size len = ~0) {out.reset();uAddStringLower(out,src,len);}
inline void uStringUpper(pfc::string_base & out,const char * src,t_size len = ~0) {out.reset();uAddStringUpper(out,src,len);}

inline t_size uReplaceString(pfc::string_base & out,const char * src,t_size src_len,const char * s1,t_size len1,const char * s2,t_size len2,bool casesens)
{
	out.reset();
	return uReplaceStringAdd(out,src,src_len,s1,len1,s2,len2,casesens);
}

inline t_size uReplaceChar(pfc::string_base & out,const char * src,t_size src_len,unsigned c1,unsigned c2,bool casesens)
{
	out.reset();
	return uReplaceCharAdd(out,src,src_len,c1,c2,casesens);
}

class string_lower
{
public:
	explicit string_lower(const char * ptr,t_size p_count = ~0) {uAddStringLower(m_data,ptr,p_count);}
	inline operator const char * () const {return m_data.get_ptr();}
	inline t_size length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
private:
	pfc::string8 m_data;
};

class string_upper
{
public:
	explicit string_upper(const char * ptr,t_size p_count = ~0) {uAddStringUpper(m_data,ptr,p_count);}
	inline operator const char * () const {return m_data.get_ptr();}
	inline t_size length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
private:
	pfc::string8 m_data;
};


inline BOOL uGetLongPathNameEx(const char * name,pfc::string_base & out)
{
	if (uGetLongPathName(name,out)) return TRUE;
	return uGetFullPathName(name,out);
}

struct t_font_description
{
	enum
	{
		m_facename_length = LF_FACESIZE*2,
		m_height_dpi = 480,
	};

	t_uint32 m_height;
	t_uint32 m_weight;
	t_uint8 m_italic;
	t_uint8 m_charset;
	char m_facename[m_facename_length];

	bool operator==(const t_font_description & other) const {return g_equals(*this, other);}
	bool operator!=(const t_font_description & other) const {return !g_equals(*this, other);}
	
	static bool g_equals(const t_font_description & v1, const t_font_description & v2) {
		return v1.m_height == v2.m_height && v1.m_weight == v2.m_weight && v1.m_italic == v2.m_italic && v1.m_charset == v2.m_charset && pfc::strcmp_ex(v1.m_facename, m_facename_length, v2.m_facename, m_facename_length) == 0;
	}

	HFONT SHARED_EXPORT create() const;
	bool SHARED_EXPORT popup_dialog(HWND p_parent);
	void SHARED_EXPORT from_font(HFONT p_font);
	static t_font_description SHARED_EXPORT g_from_font(HFONT p_font);
	static t_font_description SHARED_EXPORT g_from_logfont(LOGFONT const & lf);
	static t_font_description SHARED_EXPORT g_from_system(int id = TMT_MENUFONT);

	template<typename t_stream,typename t_abort> void to_stream(t_stream p_stream,t_abort & p_abort) const;
	template<typename t_stream,typename t_abort> void from_stream(t_stream p_stream,t_abort & p_abort);
};

/* relevant types not yet defined here */ template<typename t_stream,typename t_abort> void t_font_description::to_stream(t_stream p_stream,t_abort & p_abort) const {
	p_stream->write_lendian_t(m_height,p_abort);
	p_stream->write_lendian_t(m_weight,p_abort);
	p_stream->write_lendian_t(m_italic,p_abort);
	p_stream->write_lendian_t(m_charset,p_abort);
	p_stream->write_string(m_facename,PFC_TABSIZE(m_facename),p_abort);
}

/* relevant types not yet defined here */ template<typename t_stream,typename t_abort> void t_font_description::from_stream(t_stream p_stream,t_abort & p_abort) {
	p_stream->read_lendian_t(m_height,p_abort);
	p_stream->read_lendian_t(m_weight,p_abort);
	p_stream->read_lendian_t(m_italic,p_abort);
	p_stream->read_lendian_t(m_charset,p_abort);
	pfc::string8 temp;
	p_stream->read_string(temp,p_abort);
	strncpy_s(m_facename,temp,PFC_TABSIZE(m_facename));
}


struct t_modal_dialog_entry
{
	HWND m_wnd_to_poke;
	bool m_in_use;
};

extern "C" {
	void SHARED_EXPORT ModalDialog_Switch(t_modal_dialog_entry & p_entry);
	void SHARED_EXPORT ModalDialog_PokeExisting();
	bool SHARED_EXPORT ModalDialog_CanCreateNew();

	HWND SHARED_EXPORT FindOwningPopup(HWND p_wnd);
	void SHARED_EXPORT PokeWindow(HWND p_wnd);
};

static bool ModalDialogPrologue() {
	bool rv = ModalDialog_CanCreateNew();
	if (!rv) ModalDialog_PokeExisting();
	return rv;
}

//! The purpose of modal_dialog_scope is to help to avoid the modal dialog recursion problem. Current toplevel modal dialog handle is stored globally, so when creation of a new modal dialog is blocked, it can be activated to indicate the reason for the task being blocked.
class modal_dialog_scope {
public:
	//! This constructor initializes the modal dialog scope with specified dialog handle.
	inline modal_dialog_scope(HWND p_wnd) throw() : m_initialized(false) {initialize(p_wnd);}
	//! This constructor leaves the scope uninitialized (you can call initialize() later with your window handle).
	inline modal_dialog_scope() throw() : m_initialized(false) {}
	inline ~modal_dialog_scope() throw() {deinitialize();}

	//! Returns whether creation of a new modal dialog is allowed (false when there's another one active).\n
	//! NOTE: when calling context is already inside a modal dialog that you own, you should not be checking this before creating a new modal dialog.
	inline static bool can_create() throw(){return ModalDialog_CanCreateNew();}
	//! Activates the top-level modal dialog existing, if one exists.
	inline static void poke_existing() throw() {ModalDialog_PokeExisting();}

	//! Initializes the scope with specified window handle.
	void initialize(HWND p_wnd) throw()
	{
		if (!m_initialized)
		{
			m_initialized = true;
			m_entry.m_in_use = true;
			m_entry.m_wnd_to_poke = p_wnd;
			ModalDialog_Switch(m_entry);
		}
	}

	void deinitialize() throw()
	{
		if (m_initialized)
		{
			ModalDialog_Switch(m_entry);
			m_initialized = false;
		}
	}

	

private:
	modal_dialog_scope(const modal_dialog_scope & p_scope) {assert(0);}
	const modal_dialog_scope & operator=(const modal_dialog_scope &) {assert(0); return *this;}

	t_modal_dialog_entry m_entry;

	bool m_initialized;
};



#include "audio_math.h"
#include "win32_misc.h"

#include "fb2kdebug.h"

#if FB2K_LEAK_STATIC_OBJECTS
#define FB2K_STATIC_OBJECT(TYPE, NAME) static TYPE & NAME = * new TYPE;
#else
#define FB2K_STATIC_OBJECT(TYPE, NAME) static TYPE NAME;
#endif

#endif //_SHARED_DLL__SHARED_H_
