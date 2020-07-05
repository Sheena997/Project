#pragma once

#include <iostream>
#include <fstream> //文件操作
#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#include <io.h>

#define SHARED_PATH "./Shared/"
#define DOWNLOAD_PATH "./Download/"
#ifdef _WIN32
//Windows头文件
#include <WS2tcpip.h> //Windows Socket协议
#include <Iphlpapi.h> //获取网卡信息接口
#include <sstream>
#pragma  comment(lib, "Iphlpapi.lib") //获取网卡信息接口的库文件，导入库
#pragma comment(lib, "ws2_32.lib") //Windows下socket库
#else
//Linux头文件
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif


class StringUtil
{
public:
	static int64_t Str2Dig(const std::string &num)
	{
		std::stringstream tmp;
		tmp << num;
		int64_t res;
		tmp >> res;

		return res;
	}
};
//文件工具
class FileUtil
{
public:
	static bool Write(const std::string& name, const std::string &body, int64_t offset = 0)
	{
		
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "ab+");//以二进制方式打开文件追加写入数据
		if (fp == NULL)
		{
			std::cerr << "打开文件失败！\n";
			return false;
		}

		fseek(fp, offset, SEEK_SET);
		int ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size())
		{
			std::cerr << "向文件写入数据失败！\n";
			fclose(fp);
			return false;
		}
		else
		{
			std::cout << "向文件写入数据成功！\n";
		}
		fclose(fp);
		return true;
	}
	//指针参数表示这是一个输出型参数
	//const & 表示这是一个输入型参数
	//& 表示这是一个输入输出型参数
	static bool Read(const std::string &name, std::string *body)
	{
		uint64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "读取文件数据:" << name << "size:" << filesize << "\n";
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件数据失败\n";
			return false;
		}
		size_t ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "读取文件失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}

	static bool ReadRange(const std::string &name, std::string  *body, int len, int offset)
	{
		body->resize(len);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件数据失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		size_t ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len)
		{
			std::cerr << "读取文件失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}


	static int64_t GetFileSize(const std::string& name)
	{
		return boost::filesystem::file_size(name);
	}

	

	//获取分块下载
	static bool GetRange(std::string& range, int64_t &range_start, int64_t &range_end)
	{
		size_t pos1, pos2;
		pos1 = range.find("-");
		if (pos1 == std::string::npos)
			return false;

		pos2 = range.find("bytes=");
		if (pos2 == std::string::npos)
			return false;

		std::stringstream tmp;
		tmp << range.substr(pos2 + 6, pos1 - pos2 - 6);
		tmp >> range_start;
		tmp.clear();
		tmp << range.substr(pos1 + 1);
		tmp >> range_end;

		return true;
	}


};
class Adapter
{
public:
	uint32_t _ip_addr; //网卡上的IP地址
	uint32_t _mask_addr; //网卡上的子网掩码
};

class AdapterUtil
{
public:
#ifdef _WIN32
	//Windows 下的获取网卡信息实现
	static bool GetAllAdapter(std::vector<Adapter>* list)
	{
		//先有一个结构体保存网卡信息  
		//PIP_ADAPTER_INFO是Windows下存储网卡信息的结构体
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();

		//GetAdaptersInfo()是Windows下获取网卡信息的接口 ---- 网卡信息有多个，因此传入一个指针
		//因为空间不足，因此有一个输出型参数(all_adapter_size)，用于向用具返回所有网卡信息空间
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);
		//all_adapter_size用于获取实际所有网卡信息所占用的空间大小
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);

		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			//当前缓冲区溢出了，空间不足
			//因此重新给指针申请空间
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			//重新获取网卡信息
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		}

		while (p_adapters)
		{
			Adapter adapter;
			//inet_pton(int family, char* string, void* buf);将一个字符串点分十进制的IP地址转换为网络字节序
			//family：AF_INET(IPv4地址域) 、AF_INET6(IPv6地址域)
			//string：字符串点分十进制的IP地址
			//buf：一块缓冲区，用于接收转换后的网络字节序IP地址
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)
			{
				//因为有些网卡并没有启用，导致IP地址为0，所以IP地址为0的网卡信息无需获取
				list->push_back(adapter);//将网卡信息添加到vector中，返回给用户
				std::cout << "网卡名称：" << p_adapters->AdapterName << std::endl;
				std::cout << "网卡描述：" << p_adapters->Description << std::endl;
				std::cout << "IP地址：" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "子网掩码：" << p_adapters->IpAddressList.IpMask.String << std::endl;
				std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
#else
	//linux下的获取网卡信息实现
	static bool GetAllAdapter(std::vector<Adapter> *list) {
		struct ifaddrs *addrs = NULL;
		uint32_t net_seg, end_host;
		getifaddrs(&addrs);
		while (addrs) {
			if (addrs->ifa_addr->sa_family == AF_INET && strncasecmp(addrs->ifa_name, "lo", 2)) {
				sockaddr_in *addr = (sockaddr_in*)addrs->ifa_addr;
				sockaddr_in *mask = (sockaddr_in*)addrs->ifa_netmask;
				Adapter adapter;
				adapter._ip_addr = addr->sin_addr.s_addr;
				adapter._mask_addr = mask->sin_addr.s_addr;
				list->push_back(adapter);
			}
			addrs = addrs->ifa_next;
		}
		return true;
}
#endif
};
