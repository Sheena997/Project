#include "Tools.hpp"
#include "Client.hpp"
#include "httplib.h"
#include "Server.hpp"


void ClientRun()
{
	//客户端后运行
	P2PClient cli;
	cli.Start();
}
int main(int argc, char *argv[])
{
	//在主线程中运行客户端模块以及服务端模块
	//创建一个线程运行客户端模块，主线程运行服务端模块
	std::thread thr_client(ClientRun);//若客户端运行的时候，服务端还没有启动
	
	P2PServer srv;
	srv.Start();

	return 0;
}
