#pragma once

#include "Common.h"

class ThreadCache
{
public:
	//申请内存和释放内存
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	//从中心缓存（central cache）中获取对象
	void* FetchFromCentralCache(size_t size);

	//如果自由链表中对象超过一定长度，就要释放给central cache
	void ListTooLong(FreeList& freeList, size_t num, size_t size);
private:
	FreeList _freeLists[NFREELIST];

	ThreadCache* _next;//为了实现并发
	int threadId;//为了实现并发
};

//TLS:线程局部存储
//为了实现并发，保证每个线程都有自己的thread cache线程池
//每个线程都有自己的这个变量
_declspec (thread) static ThreadCache* tlsThreadCache = nullptr;