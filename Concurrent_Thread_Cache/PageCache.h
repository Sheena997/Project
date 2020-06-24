#pragma once
#include "Common.h"

class PageCache
{
public:
	//单例模式（饿汉模式）//只有一个PageCache对象
	static PageCache& GetInstance()
	{
		static PageCache Inst;
		return Inst;
	}

	//要一个多少页的span
	Span* _NewSpan(size_t numpage);
	Span* NewSpan(size_t numpage);

	//向系统申请页内存挂到自由链表中
	void SystemAllocPage(size_t numpage);

	//给出id对应的那个span
	Span* GetIdToSpan(PAGE_ID id);

	//centralcache还内存给PageCache
	void ReleaseSpanToPageCache(Span* span);
private:
	PageCache()
	{}
	PageCache(const PageCache&) = delete;

	SpanList _spanLists[MAXPAGES];
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	std::mutex _mtx;
};

