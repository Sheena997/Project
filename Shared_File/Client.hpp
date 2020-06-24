#pragma once
#include "Tools.hpp"
#include <thread>
#include "httplib.h"

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define SHARED_PATH "./Shared/"
#define DOWNLOAD_PATH "./Download/"
//1����10λ��1K������20λ��1M
//#define MAX_RANGE 100 << 20
#define MAX_RANGE (5)


/*
һ����ȡ��������
    1.��ȡ������Ϣ�����õ�������������IP��ַ�б�
	2.�����IP��ַ�б��е����������������Ϊ������ʱ�䣬ʹ�ö��̣߳�
	3.�ȴ������߳����������ϣ��ж���Խ��������Գɹ������ʾ��Ӧ����Ϊ��������������������ӵ�_online_host�б���
	4.����������������IP��ӡ���������û�ѡ��
������ȡ�ļ��б�
    1.�ȷ������󣬵õ���Ӧ��������ģ��ļ����ƣ�
	2.��ӡ����  --- ���������Ӧ���ļ����ƴ�ӡ���������û�ѡ��
���������ļ�
    1.�����˷����ļ���������
	2.�õ�����˷��͹�������Ӧ�����������Ӧ����е�body���־����ļ��е���������
	3.�����ļ������õ��Ŀͻ��˷��͹�����HTTP��Ӧ�е�body���ֵ��ļ��е���������д��մ������ļ��У��ر��ļ�
��1���ֿ������ļ�
    1.����HEAD����ͨ������˷���������Ӧͷ���е�Content-Length��ȡ���ļ��Ĵ�С
	2.�����ļ���С���зֿ�
	3.��һ����ֿ����ݣ�������˷��͹�������Ӧ����е�body���֣��ļ����ݣ���һд�봴�����ļ��е�ָ��λ��
*/

class P2PHost
{
public:
	uint32_t _ip_addr;//Ҫ��Ե�������IP��ַ
	bool _pair_ret;//���ڴ����Խ������Գɹ�true��ʧ�ܾ���false

};
class P2PClient
{
public:
	//�ͻ��˳������ں���
	bool Start()
	{
		//�ͻ��˳�����Ҫѭ�����У���Ϊ�����ļ���ֹһ��
		//�������������ÿ�ζ�Ҫ���»�ȡ�����������-----�����ǲ������
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}


	//������Ե��߳���ں���
	void HostPair(P2PHost* host)
	{
		//1. ��֯httpЭ���ʽ����������
		//2. �һ��tcp�ͻ��ˣ������ݷ���
		//3. �ȴ��������˵Ļظ������н���
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);//�������ֽ���IP��ַת�����ַ���IP��ַ
		httplib::Client cli(buf, P2P_PORT);          //ʵ����httplib�ͻ��˶���
		auto rsp = cli.Get("/hostpair");             //�������������ԴΪ/hostpair��GET����,�����ӽ���ʧ�ܣ�GET�᷵�ؿ�ָ��
		if (rsp && rsp->status == 200)               //�ж���Ӧ����Ƿ���ȷ
			host->_pair_ret = true;                  //����������Խ�� 

		return;
	}

	//һ����ȡ��������
	bool GetOnlineHost()
	{
		//�Ƿ�����ƥ��
		char ch = 'Y';
		if (!_online_host.empty())
		{
			std::cout << "�Ƿ����²鿴����������Y/N��:";
			fflush(stdout);
			std::cin >> ch;
		}

		if (ch == 'Y')
		{
			std::cout << "��ʼ����ƥ�䡣����\n";
			//1. ��ȡ������Ϣ�������õ������������е�IP��ַ�б�
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			std::vector<P2PHost> host_list;

			//���forѭ���������֮��host_list�������������������е�������IP��ַ
			for (int i = 0; i < list.size(); ++i)
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				// ��������� = ip��ַ & ��������
				uint32_t net = (ntohl(ip & mask));
				// ������������� = (~��������)
				uint32_t max_host = (~ntohl(mask));


				//2. �����IP��ַ�б��е����������������(Ϊ�����̴���ʱ�䣬ʹ�ö��߳�)
				// �õ�ÿһ������IP��ַ��������Ϊ0��IP��ַ�������ַ��������ȫΪ1����udp�Ĺ㲥��ַ���ܷ��䣩
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					//�������IP�ļ���Ӧ��ʹ�������ֽ��������ź������ţ�С���ֽ���������ӵĽ�������ر��
					uint32_t host_ip = net + j;
					P2PHost host;
					host._ip_addr = htonl(host_ip);//����������ֽ����IP��ַת��Ϊ���� �ֽ���
					host._pair_ret = false;
					host_list.push_back(host);//host_list�����������������ַ
				}
			}


			//3. �ȴ������߳����������ϣ��ж���Խ�������������õ���Ӧ�����Ӧ����Ϊ����������������������ӵ�_online_host��
			//��host_list�е����������߳̽������
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); ++i)
			{ 
				thr_list[i] = new std::thread(&P2PClient::HostPair, this, &host_list[i]);
			}

			std::cout << "��������ƥ���У����Ժ󣡣���\n";
			for (int i = 0; i < host_list.size(); ++i)
			{
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true)
					_online_host.push_back(host_list[i]);
				delete thr_list[i];
			}
		}


		//4. ����������������IP��ӡ���������û�ѡ��
		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//5. ��ӡ���������б����û�ѡ��
		std::cout << "��ѡ����Ե���������ȡ�����ļ��б�";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip);//�û�ѡ������֮�󣬵��û�ȡ�ļ��б�ӿ�


		return true;
	}


	//������ȡ�ļ��б�
	bool GetShareList(const std::string &host_ip)
	{
		//�����˷���һ���ļ��б��ȡ����
		//1. �ȷ�������
		//2. �õ���Ӧ֮�󣬽������ģ��ļ����ƣ�
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
	
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "��ȡ�ļ��б���Ӧ����\n";
			return false;
		}
		//��ӡ���� --- ��ӡ�������Ӧ���ļ������б��û�ѡ��
		//body: filename1\r\nfilename2\r\n...
		std::cout << rsp->body << std::endl;
		std::cout << "\n��ѡ��Ҫ���ص��ļ���";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}


	//���������ļ�
	bool DownloadFile(const std::string &host_ip, const std::string& filename)
	{
		//1. �����˷����ļ���������
		//2. �õ���Ӧ�������Ӧ����е�body���ľ����ļ�����
		//3. �����ļ������ļ�����д���ļ��У��ر��ļ�
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);

		std::cout << "�����˷����ļ���������" << host_ip << ":" << req_path << std::endl;

		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "�����ļ�����ȡ��Ӧ��Ϣʧ��:" << rsp->status << std::endl;
			return false;
		}

		std::cout << "��ȡ�ļ�������Ӧ�ɹ�\n";
		//�ж��ļ��Ƿ���ڣ���������ھʹ���
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
			boost::filesystem::create_directory(DOWNLOAD_PATH);//�����ļ�
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}

		std::cout << "�ļ����سɹ���\n";

		return true;
	}



	
	int64_t getfilesize(const std::string& host_ip, const std::string& req_path)
	{
		//1. ����HEAD����ͨ����Ӧͷ����Content-Length��Ϣ��ȡ���ļ���С
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "��ȡ�ļ���С��Ϣʧ�ܣ�\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");
		int64_t filesize = StringUtil::Str2Dig(clen);

		return filesize;
	}
	bool RangeDownload(const std::string& host_ip, const std::string& filename)
	{
		std::string req_path = "/download/" + filename;
		int64_t filesize = getfilesize(host_ip, req_path);

		//2. �����ļ���С���зֿ�
		//����ļ���С��99M��û��Ҫ�ֿ�
		//����ļ���С��101M����Ϊ0-100M��101M-
		//��1������ֿ��С
		if (filesize < MAX_RANGE)
		{
			//�� ���ļ���СС�ڿ��С����ֱ�������ļ�
			std::cout << "�ļ���СС�ڿ��С��ֱ�������ļ���" << std::endl;
			return DownloadFile(host_ip, filename);
		}
		std::cout << "�ļ���С���ڿ��С���ֿ������ļ���" << std::endl;
		int64_t range_count = 0;
		if ((filesize % MAX_RANGE) != 0)
		{
			//�� ���ļ���С�����������С����ֿ����Ϊ �ļ���С/�ֿ��С+1
			range_count = (filesize / MAX_RANGE) + 1;
		}
		else
		{
			//�� ���ļ���С�պ��������С����ֿ����Ϊ �ļ���С/�ֿ��С
			range_count = filesize / MAX_RANGE;
		}

		//��2���ֿ�
		int64_t range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; ++i)
		{
			range_start = i * MAX_RANGE;
			if (i == (range_count - 1))
				range_end = filesize - 1;//�������һ���ֿ�
			else
				range_end = ((i + 1) * MAX_RANGE) - 1;//�����һ���ֿ�


			std::cout << "�ͻ�������ֿ�����" << range_start << "-" << range_end << std::endl;
			//3. ��һ����ֿ����ݣ��õ���Ӧ֮��д���ļ���ָ��λ��
			//header.insert(httplib::make_range_header({ { range_start, range_end } }));//����һ��range����
			_rangeDownload(host_ip, filename, range_start, range_end);

		}

		std::cout << "�ļ����سɹ���" << std::endl;

		return true;
	}
	bool _rangeDownload(const std::string &host_ip, const std::string &filename, int64_t s, int64_t e)
	{
		std::string req_path = "/download/" + filename;
		std::string realpath = DOWNLOAD_PATH + filename;

		/*
		httplib::Headers header;
		header = httplib::make_range_header({(s, e)});
		header.insert(httplib::make_range_header({(s, e)}));����һ��range����
		*/
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str(), { httplib::make_range_header(s,e) });
		if (rsp == NULL || rsp->status != 206)
		{
			if (rsp == NULL)
				std::cout << "�ͻ��˻ظ�Ϊ��\n";
			else
				std::cout << "��Ӧ״̬�룺" << rsp->status << "\n";
			std::cout << "�ļ�����ʧ��\n";
			return false;
		}

		
		FileUtil::Write(realpath, rsp->body, s);
		std::cout << "�ͻ����ļ����سɹ���\n";

		return true;
	}

private:
	std::vector<P2PHost> _online_host;
};