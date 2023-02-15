# 编译说明（括号中为项目设置中位置）
* 编译配置：Release X86；
* 编译器设置（C/C++）：禁用所有优化（优化），禁用安全检查（代码生成），关闭符合模式（语言），关闭移除未引用的代码和数据（语言）；
* 链接器设置：关闭引用（优化），启用COMDAT折叠（优化）。
# 测试说明
* 测试环境：Windows 11 22H2 Beta Preview
* 测试步骤：
	1. 测试Infect.exe感染功能。
		将test.txt，test1.exe、infect.exe放入同一文件夹中，运行infect.exe。
		test1.exe文件明显变大，成功被感染。
	2. 测试shellcode二次感染和复制文件功能。
		再将test2.exe移入文件夹中，运行test1.exe。
		test.txt被成功复制到virus文件夹下，infect.exe，test2.exe文件明显变大，成功被感染。
	3. 测试二次感染后shellcode功能以及避免重复感染功能。
		删掉virus文件夹，运行test2.exe。
		test.txt被成功复制到virus文件夹下，infect.exe，test1.exe文件大小不变，没有重复感染

