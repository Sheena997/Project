#pragma once

#include "Common.h"

class CentralCache
{
public:
	//����ģʽ(����ģʽ)ֻ��һ��������Ϊ����threadcacheֻ��һ��centralcache
	static CentralCache& GetInstance()
	{
		static CentralCache Inst;
		return Inst;
	}

	//ThreadCache��CentralCache�����ڴ�
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);

	//��һ�������Ķ����ͷ�span�����
	void  ReleaseListToSpans(void* start, size_t size);

	//��spanlist ���� page cache�л�ȡһ��span
	Span* GetOneSpan(size_t size);
private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;

	SpanList _spanLists[NFREELIST];
};

