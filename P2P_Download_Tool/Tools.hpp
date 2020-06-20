#pragma once

#include <iostream>
#include <fstream> //�ļ�����
#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#ifdef _WIN32
//Windowsͷ�ļ�
#include <WS2tcpip.h> //Windows SocketЭ��
#include <Iphlpapi.h> //��ȡ������Ϣ�ӿ�
#include <sstream>
#pragma  comment(lib, "Iphlpapi.lib") //��ȡ������Ϣ�ӿڵĿ��ļ��������
#pragma comment(lib, "ws2_32.lib") //Windows��socket��
#else
//Linuxͷ�ļ�
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
//�ļ�����
class FileUtil
{
public:
	static bool Write(const std::string& name, const std::string &body, int64_t offset = 0)
	{
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "wb+");//�Զ����Ʒ�ʽ���ļ�д������
		if (fp == NULL)
		{
			std::cerr << "���ļ�ʧ�ܣ�\n";
			return false;
		}

		fseek(fp, offset, SEEK_SET);
		int ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size())
		{
			std::cerr << "���ļ�д������ʧ�ܣ�\n";
			fclose(fp);
			return false;
		}

		fclose(fp);
		return true;
	}

	//ָ�������ʾ����һ������Ͳ�����
	//const &��ʾ����һ�������Ͳ���
	//&��ʾ����һ����������Ͳ���
	//��ȡ�ļ��е���������
	static bool Read(const std::string &name, std::string *body)
	{
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "��ȡ�ļ����ݣ�" << name << " ��size��" << filesize << std::endl;
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}

		int ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "���ļ���ȡ����ʧ�ܣ�\n";
			fclose(fp);
			return false;
		}
		fclose(fp);

		return true;
	}

	//��name�ļ���offsetλ�ÿ�ʼ��ȡlen���ȵ��ļ����ݷŵ�body��
	static bool ReadRange(const std::string &name, std::string *body, int64_t len, int64_t offset)
	{
		body->resize(len);
		std::cout << "��ȡ�ļ����ݣ�" << std::endl;
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);//��ת��ָ��λ��
		int ret = fread(&(*body)[0], 1, len, fp);//��ȡָ�����ȵ�body��
		if (ret != len)
		{
			std::cerr << "���ļ��ж�ȡ����ʧ��\n";
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

	//��ȡ�ֿ�����
	static bool GetRange(const std::string& range, int* pstart, int* pend)
	{
		size_t pos1 = range.find('-'), pos2 = range.find('=');
		*pend = std::atol(range.substr(pos1 + 1).c_str());

		*pstart = std::atol(range.substr(pos2 + 1, pos1 - pos2 - 1).c_str());

		return true;
	}

};
class Adapter
{
public:
	uint32_t _ip_addr; //�����ϵ�IP��ַ
	uint32_t _mask_addr; //�����ϵ���������
};

class AdapterUtil
{
public:
#ifdef _WIN32
	//Windows �µĻ�ȡ������Ϣʵ��
	static bool GetAllAdapter(std::vector<Adapter>* list)
	{
		//����һ���ṹ�屣��������Ϣ  
		//PIP_ADAPTER_INFO��Windows�´洢������Ϣ�Ľṹ��
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();

		//GetAdaptersInfo()��Windows�»�ȡ������Ϣ�Ľӿ� ---- ������Ϣ�ж������˴���һ��ָ��
		//��Ϊ�ռ䲻�㣬�����һ������Ͳ���(all_adapter_size)���������þ߷�������������Ϣ�ռ�
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);
		//all_adapter_size���ڻ�ȡʵ������������Ϣ��ռ�õĿռ��С
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);

		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			//��ǰ����������ˣ��ռ䲻��
			//������¸�ָ������ռ�
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			//���»�ȡ������Ϣ
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		}

		while (p_adapters)
		{
			Adapter adapter;
			//inet_pton(int family, char* string, void* buf);��һ���ַ������ʮ���Ƶ�IP��ַת��Ϊ�����ֽ���
			//family��AF_INET(IPv4��ַ��) ��AF_INET6(IPv6��ַ��)
			//string���ַ������ʮ���Ƶ�IP��ַ
			//buf��һ�黺���������ڽ���ת����������ֽ���IP��ַ
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)
			{
				//��Ϊ��Щ������û�����ã�����IP��ַΪ0������IP��ַΪ0��������Ϣ�����ȡ
				list->push_back(adapter);//��������Ϣ��ӵ�vector�У����ظ��û�
				std::cout << "�������ƣ�" << p_adapters->AdapterName << std::endl;
				std::cout << "����������" << p_adapters->Description << std::endl;
				std::cout << "IP��ַ��" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "�������룺" << p_adapters->IpAddressList.IpMask.String << std::endl;
				std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
#else
	// Linux �µĻ�ȡ������Ϣʵ��
	static bool GetAllAdapter(std::vector<Adapter>* list)
	{}
#endif
};