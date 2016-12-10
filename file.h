#pragma once

#include <Windows.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <assert.h>
#include <tchar.h>
#include <vector>
#include "buffer.h"

struct File {

	// =========================
	// <de>construct

	inline File(
		_In_ LPCSTR lpFileName,
		_In_ HANDLE handle
	) : h(handle) {
		
		const char* name = strrchr(lpFileName, '\\');
		if (!name) name = lpFileName;
		else ++name;

		this->name.resize(strlen(name));
		this->name.resize(MultiByteToWideChar(CP_UTF8, 0, lpFileName, this->name.size(), this->name.data(), this->name.size()));

		if (h == INVALID_HANDLE_VALUE) _throw(TEXT("Failed to access"), ERROR_FILE_NOT_FOUND);
	}

	inline File(
		_In_ LPCWSTR lpFileName,
		_In_ HANDLE  handle
	) : h(handle) {
		
		const wchar_t* name = wcsrchr(lpFileName, '\\');
		if (!name) name = lpFileName;
		else ++name;

		this->name.resize(wcslen(name));
		wmemcpy(this->name.data(), name, this->name.size());

		if (h == INVALID_HANDLE_VALUE) _throw(TEXT("Failed to access"));
	}

	inline File(
		_In_     LPCWSTR               lpFileName,
		_In_     DWORD                 dwDesiredAccess,
		_In_     DWORD                 dwShareMode,
		_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		_In_     DWORD                 dwCreationDisposition,
		_In_     DWORD                 dwFlagsAndAttributes,
		_In_opt_ HANDLE                hTemplateFile
	) : File(lpFileName, CreateFileW(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile)) { }

	inline File(
		_In_     LPCSTR                lpFileName,
		_In_     DWORD                 dwDesiredAccess,
		_In_     DWORD                 dwShareMode,
		_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		_In_     DWORD                 dwCreationDisposition,
		_In_     DWORD                 dwFlagsAndAttributes,
		_In_opt_ HANDLE                hTemplateFile
	) : File(lpFileName, CreateFileA(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile)) { }

	static File *tryOpen(
		_In_     LPCSTR                lpFileName,
		_In_     DWORD                 dwDesiredAccess,
		_In_     DWORD                 dwShareMode,
		_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		_In_     DWORD                 dwCreationDisposition,
		_In_     DWORD                 dwFlagsAndAttributes,
		_In_opt_ HANDLE                hTemplateFile
	) {
		HANDLE hFile = CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		if (hFile != INVALID_HANDLE_VALUE) return new File(lpFileName, hFile);
		return NULL;
	}

	static File *tryOpen(
		_In_     LPCWSTR               lpFileName,
		_In_     DWORD                 dwDesiredAccess,
		_In_     DWORD                 dwShareMode,
		_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		_In_     DWORD                 dwCreationDisposition,
		_In_     DWORD                 dwFlagsAndAttributes,
		_In_opt_ HANDLE                hTemplateFile
	) {
		HANDLE hFile = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		if (hFile != INVALID_HANDLE_VALUE) return new File(lpFileName, hFile);
		return NULL;
	}

	inline ~File(
	) {
		CloseHandle(h);
		OutputDebugStringA("asd\n");
	}

	// =========================
	// implicit conversation into handle

	inline operator HANDLE(
	) {
		return h;
	}

	// =========================
	// attributes

	inline DWORD getSize(
		_In_opt_ LPDWORD lpFileSizeHigh
	) {
		DWORD dwSize = GetFileSize(h, lpFileSizeHigh);
		if (dwSize == INVALID_FILE_SIZE) _throw(TEXT("Unable to get size of"));
		return dwSize;
	}

	inline ULONGLONG getSize(
	) {
		ULARGE_INTEGER uliSize;
		uliSize.LowPart = GetFileSize(h, &uliSize.HighPart);
		if (uliSize.LowPart == INVALID_FILE_SIZE) _throw(TEXT("Unable to get size of"));
		return uliSize.QuadPart;
	}

	// =========================
	// read

	inline DWORD read(
		_Out_       LPVOID       lpBuffer,
		_In_        DWORD        nNumberOfBytesToRead,
		_Inout_opt_ LPOVERLAPPED lpOverlapped = NULL
	) {
		static DWORD dwRead;
		if (!ReadFile(h, lpBuffer, nNumberOfBytesToRead, &dwRead, lpOverlapped)) {
			_throw(TEXT("Failed to read"));
		}
		return dwRead;
	}

	/* null terminated */
	inline void read(
		_In_        std::vector<char> &buffer,
		_In_        DWORD             nNumberOfBytesToRead,
		_Inout_opt_ LPOVERLAPPED      lpOverlapped = NULL
	) {
		assert(nNumberOfBytesToRead <= getSize(NULL));
		buffer.resize(nNumberOfBytesToRead + 1);
		DWORD dwRead = read(buffer.data(), nNumberOfBytesToRead, lpOverlapped);
		buffer.resize(dwRead + 1);
		buffer.at(dwRead) = 0;
	}

	// =========================
	// write

	inline DWORD write(
		_In_        LPCVOID      lpBuffer,
		_In_        DWORD        nNumberOfBytesToWrite,
		_Inout_opt_ LPOVERLAPPED lpOverlapped = NULL
	) {
		static DWORD dwWritten;
		if (!WriteFile(h, lpBuffer, nNumberOfBytesToWrite, &dwWritten, lpOverlapped)) {
			_throw(TEXT("Failed to write to"));
		}
		return dwWritten;
	}

	inline DWORD write(
		_In_        const std::string &buffer,
		_Inout_opt_ LPOVERLAPPED      lpOverlapped = NULL
	) {
		return write(buffer.c_str(), buffer.size(), lpOverlapped);
	}

	inline DWORD write(
		_In_        const std::vector<char> &buffer,
		_Inout_opt_ LPOVERLAPPED            lpOverlapped = NULL
	) {
		return write(buffer.data(), buffer.size(), lpOverlapped);
	}

	inline void write(
		_In_        HANDLE       hSource,
		_Inout_opt_ LPOVERLAPPED lpOverlapped = NULL
	) {
		DWORD dwRead;
		do {
			if (!ReadFile(hSource, BUFFERA, BUFFERA_SIZE, &dwRead, lpOverlapped)) {
				_throw(TEXT("Failed to read from source file when updating"));
			}
		} while (write(BUFFERA, dwRead, lpOverlapped) == BUFFERA_SIZE);
	}

	// =========================
	// pointer position

	inline DWORD setPointer(
		_In_     const LONG  lDistanceToMove,
		_In_opt_ const PLONG lpDistanceToMoveHigh,
		_In_     const DWORD dwMoveMethod
	) {
		DWORD dwLen = SetFilePointer(h, 0, lpDistanceToMoveHigh, dwMoveMethod);
		if (dwLen == INVALID_SET_FILE_POINTER) {
			_throw(TEXT("Unable to set file pointer of"));
		}
		return dwLen;
	}

	inline LONGLONG setPointer(
		_In_ const LONGLONG llDistanceToMove,
		_In_ const DWORD    dwMoveMethod
	) {
		LARGE_INTEGER li;
		li.QuadPart = llDistanceToMove;
		li.LowPart = SetFilePointer(h, li.LowPart, &li.HighPart, dwMoveMethod);
		if (li.LowPart == INVALID_SET_FILE_POINTER) {
			_throw(TEXT("Unable to set file pointer of"));
		}
		return li.QuadPart;
	}

	inline File& at(
		_In_     const LONG  lDistanceFromBegin,
		_In_opt_ const PLONG lpDistanceFromBeginHigh
	) {
		setPointer(lDistanceFromBegin, lpDistanceFromBeginHigh, FILE_BEGIN);
		return *this;
	}

	inline File& at(
		_In_ const LONGLONG llPosition
	) {
		setPointer(llPosition, FILE_BEGIN);
		return *this;
	}

	// =========================
	// miscellaneous

	inline void shift(
		_In_     LONG         lOffset,
		_In_opt_ LPOVERLAPPED lpOverlapped = NULL
	) {
		if (lOffset >= 0) {
			LONGLONG i = setPointer(0LL, FILE_CURRENT);
			LONGLONG j = setPointer(0LL, FILE_END);

			while ((j -= BUFFERA_SIZE) > i) {
				setPointer(j, FILE_BEGIN);
				read(BUFFERA, BUFFERA_SIZE, lpOverlapped);
				setPointer(j + lOffset, FILE_BEGIN);
				write(BUFFERA, BUFFERA_SIZE, lpOverlapped);
			}

			j += BUFFERA_SIZE - i; // remainder = j - i
			setPointer(i, FILE_BEGIN);
			read(BUFFERA, (DWORD) j, lpOverlapped);
			setPointer(i + lOffset, FILE_BEGIN);
			write(BUFFERA, (DWORD) j, lpOverlapped);
		} else {
			DWORD dwRead;
			for (;;) {
				dwRead = read(BUFFERA, BUFFERA_SIZE, lpOverlapped);
				setPointer((LONG) (lOffset - dwRead), FILE_CURRENT);
				if (write(BUFFERA, dwRead, lpOverlapped) != BUFFERA_SIZE) break;
				setPointer(-lOffset, FILE_CURRENT);
			}
			SetEndOfFile(h);
		}
	}

	inline std::wstring getName() {
		return std::wstring(name.data(), name.size());
	}

private:

	// =========================
	// exceptions, throw <TCHAR> string

	void _throw(LPCTSTR foremsg, DWORD dwErrorID) {
		std::vector<TCHAR> out;
		size_t forelen = _tcslen(foremsg) + 1;
		out.resize(forelen + name.size() + 102 * sizeof(TCHAR));
		out[forelen - 1] = ' ';
		memcpy(out.data(), foremsg, (forelen - 1) * sizeof(TCHAR));
		TCHAR *base = out.data() + forelen;
		for (int i = name.size(); i--;) {
			base[i] = name[i];
		}
		if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwErrorID,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), base + name.size() + 2, 100, NULL)) {
			base[name.size()] = ':';
			base[name.size() + 1] = ' ';
		} else {
			base[name.size()] = '.';
			base[name.size() + 1] = 0;
		}
#ifdef UNICODE
		throw std::wstring(out.data());
#else
		throw std::string(out.data());
#endif
	}
	
	void _throw(LPCTSTR foremsg) { _throw(foremsg, GetLastError()); }

	// =========================
	// members

	HANDLE h;
	std::vector<wchar_t> name;

};
