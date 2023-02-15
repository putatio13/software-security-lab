	add esp, 55h
	jmp esp
	
	/*
		数据区
		"ExitProcess 00 00 00 00 00";-76
		"MessageBoxA 00 00 00 00 00";-60
		"LoadLibraryExA 00 00";-44
		"user32.dll 00 00 00 00 00 00";-28
		" 00 00 00 00";-12 // 顺序依次为kernel32、ExitProcess
		" 00 00 00 00";-8 // 存放的是GetProcAddressA
		" 00 00 00 00";-4 // 顺序依次为LoadLibraryExA、user32.dll、MessageBoxA
	*/
	
	call A
A:
	pop edi
	sub edi, 14h

	// 获取kernel32
	mov eax, fs: [30h]
	mov eax, [eax + 0ch]
	mov eax, [eax + 1ch]
	mov eax, [eax]
	mov eax, [eax]
	mov eax, [eax + 8h]
	mov [edi - 12], eax
	
	// 找到导出表位置
	push edi
	mov edi, eax
	mov eax, [edi + 3Ch]
	mov edx, [edi + eax + 78h]
	add edx, edi
	
	// 设置循环次数
	mov ecx, [edx + 14h]
	mov ebx, [edx + 20h]
	add ebx, edi
search :
	dec ecx
	mov esi, [ebx + ecx * 4]
	add esi, edi; 依次找每个函数名称
		; GetProcAddress
	mov eax, 0x50746547
	cmp[esi], eax; 'PteG'
	jne search
	mov eax, 0x41636f72
	cmp[esi + 4], eax; 'Acor'
	jne search
		; 如果是GetProcA，表示找到了
	mov ebx, [edx + 24h]
	add ebx, edi; ebx = 序号数组地址, AddressOf
	mov cx, [ebx + ecx * 2]; ecx = 计算出的序号值
	mov ebx, [edx + 1Ch]
	add ebx, edi; ebx＝函数地址的起始位置，AddressOfFunction
	mov eax, [ebx + ecx * 4]
	add eax, edi; 利用序号值，得到出GetProcAddress的地址
	pop edi
	mov ebx, edi; 将begin的地址传给ebx
	mov [ebx - 8], eax


	// 获得LoadLibraryExA
	sub ebx, 44
	push ebx; 传入的参数是要用的函数名字LoadLibraryExA
	add ebx, 44
	push [ebx - 12]; 再次传入一个参数给函数，参数为kernel32.dll的imagebase，就是DLL模块句柄
	call [ebx - 8]; 调用函数GetProcAddress得到函数LoadLibrary的地址
	mov [ebx - 4], eax

	push 0x00000010; 传入参数使用函数LoadLibraryExA
	push 0x00000000; 调用函数是用来调用其他的dll

	// 获得user32
	sub ebx, 28
	push ebx
	add ebx, 28
	call [ebx - 4]
	mov [ebx - 4], eax


	// 获得MessageBoxA
	sub ebx, 60
	push ebx
	add ebx, 60
	push [ebx - 4]
	call [ebx - 8]
	mov [ebx - 4], eax
	
	// 获得ExitProcess
	sub ebx, 76
	push ebx
	add ebx, 76
	push [ebx - 12]
	call [ebx - 8]
	mov [ebx - 12], eax
	
	xor ebx, ebx	// 将 ebx 清0
	push ebx
	push 0x2020206f	//    o
	push 0x6c6c6568 // lleh  push一个hello当做标题，push的时候不够了要用0x20填充为空
	mov eax, esp	// 把标题 hello 赋值给 eax
	push ebx		// ebx 压栈，ebp为0，作用是将两个连续的hello断开，因为下面又要压一个hello作为弹框内容
	push 0x2020206f	// 再push一个hello当做内容
	push 0x6c6c6568
	mov ecx, esp		// 把内容 hello 赋值给 ecx，esp 指向了当前的栈指针地址，所以可以赋值

	// 下面就是将MessageBox的参数压栈
	push ebx		// messageBox 第四个参数
	push eax		// messageBox 第三个参数
	push ecx		// messageBox 第二个参数
	push ebx		// messageBox 第一个参数

	mov eax, [edi - 4]	// messageBox 函数地址赋值给 eax
	call eax			// 调用 messageBox
	
	mov eax, [edi - 12]; 8b 47 f4
	call eax;ff do
	
	e8 00 00 00 00
	5f
	83 ef 14
	81 ec 00 01 00 00
	64 a1 30 00 00 00
	8b 40 0c
	8b 40 1c
	8b 00
	8b 00
	8b 40 08
	89 47 f4
	57
	89 c7
	8b 47 3c
	8b 54 07 78
	01 fa
	8b 4a 14
	8b 5a 20
	01 fb
	49
	8b 34 8b
	01 fe
	b8 47 65 74 50
	39 06
	75 f1
	b8 72 6f 63 41
	39 46 04
	75 e7
	8b 5a 24
	01 fb
	66 8b 0c 4b
	8b 5a 1c
	01 fb
	8b 04 8b
	01 f8
	5f
	89 fb
	89 43 f8
	83 eb 2c
	53
	83 c3 2c
	ff 73 f4
	ff 53 f8
	89 43 fc
	6a 10
	6a 00
	
	83 eb 1c
	53
	83 c3 1c
	ff 53 fc
	89 43 fc
	
	83 eb 3c
	53
	83 c3 3c
	ff 73 fc
	ff 53 f8
	89 43 fc	
	// 开始修改
	83 eb 4c
	53
	83 c3 4c
	ff 73 f4
	ff 53 f8
	89 43 f4

	
