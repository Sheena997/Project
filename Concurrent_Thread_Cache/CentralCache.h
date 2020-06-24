#pragma once

#include "Common.h"

class CentralCache
{
public:
	//单例模式(饿汉模式)只有一个对象，因为所有threadcache只有一个centralcache
	static CentralCache& GetInstance()
	{
		static CentralCache Inst;
		return Inst;
	}

	//ThreadCache找CentralCache申请内存
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);

	//将一定数量的对象释放span跨度中
	void  ReleaseListToSpans(void* start, size_t size);

	//从spanlist 或者 page cache中获取一个span
	Span* GetOneSpan(size_t size);
private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;

	SpanList _spanLists[NFREELIST];
};

