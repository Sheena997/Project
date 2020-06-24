#pragma once

#include "Common.h"

class ThreadCache
{
public:
	//�����ڴ���ͷ��ڴ�
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	//�����Ļ��棨central cache���л�ȡ����
	void* FetchFromCentralCache(size_t size);

	//������������ж��󳬹�һ�����ȣ���Ҫ�ͷŸ�central cache
	void ListTooLong(FreeList& freeList, size_t num, size_t size);
private:
	FreeList _freeLists[NFREELIST];

	ThreadCache* _next;//Ϊ��ʵ�ֲ���
	int threadId;//Ϊ��ʵ�ֲ���
};

//TLS:�ֲ߳̾��洢
//Ϊ��ʵ�ֲ�������֤ÿ���̶߳����Լ���thread cache�̳߳�
//ÿ���̶߳����Լ����������
_declspec (thread) static ThreadCache* tlsThreadCache = nullptr;