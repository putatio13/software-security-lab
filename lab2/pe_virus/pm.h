#pragma once
#define IS_NOT_INFECTED 0
#define FILE_FAILURE 1
#define IS_INFECTED 2
#define IMAGE_INFECTED_SINGNATURE 0x06060606

typedef struct PE_
{
	HANDLE hFile;
	int status;

	// DOS Header
	LONG e_lfanew;

	// Optional Header
	DWORD AddressOfEntry;
	DWORD ImageBase;
	DWORD FileAlignment;
	DWORD SectionAlignment;
	DWORD SizeOfImage;

	_IMAGE_SECTION_HEADER* section_headers;
	int number_of_sections;
} PE;

typedef struct NEW_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InIntializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    PVOID SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    PVOID Reserved5[3];
#pragma warning(push)
#pragma warning(disable: 4201) // we'll always use the Microsoft compiler
    union {
        ULONG CheckSum;
        PVOID Reserved6;
    } DUMMYUNIONNAME;
#pragma warning(pop)
    ULONG TimeDateStamp;
} NEWLDR_DATA_TABLE_ENTRY, * NEWPLDR_DATA_TABLE_ENTRY; // 原始的结构体没有BaseDllName这一项， 需要重新定义

#define _LDR_DATA_TABLE_ENTRY NEW_LDR_DATA_TABLE_ENTRY //重定义LDR
#define LDR_DATA_TABLE_ENTRY NEWLDR_DATA_TABLE_ENTRY
#define PLDR_DATA_TABLE_ENTRY NEWPLDR_DATA_TABLE_ENTRY