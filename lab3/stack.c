#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
int main(int argc, const char* argv[]) {
    HANDLE hFile;
    CHAR buffer[512] = {};
    DWORD read, write;

    hFile = CreateFile(TEXT(".\\hack"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ReadFile(hFile, buffer, 512, &read, NULL);

    HINSTANCE LibHandle;
    char* ProcAdd;
    LibHandle = LoadLibrary(L"user32");
    ProcAdd = (char *)GetProcAddress(LibHandle, "MessageBoxA");
    unsigned int k = (unsigned int)ProcAdd;

    UINT8 insert[4] = { 0 };
    int t = 0xff;
    for (int i = 0; i < 4; i++) {
        insert[i] = t & k;
        k = k >> 8;
    }
    SetFilePointer(hFile, 309, NULL, FILE_BEGIN);
    if(WriteFile(hFile, insert, 4, &write, NULL))
        printf("已修改MessageBoxA函数地址\n");
    LibHandle = LoadLibrary(L"kernel32");
    ProcAdd = (char*)GetProcAddress(LibHandle, "ExitProcess");
    k = (unsigned int)ProcAdd;
    for (int i = 0; i < 4; i++) {
        insert[i] = t & k;
        k = k >> 8;
    }
    SetFilePointer(hFile, 316, NULL, FILE_BEGIN);
    if (WriteFile(hFile, insert, 4, &write, NULL))
        printf("已修改ExitProcess函数地址\n");
    CloseHandle(hFile);
    system("pause");
	return 0;
}