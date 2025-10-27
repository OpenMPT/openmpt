/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_IO_FILE_ATOMIC_ATOMIC_FILE_HPP
#define MPT_IO_FILE_ATOMIC_ATOMIC_FILE_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/saturate_cast.hpp"
#include "mpt/path/os_path.hpp"
#include "mpt/path/os_path_long.hpp"
#include "mpt/system_error/system_error.hpp"

#include <stdexcept>
#include <utility>

#include <cstddef>

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace IO {



template <typename T>
class atomic_file_lock_guard {

private:

	T & file;
	
public:

	atomic_file_lock_guard(T & f)
		: file(f)
	{
		file.lock();
	}

	~atomic_file_lock_guard() noexcept(false) {
		file.unlock();
	}

	atomic_file_lock_guard(const atomic_file_lock_guard &) = delete;
	atomic_file_lock_guard & operator =(const atomic_file_lock_guard &) = delete;

};



#if MPT_OS_WINDOWS


class atomic_file_ref {

protected:

	const mpt::PathString m_filename;
	HANDLE m_hFile = NULL;

public:

	atomic_file_ref(mpt::PathString filename)
		: m_filename(std::move(filename))
	{
		mpt::windows::CheckBOOL(CopyFile(mpt::support_long_path(m_filename).c_str(), mpt::support_long_path(m_filename + MPT_PATHSTRING(".bak")).c_str(), FALSE));
		m_hFile = mpt::windows::CheckFileHANDLE(CreateFile(mpt::support_long_path(m_filename).c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

protected:

	atomic_file_ref(mpt::PathString filename, bool shared)
		: m_filename(std::move(filename))
	{
		m_hFile = mpt::windows::CheckFileHANDLE(CreateFile(mpt::support_long_path(m_filename).c_str(), GENERIC_READ | GENERIC_WRITE, shared ? (FILE_SHARE_READ | FILE_SHARE_WRITE) : 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

public:

	atomic_file_ref(const atomic_file_ref &) = delete;
	atomic_file_ref & operator =(const atomic_file_ref &) = delete;

	void lock() {
	}

	std::vector<std::byte> read() {
		std::vector<std::byte> data;
		{
			LARGE_INTEGER pos{};
			pos.QuadPart = 0;
			mpt::windows::CheckBOOL(SetFilePointerEx(m_hFile, pos, NULL, FILE_BEGIN));
		}
		LARGE_INTEGER size{};
		mpt::windows::CheckBOOL(GetFileSizeEx(m_hFile, &size));
		{
			data.resize(mpt::saturate_cast<std::size_t>(size.QuadPart));
			uint64 bytesToRead = size.QuadPart;
			uint64 bytesRead = 0;
			while (bytesToRead > 0) {
				DWORD bytesChunkToRead = mpt::saturate_cast<DWORD>(bytesToRead);
				DWORD bytesChunkRead = 0;
				mpt::windows::CheckBOOL(ReadFile(m_hFile, data.data() + bytesRead, bytesChunkToRead, &bytesChunkRead, NULL));
				bytesRead += bytesChunkRead;
				bytesToRead -= bytesChunkRead;
				if (!bytesChunkRead) {
					throw std::runtime_error("");
					break;
				}
			}
		}
		{
			LARGE_INTEGER pos{};
			pos.QuadPart = 0;
			mpt::windows::CheckBOOL(SetFilePointerEx(m_hFile, pos, NULL, FILE_BEGIN));
		}
		return data;
	}

	void write(mpt::byte_span data, bool sync = false) {
		{
			LARGE_INTEGER pos{};
			pos.QuadPart = 0;
			mpt::windows::CheckBOOL(SetFilePointerEx(m_hFile, pos, NULL, FILE_BEGIN));
		}
		mpt::windows::CheckBOOL(SetEndOfFile(m_hFile));
		{
			uint64 bytesToWrite = data.size();
			uint64 bytesWritten = 0;
			while (bytesToWrite > 0) {
				DWORD bytesChunkToWrite = mpt::saturate_cast<DWORD>(bytesToWrite);
				DWORD bytesChunkWritten = 0;
				mpt::windows::CheckBOOL(ReadFile(m_hFile, data.data() + bytesWritten, bytesChunkToWrite, &bytesChunkWritten, NULL));
				bytesWritten += bytesChunkWritten;
				bytesToWrite -= bytesChunkWritten;
				if (!bytesChunkWritten) {
					throw std::runtime_error("");
					break;
				}
			}
		}
		mpt::windows::CheckBOOL(SetEndOfFile(m_hFile));
		{
			LARGE_INTEGER pos{};
			pos.QuadPart = 0;
			mpt::windows::CheckBOOL(SetFilePointerEx(m_hFile, pos, NULL, FILE_BEGIN));
		}
		if (sync) {
			mpt::windows::CheckBOOL(FlushFileBuffers(m_hFile));
		}
	}

	void unlock() {
	}

	~atomic_file_ref() {
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}

};


class atomic_shared_file_ref final
	: protected atomic_file_ref {

protected:

	std::size_t m_locks = 0;

public:

	atomic_shared_file_ref(mpt::PathString filename)
		: atomic_file_ref(std::move(filename), true)
	{
		return;
	}

	atomic_shared_file_ref(const atomic_shared_file_ref &) = delete;
	atomic_shared_file_ref & operator =(const atomic_shared_file_ref &) = delete;

	void lock() {
		if (m_locks == 0) {
			OVERLAPPED overlapped{};
			overlapped.Internal = 0;
			overlapped.InternalHigh = 0;
			overlapped.Offset = 0x00000000;
			overlapped.OffsetHigh = 0x00000000;
			overlapped.hEvent = NULL;
			mpt::windows::CheckBOOL(LockFileEx(m_hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xffffffff, 0xffffffff, &overlapped));
		}
		m_locks += 1;
	}

	std::vector<std::byte> read() {
		atomic_file_lock_guard g{*this};
		return atomic_file_ref::read();
	}

	void write(mpt::byte_span data, bool sync = false) {
		atomic_file_lock_guard g{*this};
		atomic_file_ref::write(data, sync);
	}

	void unlock() {
		assert(m_locks > 0);
		m_locks -= 1;
		if (m_locks == 0) {
			OVERLAPPED overlapped{};
			overlapped.Internal = 0;
			overlapped.InternalHigh = 0;
			overlapped.Offset = 0x00000000;
			overlapped.OffsetHigh = 0x00000000;
			overlapped.hEvent = NULL;
			mpt::windows::CheckBOOL(UnlockFileEx(m_hFile, 0, 0xffffffff, 0xffffffff, &overlapped));
		}
	}

	~atomic_shared_file_ref() {
		assert(m_locks == 0);
	}

};


#endif // MPT_OS_WINDOWS



} // namespace IO



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_IO_FILE_ATOMIC_ATOMIC_FILE_HPP
