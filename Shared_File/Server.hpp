#pragma once
#include "Tools.hpp"
#include <thread>
#include <boost/filesystem.hpp>
#include "httplib.h"

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define SHARED_PATH "./Shared/"


/*
һ�����յ�����������󣬷���200��Ӧ���ͻ���
�������յ��ļ��б����󣬻�ȡ�����ļ��б� ---- ������������һ������Ŀ¼�����ǹ���Ŀ¼�µ��ļ�����Ҫ��������˵�
    1.�鿴Ŀ¼�Ƿ���ڣ���Ŀ¼�����ڣ��򴴽�һ��Ŀ¼
	2.��ȡ�ļ����ƣ���ÿ���ļ����Ʒŵ�rsp�����Ĳ���
�������յ�ָ�����ļ���������
    1.����ͻ���Ҫ���ص��ļ����Ǹ�Ŀ¼���߲����ڣ��򱨴���
 ��1������ǿͻ��˷��͹�������GET����
	2.���ļ�����ȡ�ļ����ݣ������ļ���������ΪHTTP��Ӧ��body���ַ��͸��ͻ���
 ��2������ͻ��˷��͹�������HEAD����
    2.��ȡ�ļ���С�����ļ���С����HTTP��Ӧ��ͷ������ͷ��������HTTP��Ӧ��Ϊ200�����ͻ���
*/

class P2PServer
{
public:
	bool Start()
	{
		//�����Կͻ�������Ĵ���ʽ��Ӧ��ϵ
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);
		//������ʽ�С� . ��ƥ���\n����\r֮��������ַ���* ����ʾƥ��ǰ�ߵ��ַ���Ա��
		//��ֹ���Ϸ��������ͻ������������м���download·��
		_srv.Get("/download/.*", RecvDownload);//������ʽ���������ַ���ָ���ĸ�ʽ������йؼ�����������
		

		//�������������
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}
private:
	//����200��Ӧ���ͻ���
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;

		return;
	}

	//��ȡ�����ļ��б� ---��������������һ������Ŀ¼���������Ŀ¼�µ��ļ�����Ҫ��������˵�
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.�鿴Ŀ¼�Ƿ���ڣ���Ŀ¼�����ڣ��򴴽����Ŀ¼
		if (!boost::filesystem::exists(SHARED_PATH))//�ж��ļ��Ƿ����
		{
			//����Ŀ¼
			boost::filesystem::create_directory(SHARED_PATH);
		}
		//ʵ����Ŀ¼������
		boost::filesystem::directory_iterator begin(SHARED_PATH);
		//ʵ����Ŀ¼��������ĩβ
		boost::filesystem::directory_iterator end;

		//��ʼ����Ŀ¼
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//����Ǹ�Ŀ¼�Ͳ�����
				//��ǰ�汾����ֻ��ȡ��ͨ�ļ����ƣ�������㼶Ŀ¼�Ĳ���
				continue;
			}

			//��ȡ�ļ�����
			std::string name = begin->path().filename().string();//ֻҪ�ļ�����Ҫ·��
			//��ÿ���ļ����Ʒŵ�rsp�����Ĳ���
			rsp.body += name + "\r\n"; //�ļ����ĸ�ʽ��filename1\r\nfilename2\r\n...

		}
		rsp.status = 200;

		return;
	}


	//�����ļ����ص�����
	static void RecvDownload(const httplib::Request &req, httplib::Response &rsp)
	{
		//req.path - �ͻ����������Դ·��  /download/filename
		//ֻ��Ҫ��ȡfilename������Ҫǰ���download
		boost::filesystem::path req_path(req.path);
		//ֻ��ȡ�ļ����ƣ���Ҫ·��
		std::string name = req_path.filename().string();
		//�ļ�ʵ�ʵ�·��Ӧ�����ڹ����Ŀ¼��
		std::string realpath = SHARED_PATH + name;

		//�ж��ļ��Ƿ��д��ڣ������ǲ���Ŀ¼
		//boost::filesystem::exits()�ж��ļ��Ƿ����
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			//����ļ������ڣ������ļ���Ŀ¼���򱨴���
			rsp.status = 404;
			return;
		}
		if (req.method == "GET")
		{
			//�ж��Ƿ��Ƿֿ鴫������ݣ���������������Ƿ���Rangeͷ���ֶ�
			if (req.has_header("Range"))
			{
				//�ж�����ͷ�а���Range�ֶ�===�ֿ鴫��
				//��Ҫ֪���ֿ������Ƕ���
				std::string range_str = req.get_header_value("Range");//Range: bytes=start-end
				int64_t range_start, range_end;
				
				//�����ͻ��˵�range����
				FileUtil::GetRange(range_str, range_start, range_end);
				std::string body;
				int64_t range_len = range_end - range_start + 1;//����һ��ĳ���
				std::cout << "Range��" << realpath  << "=��"<< "range: " << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &body, range_len, range_start);
				rsp.set_content(body.c_str(), range_len, "text/plain");
				rsp.set_header("Content-Length", std::to_string(rsp.body.size()).c_str());
				rsp.status = 206;
				std::cout << "�������Ӧ�����������!!!" << std::endl;
				/*
				//����С�ı��ļ������ļ�������string��ʱ�������sheena.txt
				std::cout << "�������Ӧ����������ϣ�[" << rsp.body << "]\n";
				*/
			}
			else
			{
				//û��Rangeͷ����Ϣ������һ���������ļ����أ�����Ҫ�ֿ�
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{
					//��ȡ�ļ�ʧ��
					rsp.status = 500; //�ڲ�����
					return;
				}
				rsp.status = 200;//��ȷ������
			}
		}
		else
		{
			//��������HEAD����� ====�� �ͻ���ֻҪͷ����Ҫ����
			//��ȡ�ļ���С
			int64_t filesize = FileUtil::GetFileSize(realpath);
			//����Ҫ���͸�HTTP��Ӧ��ͷ����Ϣ
			//response::set_header(const std::string &key, const std::string &val)
			rsp.set_header("Content-Length",std::to_string(filesize).c_str());
			rsp.status = 200;
		}
		std::cout << "������ļ�����������Ӧ��ϣ�\n";

		return;
	}

private:
	httplib::Server _srv;
};