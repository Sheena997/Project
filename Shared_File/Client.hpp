#pragma once
#include "Tools.hpp"
#include <thread>
#include "httplib.h"

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define SHARED_PATH "./Shared/"
#define DOWNLOAD_PATH "./Download/"
//1左移10位是1K，左移20位是1M
//#define MAX_RANGE 100 << 20
#define MAX_RANGE (5)


/*
一、获取在线主机
    1.获取网卡信息进而得到局域网的所有IP地址列表
	2.向逐个IP地址列表中的主机发送配对请求（为了缩短时间，使用多线程）
	3.等待所有线程主机配对完毕，判断配对结果。若配对成功，则表示对应主机为在线主机，将该主机添加到_online_host列表中
	4.将所有在线主机的IP打印出来，供用户选择
二、获取文件列表
    1.先发送请求，得到响应后解析正文（文件名称）
	2.打印正文  --- 将服务端响应的文件名称打印出来，供用户选择
三、下载文件
    1.向服务端发送文件下载请求
	2.得到服务端发送过来的响应结果，其中响应结果中的body部分就是文件中的数据内容
	3.创建文件，将得到的客户端发送过来的HTTP响应中的body部分的文件中的数据内容写入刚创建的文件中，关闭文件
（1）分块下载文件
    1.发送HEAD请求，通过服务端发过来的响应头部中的Content-Length获取到文件的大小
	2.根据文件大小进行分块
	3.逐一请求分块数据，将服务端发送过来的响应结果中的body部分（文件数据）逐一写入创建的文件中的指定位置
*/

class P2PHost
{
public:
	uint32_t _ip_addr;//要配对的主机的IP地址
	bool _pair_ret;//用于存放配对结果，配对成功true，失败就是false

};
class P2PClient
{
public:
	//客户端程序的入口函数
	bool Start()
	{
		//客户端程序需要循环运行，因为下载文件不止一次
		//但是这样造成了每次都要重新获取在线主机配对-----这样是不合理的
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}


	//主机配对的线程入口函数
	void HostPair(P2PHost* host)
	{
		//1. 组织http协议格式的请求数据
		//2. 搭建一个tcp客户端，将数据发送
		//3. 等待服务器端的回复，进行解析
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);//将网络字节序IP地址转换成字符串IP地址
		httplib::Client cli(buf, P2P_PORT);          //实例化httplib客户端对象
		auto rsp = cli.Get("/hostpair");             //向服务器发送资源为/hostpair的GET请求,若连接建立失败，GET会返回空指针
		if (rsp && rsp->status == 200)               //判断响应结果是否正确
			host->_pair_ret = true;                  //重置主机配对结果 

		return;
	}

	//一、获取在线主机
	bool GetOnlineHost()
	{
		//是否重新匹配
		char ch = 'Y';
		if (!_online_host.empty())
		{
			std::cout << "是否重新查看在线主机（Y/N）:";
			fflush(stdout);
			std::cin >> ch;
		}

		if (ch == 'Y')
		{
			std::cout << "开始主机匹配。。。\n";
			//1. 获取网卡信息，进而得到局域网中所有的IP地址列表
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			std::vector<P2PHost> host_list;

			//这个for循环运行完毕之后，host_list将包含所有网卡的所有的主机的IP地址
			for (int i = 0; i < list.size(); ++i)
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				// 计算网络号 = ip地址 & 子网掩码
				uint32_t net = (ntohl(ip & mask));
				// 计算最大主机号 = (~子网掩码)
				uint32_t max_host = (~ntohl(mask));


				//2. 向逐个IP地址列表中的主机发送配对请求(为了缩短处理时间，使用多线程)
				// 得到每一个主机IP地址（主机号为0的IP地址是网络地址，主机号全为1的是udp的广播地址不能分配）
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					//这个主机IP的计算应该使用主机字节序的网络号和主机号（小端字节序，这样相加的结果不会特别大）
					uint32_t host_ip = net + j;
					P2PHost host;
					host._ip_addr = htonl(host_ip);//将这个主机字节序的IP地址转换为网络 字节序
					host._pair_ret = false;
					host_list.push_back(host);//host_list里包含了所有主机地址
				}
			}


			//3. 等待所有线程主机配对完毕，判断配对结果，若配对请求得到响应，则对应主机为在线主机，将在线主机添加到_online_host中
			//对host_list中的主机创建线程进行配对
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); ++i)
			{ 
				thr_list[i] = new std::thread(&P2PClient::HostPair, this, &host_list[i]);
			}

			std::cout << "正在主机匹配中，请稍后！！！\n";
			for (int i = 0; i < host_list.size(); ++i)
			{
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true)
					_online_host.push_back(host_list[i]);
				delete thr_list[i];
			}
		}


		//4. 将所有在线主机的IP打印出来，供用户选择
		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//5. 打印在线主机列表，供用户选择
		std::cout << "请选择配对的主机，获取共享文件列表：";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip);//用户选择主机之后，调用获取文件列表接口


		return true;
	}


	//二、获取文件列表
	bool GetShareList(const std::string &host_ip)
	{
		//向服务端发送一个文件列表获取请求
		//1. 先发送请求
		//2. 得到响应之后，解析正文（文件名称）
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
	
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "获取文件列表响应错误\n";
			return false;
		}
		//打印正文 --- 打印服务端响应的文件名称列表供用户选择
		//body: filename1\r\nfilename2\r\n...
		std::cout << rsp->body << std::endl;
		std::cout << "\n请选择要下载的文件：";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}


	//三、下载文件
	bool DownloadFile(const std::string &host_ip, const std::string& filename)
	{
		//1. 向服务端发送文件下载请求
		//2. 得到响应结果，响应结果中的body正文就是文件数据
		//3. 创建文件，将文件数据写入文件中，关闭文件
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);

		std::cout << "向服务端发送文件下载请求" << host_ip << ":" << req_path << std::endl;

		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "下载文件，获取响应信息失败:" << rsp->status << std::endl;
			return false;
		}

		std::cout << "获取文件下载响应成功\n";
		//判断文件是否存在，如果不存在就创建
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
			boost::filesystem::create_directory(DOWNLOAD_PATH);//创建文件
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}

		std::cout << "文件下载成功！\n";

		return true;
	}



	
	int64_t getfilesize(const std::string& host_ip, const std::string& req_path)
	{
		//1. 发送HEAD请求，通过响应头部的Content-Length信息获取到文件大小
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "获取文件大小信息失败！\n";
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

		//2. 根据文件大小进行分块
		//如果文件大小是99M，没必要分块
		//如果文件大小是101M，分为0-100M，101M-
		//（1）计算分块大小
		if (filesize < MAX_RANGE)
		{
			//① 若文件大小小于块大小，则直接下载文件
			std::cout << "文件大小小于块大小，直接下载文件！" << std::endl;
			return DownloadFile(host_ip, filename);
		}
		std::cout << "文件大小大于块大小，分块下载文件！" << std::endl;
		int64_t range_count = 0;
		if ((filesize % MAX_RANGE) != 0)
		{
			//② 若文件大小不能整除块大小，则分块个数为 文件大小/分块大小+1
			range_count = (filesize / MAX_RANGE) + 1;
		}
		else
		{
			//③ 若文件大小刚好整除块大小，则分块个数为 文件大小/分块大小
			range_count = filesize / MAX_RANGE;
		}

		//（2）分块
		int64_t range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; ++i)
		{
			range_start = i * MAX_RANGE;
			if (i == (range_count - 1))
				range_end = filesize - 1;//不是最后一个分块
			else
				range_end = ((i + 1) * MAX_RANGE) - 1;//是最后一个分块


			std::cout << "客户端请求分块下载" << range_start << "-" << range_end << std::endl;
			//3. 逐一请求分块数据，得到响应之后写入文件的指定位置
			//header.insert(httplib::make_range_header({ { range_start, range_end } }));//设置一个range区间
			_rangeDownload(host_ip, filename, range_start, range_end);

		}

		std::cout << "文件下载成功！" << std::endl;

		return true;
	}
	bool _rangeDownload(const std::string &host_ip, const std::string &filename, int64_t s, int64_t e)
	{
		std::string req_path = "/download/" + filename;
		std::string realpath = DOWNLOAD_PATH + filename;

		/*
		httplib::Headers header;
		header = httplib::make_range_header({(s, e)});
		header.insert(httplib::make_range_header({(s, e)}));设置一个range区间
		*/
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str(), { httplib::make_range_header(s,e) });
		if (rsp == NULL || rsp->status != 206)
		{
			if (rsp == NULL)
				std::cout << "客户端回复为空\n";
			else
				std::cout << "响应状态码：" << rsp->status << "\n";
			std::cout << "文件下载失败\n";
			return false;
		}

		
		FileUtil::Write(realpath, rsp->body, s);
		std::cout << "客户端文件下载成功！\n";

		return true;
	}

private:
	std::vector<P2PHost> _online_host;
};