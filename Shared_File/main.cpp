#include "Tools.hpp"
#include "Client.hpp"
#include "httplib.h"
#include "Server.hpp"


void ClientRun()
{
	//�ͻ��˺�����
	P2PClient cli;
	cli.Start();
}
int main(int argc, char *argv[])
{
	//�����߳������пͻ���ģ���Լ������ģ��
	//����һ���߳����пͻ���ģ�飬���߳����з����ģ��
	std::thread thr_client(ClientRun);//���ͻ������е�ʱ�򣬷���˻�û������
	
	P2PServer srv;
	srv.Start();

	return 0;
}
