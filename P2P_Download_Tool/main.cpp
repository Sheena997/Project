#include "Tools.hpp"
#include "P2PClient.hpp"
#include "httplib.h"
#include "P2PServer.hpp"


void ClientRun()
{
	//�ͻ��˺�����
	Sleep(1);
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
