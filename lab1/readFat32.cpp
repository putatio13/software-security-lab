// readFat32.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <Locale.h>

#define sector_size 512 // 扇区大小
BYTE sector_buffer[sector_size]; // 磁盘内容读取缓冲区
BYTE FDT[32]; // FDT缓冲区
ULONGLONG root_offset; // root_offset始终指向当前目录偏移量
DWORD cluster_list[sector_size/4]; // cluster_list始终指向当前所处位置簇链
DWORD FAT[sector_size/4];

// BPB
typedef struct BPB_
{
    BYTE bytes_per_sector[2]; // 0xB-0xC
    BYTE sectors_per_cluster; // 0xD
    BYTE reserved_sector[2]; // 0xE-0xF
    BYTE fat_number; // 0x10
    BYTE untitled1[15]; // 0x11-0x1f
    BYTE total_sectors[4]; // 0x20-0x23
    BYTE sectors_per_fat[4]; // 0x24-0x27
    BYTE untitled2[4]; // 0x28-0x2B
    BYTE BPB_RootClus[4]; //0x2C-0x2F
    BYTE untitled3[42]; // 0x30-0x59
} BPB;

// DBR
typedef struct DBR_
{
    BYTE jumpcode[3]; // 0x0-0x2 EB 58 90
    BYTE label[8]; // 0x3-0xA producer and os version
    BPB bpb; // 0xB-0x59
    BYTE bootloader[420]; // 0x5A-0x01FD
    BYTE end_sign[2]; // 0x01FE-0x01FF
} DBR;

// 使用链表存储文件夹
typedef struct path_
{
    int long_folder; // 是否是长目录项
    char* name;
    char* extension;
    struct path_* root_folder;
    struct path_* next_folder;
} path;

// 短目录项
typedef struct sFDT_
{
    BYTE filename[8]; // 0x0-0x7
    BYTE extension[3]; //0x8-0xA
    BYTE attr; // 0xB
    BYTE untitled1[8]; // 0xC-0x13
    // 大端
    BYTE cluster_high[2]; // 0x14-0x15
    BYTE untitled2[4]; // 0x16-0x19
    BYTE cluster_low[2]; // 0x1A-0x1B
    BYTE size[4]; //0x1C-0x1F
}sFDT;

// 长目录项
typedef struct lFDT_
{
    BYTE ordinal; // 0x0
    WCHAR filename1[5]; // 0x1-0xA 
    BYTE attr; // 0xB 值为0xF
    BYTE untitled1[2]; // 0xC-0xD
    WCHAR filename2[6]; // 0xE-0x19
    BYTE untitled2[2]; // 0x1A-0x1B
    WCHAR filename3[2]; // 0x1C-0x1F
}lFDT;

// 使用链表存储长目录项
typedef struct lFDT_list_
{
    lFDT attr;
    struct lFDT_list_* next;
}lFDT_list;

// BYTE数组到int
int to_int(BYTE* list, int n)
{
    n--;
    int t = 0;
    while (n >= 0)
    {
        t *= 256;
        t += list[n];
        n--;
    }
    return t;
}

// 根据文件路径计算md5
int md5sum(char* path, char* result) {
    int len = strlen(path);
    char cmd[1000] = { 0 };
    char buffer[128];                                     //定义缓冲区
    memcpy(cmd, "certutil -hashfile \"", 20);
    memcpy(cmd + 20, path, len); // 注意char转换为wchar后长度改变
    memcpy(cmd + 20 + len, "\" MD5", 5);
    FILE* pipe = _popen(cmd, "r");                  //打开管道，并执行命令 
    if (!pipe)
        return 0;                                 //返回0表示运行失败 

    if(!feof(pipe)) 
    {
        fgets(buffer, 128, pipe);
        fgets(result, 32, pipe);
        result[32] = '\0';
    }
    _pclose(pipe);                                          //关闭管道 
    return 1;                                                 //返回1表示运行成功 
}

// 解析文件路径，返回存有文件路径信息的链表
path* parse_file_folder(char* file_path)
{
    path* root_path;
    path* pre;
    path* p;
    root_path = (path*)(malloc(sizeof(path)));
    root_path->root_folder = NULL;
    root_path->long_folder = 0;
    pre = root_path;
    int i = 0;
    char space[2] = " ";
    char* test=NULL;
    while ( file_path[i] != '\0')
    {
        int ext = 0;
        int length = 0;
        if (i == 0) // 盘符
        {
            length++;
            if (file_path[1] != ':')
                    exit(1);
            i += 2;
            while (file_path[i] == '/' || file_path[i] == '\\' )
            {
                i++;
            }
            root_path->name = (char*)malloc(sizeof(char)*(length+1));
            memcpy(root_path->name, file_path, length);
            root_path->name[length] = '\0';
            root_path->extension = NULL;
            continue;
        }
        p = (path*)malloc(sizeof(path));
        p->next_folder = NULL;
        while (file_path[i] != '/' && file_path[i] != '\\'&& file_path[i] != '\0')
        {
            i++;
            length++;
            if (file_path[i] == '.')
            {
                ext = 1;
                break;
            }
        }
        p->name = (char*)malloc(sizeof(char) * (length + 1));
        memcpy(p->name, file_path + i - length , length);
        p->name[length] = '\0';
        p->extension = NULL;
        p->long_folder = strlen(p->name) > 8 ? 1 : 0;
        if (strpbrk(p->name, space) != NULL) // 文件名中有空格FAT32一律当作长文件名处理
            p->long_folder = 1;
        if (ext == 1)
        {
            i++;
            length = 0;
            while (file_path[i] != '/' && file_path[i] != '\\' && file_path[i] != '\0')
            {
                i++;
                length++;
            }
            if (file_path[i] != '\0')
                exit(1); // 文件后缀名错误
            p->extension = (char*)malloc(sizeof(char) * (length + 1));
            memcpy(p->extension, file_path + i - length, length);
            p->extension[length] = '\0';
            p->long_folder = strlen(p->extension) > 3 ? 1 : p->long_folder;
            if (strpbrk(p->extension, space) != NULL)
                p->long_folder = 1;
        }
        p->root_folder = pre;
        pre->next_folder = p;
        pre = p;
        while (file_path[i] == '/' || file_path[i] == '\\')
            i++;
    }
    return root_path;
}

// 进入下一层目录，释放当前目录
path* next_folder(path* root_path)
{
    path* p = root_path->next_folder;
    if (p == NULL)
    {
        return NULL;
    }
    free(root_path->name);
    if (root_path->extension != NULL)
        free(root_path->extension);
    free(root_path);
    return p;
}

// 打开磁盘，返回操作磁盘的句柄
HANDLE open_disk(path* root_path)
{
    HANDLE hDevice;
    TCHAR partition[] = TEXT("\\\\.\\C:");
    partition[4] += root_path->name[0] - 'C';
    hDevice = CreateFileW(partition, // 设备名称,这里指硬盘甚至可以是分区/扩展分区名，不区分大小写 
        GENERIC_READ,                // 读
        FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
        NULL,             // default security attributes
        OPEN_EXISTING,    // disposition
        0,                // file attributes
        NULL);            // do not copy file attributes
    if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
    {
        printf("权限缺失或磁盘错误\n");
        exit(1);
    }

    return hDevice;
}

// 初始化DBR并设置根偏移
DBR init_DBR(HANDLE hDevice)
{
    DBR dbr;
    BOOL bResult;
    bResult = ReadFile(hDevice, &dbr, 0x200, NULL, NULL);
    if (bResult == 0) // cannot open the drive
    {
        printf("读取DBR错误\n");
        exit(1);
    }
    root_offset = to_int(dbr.bpb.reserved_sector, 2) + dbr.bpb.fat_number * to_int(dbr.bpb.sectors_per_fat, 4);
    root_offset *= 0x200;
    return dbr;
}

// 读取指定簇号指定扇区号内容 写入 sector_buffer中
void read_cluster(HANDLE hDevice, DWORD cluster, BYTE sector, BPB bpb)
{
    BOOL bResult;
    ULONGLONG total_sector = bpb.sectors_per_cluster * (cluster - to_int(bpb.BPB_RootClus, 4)) + sector;
    LARGE_INTEGER offset;//读取位置 
    offset.QuadPart = (ULONGLONG)root_offset + total_sector * sector_size;
    SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);//从这个位置开始读，DBR是FILE_BEGIN，相对位移！！！ 
    bResult = ReadFile(hDevice, sector_buffer, sector_size, NULL, NULL); // 一次性读取一个扇区
    if (bResult == 0)
    {
        printf("读取扇区错误！");
        exit(1);
    }
    return;
}

// 读取FAT 填充簇链
void find_fat(HANDLE hDevice, BPB bpb)
{
    BOOL bResult;
    int cursor = 0;

    LARGE_INTEGER offset;//读取位置 
    DWORD pre_cluster = cluster_list[0];

    // 按照cluser_list[0]第一次读取FAT
    if (pre_cluster >= DWORD(128 * (cursor+1)))
    {
        cursor = pre_cluster / 128;
        pre_cluster %= 128;
    }
    offset.QuadPart = (ULONGLONG)(to_int(bpb.reserved_sector, 2) + cursor) * 512;
    SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);// 从这个位置开始读，DBR是FILE_BEGIN，相对位移！！！ 
    bResult = ReadFile(hDevice, FAT, sector_size, NULL, NULL); // 一次性读取128个簇号

    if (bResult == 0)
    {
        printf("FAT读取失败");
        exit(1);
    }
    int i = 0;
    while (true)
    {
        if (FAT[pre_cluster] == DWORD(0xfffffff)) // 读到结束标志， 直接写在while中会发生未知bug
            break;
        cluster_list[i + 1] = FAT[pre_cluster];
        i++;
        pre_cluster = cluster_list[i]; // pre_cluster此时为真实簇号
        if (pre_cluster == DWORD(0xfffffff))
            break;
        if (pre_cluster >= DWORD(128 * (cursor + 1)) || pre_cluster < DWORD(128 * cursor)) // pre_cluster超过了cursor范围
        {
            cursor = pre_cluster / 128;
            pre_cluster %= 128;
            offset.QuadPart = (ULONGLONG)(to_int(bpb.reserved_sector, 2) + cursor) * 512;
            SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);// 从这个位置开始读，DBR是FILE_BEGIN，相对位移！！！ 
            bResult = ReadFile(hDevice, FAT, sector_size, NULL, NULL); // 一次性读取128个簇号
            if (bResult == 0)
            {
                printf("FAT读取失败");
                exit(1);
            }
        }
        pre_cluster %= 128;
    }
    cluster_list[i + 1] = FAT[cluster_list[i]%128];
    return;
}

// 根据簇链和输出路径写入文件
char* write_file(HANDLE hDevice, char* output, path* root_path, DWORD size, BPB bpb)
{
    char* filename;
    int cursor = 0;
    int sector = bpb.sectors_per_cluster;
    int output_len = strlen(output);
    path* out_path = parse_file_folder(output);
    path* p = NULL;
    int file_len = strlen(root_path->name);
    int ext_len = strlen(root_path->name);
    int total_len = output_len + file_len + ext_len;
    filename = (char*)malloc(sizeof(char) * (total_len + 20));
    sprintf(filename + cursor, "%c:\\", out_path->name[0]);
    cursor += 4;
    out_path = next_folder(out_path);
    while (out_path != NULL)
    {
        p = out_path->next_folder;
        if (out_path->extension != NULL)
        {
            printf("输出路径错误！");
            exit(1);
        }
        int len = strlen(out_path->name);
        sprintf(filename, "%s%s\\", filename, out_path->name);
        cursor += (len+1);
        free(out_path);
        out_path = p;
    }
    sprintf(filename, "%s%s.%s", filename, root_path->name, root_path->extension);
    cursor += file_len + ext_len + 1;
    filename[cursor] = '\0';
    FILE* out = NULL;
    out = fopen(filename, "wb");
    int i = 0;// 控制簇号
    int j = 0;// 控制扇区号
    int count = 0;
    while (cluster_list[i] != DWORD(0xfffffff))
    {
        int flag = 0;
        while (j < sector)
        {
            read_cluster(hDevice, cluster_list[i], j, bpb); // 读取一个扇区的数据
            DWORD output_size = sector_size < size ? sector_size : size;
            fwrite(sector_buffer, output_size , 1, out);
            count++;
            if (size <= 512)// 文件最后一个扇区
            {
                flag = 1;
                break;
            }
			else 
            {
				size -= 512;
				j++;
			}
        }
        if (flag == 1)
            break; 
        i++;
        j = 0;
    }
    fclose(out);
    return filename;
}

// 返回长目录项链和root_path比较结果
BOOL compare_long_directory(lFDT_list* list_head, path* root_path, int found)
{
    BOOL flag = 0;

    // 合并拓展
    int file_len = strlen(root_path->name);
    int ext_len = root_path->extension == NULL ? 0 : (strlen(root_path->extension) + 1);// 加上点的长度
    int total_len = file_len + ext_len;
    char* file = (char*)malloc(sizeof(char) * (total_len + 1));
    memcpy(file, root_path->name, file_len);
    if (ext_len != 0)
    {
        file[file_len] = '.';
        memcpy(file + file_len + 1, root_path->extension, ext_len);
    }
    file[total_len] = '\0';

    // 转换目录内容为char
    char* filename = NULL;
    WCHAR* wfilename = NULL;
    
    filename = (char*)malloc(sizeof(char) * (found*13*4+1));
    wfilename = (WCHAR*)malloc(sizeof(WCHAR) * (found * 13 + 1));
    wmemset(wfilename, 0, found * 13 + 1);

    lFDT_list* pre = list_head;
    lFDT_list* p = NULL;
    int i = 0;
    while (pre != NULL)
    {
        p = pre->next;
        wmemcpy(wfilename + 13 * i, pre->attr.filename1, 5);
        wmemcpy(wfilename + 13 * i + 5, pre->attr.filename2, 6);
        wmemcpy(wfilename + 13 * i + 11, pre->attr.filename3, 2);
        i++;
        free(pre);
        pre = p; 
    }

    int len = wcstombs(filename, wfilename, (found * 13 * 4 + 1));
    if (len == total_len) // 长度相等
    {
        if (!_stricmp(file, filename))
        {
            flag = true;
        }
    }
    free(wfilename);
    free(filename);
    free(file);
    return flag;
}

// 返回短目录项和root_path比较结果
BOOL compare_directory(sFDT sfdt, path* root_path)
{
    BOOL flag = 0;
    int file_len = strlen(root_path->name);
    int ext_len = root_path->extension == NULL ? 0 : strlen(root_path->extension);
    char filename[9] = {0};
    char extension[4] = {0};
    char space[2] = " ";
    memcpy(filename, &sfdt.filename, 8);
    memcpy(extension, &sfdt.extension, 3);
    flag = filename[file_len] == '\0' || filename[file_len] == ' '; // 比较长度 fat32中短目录项文件名绝对不含空格 
    if (flag)  // 文件名长度相等
    {
        flag = !(_strnicmp(root_path->name, filename, file_len));
        if (flag) // 文件名内容相等
        {
            flag = extension[ext_len] == '\0' || extension[ext_len] == ' ';
            if (flag)  // 拓展长度相等
            {
                if (ext_len != 0) // 有拓展
                {
                    flag = !(_strnicmp(root_path->name, filename, file_len));
                }
                else
                    flag = true;
            }
        }
        else
            flag = false;
    }
    return flag;
}

// LFDT解析 解决字符对齐问题
lFDT parsing_wchar(BYTE* FDT)
{
    lFDT lfdt;
    int cursor = 0;
    lfdt.ordinal = FDT[cursor];
    cursor++;
    for (int i = 0; i < 5; i++)
    {
        lfdt.filename1[i] = FDT[cursor] + FDT[cursor+1] * 256;
        cursor += 2;
    }
    lfdt.attr = FDT[cursor];
    cursor++;
    for (int i = 0; i < 2; i++)
    {
        lfdt.untitled1[i] = FDT[cursor];
        cursor++;
    }
    for (int i = 0; i < 6; i++)
    {
        lfdt.filename2[i] = FDT[cursor] + FDT[cursor+1] * 256;
        cursor += 2;
    }
    for (int i = 0; i < 2; i++)
    {
        lfdt.untitled2[i] = FDT[cursor];
        cursor++;
    }
    for (int i = 0; i < 2; i++)
    {
        lfdt.filename3[i] = FDT[cursor] + FDT[cursor+1] * 256;
        cursor += 2;
    }
    return lfdt;
}

// 返回root_path对应目录项对应的cluster
sFDT search_directory(HANDLE hDevice, path* root_path, BPB bpb)
{
    int long_folder = root_path->long_folder;
    int i = 0; //簇号控制变量
    int j = 0; //扇区号控制变量
    int k = 0; // FDT控制变量
    BOOL flag = false;// 判断是否找到目录项
    BOOL found = false; //判断是否找到长目录项
    lFDT_list* list_head = NULL;
    sFDT sfdt;
    DWORD result = 0;
    int sectors = bpb.sectors_per_cluster;
    int count = 0;
    while (cluster_list[i] != 0x0fffffff)
    {
        while (j != sectors)
        {
            if (cluster_list[i] == DWORD(0xfffffff))
            {
                printf("该文件不存在！");
                exit(1);
            }
            k = 0;
            count++;
            read_cluster(hDevice, cluster_list[i], j, bpb);
            while (k != 16)
            {
                memcpy(FDT, sector_buffer + k * 0x20, 0x20);
                if (long_folder == 1) // 现在找的是长目录
                {
                    if (found == 0) // 还没找到第一长目录
                    {
                        if (FDT[0xB] == 0xF) // 找到第一个长目录
                        {
                            found = 1;
                            list_head = (lFDT_list*)(malloc(sizeof(lFDT_list)));
                            list_head->next = NULL;
                            list_head->attr = parsing_wchar(FDT);
                        }
                    }
                    else
                    {
                        if (FDT[0xB] != 0xF) // 找到长目录对应的短目录项
                        {
                            memcpy(&sfdt, FDT, 0x20);
                            flag = compare_long_directory(list_head, root_path, found); // 是否找到
                            if (flag == 1)
                                break;
                            found = 0;
                        }
                        else // 找到下一个长目录
                        {
                            lFDT_list* p;
                            p = (lFDT_list*)(malloc(sizeof(lFDT_list)));
                            p->attr = parsing_wchar(FDT);
                            p->next = list_head;
                            list_head = p; // 头插法
                            found++;
                        }
                    }
                }
                else // 现在找的是短目录
                {
                    if (FDT[0xB] != 0xF)
                    {
                        memcpy(&sfdt, FDT, 0x20);
                        flag = compare_directory(sfdt, root_path);
                    }
                }
                k++;
            }
            if (flag == 1)
                break;
            j++;
        }
        j = 0;
        if (flag == 1)
            break;
        i++;
    }
    if (flag == 0)
    {
        printf("找不到文件！");
        exit(1);
    }
    return sfdt;
}

// 比较文件md5
BOOL compare_file(char* file, char* out_file)
{
    char md5[33] = { 0 };
    char new_md5[33] = { 0 };
    int t = md5sum(file, md5);
    while (!t)
    {
        t = md5sum(file, md5);
    }
    t = md5sum(out_file, new_md5);
    while (!t)
    {
        t = md5sum(file, md5);
    }
    printf("\n旧文件md5为：%s\n", md5);
    printf("新文件md5为：%s\n", new_md5);
    return strncmp(md5, new_md5, 32)==0;
}

// 复制文件到指定路径
BOOL copy_file(char* file, char* output)
{
    path* root_path = NULL;
    root_path = parse_file_folder(file);
    BOOL flag = 0;
    HANDLE hDevice;
    hDevice = open_disk(root_path);
    DBR dbr;
    dbr = init_DBR(hDevice);
    root_path = next_folder(root_path);
    DWORD cluster = 2;
    char* out_file = NULL;
    sFDT sfdt;
    while (root_path != NULL)
    {
        // root_path 指向目前正在寻找的目录项
        // 旧cluster指向目前正在寻找的目录项所在的目录位置
        // 新cluster指向目前正在寻找的目录项位置
        cluster_list[0] = cluster;
        find_fat(hDevice, dbr.bpb); // 考虑文件夹跨簇情况 
        sfdt = search_directory(hDevice, root_path, dbr.bpb); // 返回文件位置首簇号
        cluster = to_int(sfdt.cluster_high, 2) * 65536 + to_int(sfdt.cluster_low, 2);
        if (root_path->extension == NULL) // 当前仍在目录中
        {
            root_path = next_folder(root_path);
        }
        else // 已找到文件
        {
            printf("文件短目录项信息如下:\n");
            printf("文件名:%s 后缀名:%s 属性:%x \n", sfdt.filename, sfdt.extension, sfdt.attr);
            printf("簇号高十六位：%x 簇号低十六位：%x\n", to_int(sfdt.cluster_high,2), to_int(sfdt.cluster_low,2));
            printf("文件大小:%x\n", to_int(sfdt.size,4));
            
            // 完善簇链 
            cluster_list[0] = cluster;
            find_fat(hDevice, dbr.bpb);
            printf("文件簇链:\n");
            for (int i = 0; cluster_list[i] != DWORD(0x0fffffff); i++)
            {
                printf("第%d项：%x ", i + 1, cluster_list[i]);
                if ((i+1) % 5 == 0)
                    printf("\n");
            }
            DWORD size = to_int(sfdt.size, 4);
            out_file = write_file(hDevice, output, root_path, size, dbr.bpb);
            root_path = next_folder(root_path);
            flag = 1;
        }
    }
    

    if (compare_file(file, out_file) == 0)
    {
        flag = 0;
    }
    return flag;
}

int main(int argc, char ** argv)
{
    setlocale(LC_ALL, "chs");
    char file[3000];
    char output[3000];
    if (argc != 3)
    {
        printf("请输入源文件路径(要求文件有后缀名)：");
        fflush(stdin);
        fgets(file, 3000, stdin);
        int t = strlen(file);
        if (file[t-1] == '\n')
        {
            file[t - 1] = '\0';
        }
        printf("请输入目标路径(要求文件夹已存在)：");
        fflush(stdin);
        fgets(output, 3000, stdin);
        t = strlen(output);
        if (output[t - 1] == '\n')
        {
            output[t - 1] = '\0';
        }
    }
    else
    {
        memcpy(file, argv[1], strlen(argv[1]) + 1);
        memcpy(output, argv[2], strlen(argv[2]) + 1);
    }
    char md5[33] = { 0 };
    if (copy_file(file, output))
        printf("文件复制成功!");
    else
    {
        printf("文件复制失败！");
    }
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
