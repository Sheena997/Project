#pragma once
#include "Tools.hpp"
#include <thread>
#include <boost/filesystem.hpp>
#include "httplib.h"

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define SHARED_PATH "./Shared/"


/*
一、接收到主机配对请求，发送200响应给客户端
二、接收到文件列表请求，获取共享文件列表 ---- 在主机上设置一个共享目录，凡是共享目录下的文件都是要共享给别人的
    1.查看目录是否存在，若目录不存在，则创建一个目录
	2.获取文件名称，将每个文件名称放到rsp的正文部分
三、接收到指定的文件下载请求
    1.如果客户端要下载的文件，是个目录或者不存在，则报错返回
 （1）如果是客户端发送过来的是GET请求
	2.打开文件，读取文件数据，将该文件的数据作为HTTP响应的body部分发送给客户端
 （2）如果客户端发送过来的是HEAD请求
    2.获取文件大小，将文件大小放入HTTP响应的头部设置头部，发送HTTP响应码为200，给客户端
*/

class P2PServer
{
public:
	bool Start()
	{
		//添加针对客户端请求的处理方式对应关系
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);
		//正则表达式中。 . ：匹配除\n或者\r之外的任意字符，* ：表示匹配前边的字符人员次
		//防止与上方的请求冲突，因此在请求中加入download路径
		_srv.Get("/download/.*", RecvDownload);//正则表达式：将特殊字符以指定的格式，表具有关键特征的数据
		

		//开启监听服务端
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}
private:
	//发送200响应给客户端
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;

		return;
	}

	//获取共享文件列表 ---》在主机上设置一个共享目录，凡是这个目录下的文件都是要共享给别人的
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.查看目录是否存在，若目录不存在，则创建这个目录
		if (!boost::filesystem::exists(SHARED_PATH))//判断文件是否存在
		{
			//创建目录
			boost::filesystem::create_directory(SHARED_PATH);
		}
		//实例化目录迭代器
		boost::filesystem::directory_iterator begin(SHARED_PATH);
		//实例化目录迭代器的末尾
		boost::filesystem::directory_iterator end;

		//开始迭代目录
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//如果是个目录就不用它
				//当前版本我们只获取普通文件名称，不做多层级目录的操作
				continue;
			}

			//获取文件名称
			std::string name = begin->path().filename().string();//只要文件名不要路径
			//将每个文件名称放到rsp的正文部分
			rsp.body += name + "\r\n"; //文件名的格式：filename1\r\nfilename2\r\n...

		}
		rsp.status = 200;

		return;
	}


	//接收文件下载的请求
	static void RecvDownload(const httplib::Request &req, httplib::Response &rsp)
	{
		//req.path - 客户端请求的资源路径  /download/filename
		//只需要获取filename，不需要前面的download
		boost::filesystem::path req_path(req.path);
		//只获取文件名称，不要路径
		std::string name = req_path.filename().string();
		//文件实际的路径应该是在共享的目录下
		std::string realpath = SHARED_PATH + name;

		//判断文件是否有存在，或者是不是目录
		//boost::filesystem::exits()判断文件是否存在
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			//如果文件不存在，或者文件是目录，则报错返回
			rsp.status = 404;
			return;
		}
		if (req.method == "GET")
		{
			//判断是否是分块传输的依据，就是这次请求中是否有Range头部字段
			if (req.has_header("Range"))
			{
				//判断请求头中包含Range字段===分块传输
				//需要知道分块区间是多少
				std::string range_str = req.get_header_value("Range");//Range: bytes=start-end
				int64_t range_start, range_end;
				
				//解析客户端的range数据
				FileUtil::GetRange(range_str, range_start, range_end);
				std::string body;
				int64_t range_len = range_end - range_start + 1;//计算一块的长度
				std::cout << "Range：" << realpath  << "=》"<< "range: " << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &body, range_len, range_start);
				rsp.set_content(body.c_str(), range_len, "text/plain");
				rsp.set_header("Content-Length", std::to_string(rsp.body.size()).c_str());
				rsp.status = 206;
				std::cout << "服务端响应区间数据完毕!!!" << std::endl;
				/*
				//测试小文本文件，当文件内容是string的时候，如测试sheena.txt
				std::cout << "服务端响应区间数据完毕：[" << rsp.body << "]\n";
				*/
			}
			else
			{
				//没有Range头部信息，则是一个完整的文件下载，不需要分块
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{
					//读取文件失败
					rsp.status = 500; //内部错误
					return;
				}
				rsp.status = 200;//正确处理了
			}
		}
		else
		{
			//这个是针对HEAD请求的 ====》 客户端只要头部不要正文
			//获取文件大小
			int64_t filesize = FileUtil::GetFileSize(realpath);
			//设置要发送给HTTP响应的头部信息
			//response::set_header(const std::string &key, const std::string &val)
			rsp.set_header("Content-Length",std::to_string(filesize).c_str());
			rsp.status = 200;
		}
		std::cout << "服务端文件下载请求响应完毕！\n";

		return;
	}

private:
	httplib::Server _srv;
};