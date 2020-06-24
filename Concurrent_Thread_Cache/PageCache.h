#pragma once
#include "Common.h"

class PageCache
{
public:
	//����ģʽ������ģʽ��//ֻ��һ��PageCache����
	static PageCache& GetInstance()
	{
		static PageCache Inst;
		return Inst;
	}

	//Ҫһ������ҳ��span
	Span* _NewSpan(size_t numpage);
	Span* NewSpan(size_t numpage);

	//��ϵͳ����ҳ�ڴ�ҵ�����������
	void SystemAllocPage(size_t numpage);

	//����id��Ӧ���Ǹ�span
	Span* GetIdToSpan(PAGE_ID id);

	//centralcache���ڴ��PageCache
	void ReleaseSpanToPageCache(Span* span);
private:
	PageCache()
	{}
	PageCache(const PageCache&) = delete;

	SpanList _spanLists[MAXPAGES];
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	std::mutex _mtx;
};

