#include "CentralCache.h"
#include "PageCache.h"

//获取span两种可能，可能下面有span，可能没有span此时要向page cache申请
Span* CentralCache::GetOneSpan(size_t size)
{
	//获取一个有对象的span
	size_t index = SizeClass::FreeListIndex(size);
	SpanList& spanlist = _spanLists[index];
	Span* it = spanlist.Begin();
	while (it != spanlist.End())
	{
		if (!it->_freelist.Empty())
		{
			//获取对象
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	//现在central cache中没有span，就找page cache去申请内存
	size_t numpage = SizeClass::NumMovePage(size);
	Span* span = PageCache::GetInstance().NewSpan(numpage);
	//把span对象切成对应大小挂到spanlist中
	char* start = (char*)(span->_pageid << PAGESHIFT); //id * 4k就是起始地址
	char* end = start + (span->_pagesize << PAGESHIFT);//页数*一页大小（4k）就是中间所有的大小
	while (start < end)
	{
		char* obj = start;
		start += size;
		//切完之后将切出的内存obj挂到span中的_freelist上
		span->_freelist.Push(obj);
	}
	span->_objsize = size;
	spanlist.PushFront(span);//将span挂到spanlist中

	return span;
}
//返回给thread cache获取的多少个span
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size)
{ 
	size_t index = SizeClass::FreeListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	Span* span = GetOneSpan(size);
	FreeList& freelist = span->_freelist;
	size_t actualNum = freelist.PopRange(start, end, num);
	span->_usecount += actualNum;
	//优化，如果一个span中的freelist为空，则将该span放到spanlist的最后
	
	spanlist.Unlock();
	return actualNum;
}


void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::FreeListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();
	while (start)
	{
		void* next = NextObj(start);
		PAGE_ID id = (PAGE_ID)start >> PAGESHIFT;
		Span* span = PageCache::GetInstance().GetIdToSpan(id);
		span->_freelist.Push(start);//头插
		--span->_usecount;

		//表示当前span且给threadcache的对象全部返回，给将span还给page cache，进行合并，减少内存碎片问题
		if (span->_usecount == 0)
		{
			size_t index = SizeClass::FreeListIndex(span->_objsize);
			_spanLists[index].Erase(span);
			span->_freelist.Clear();

			PageCache::GetInstance().ReleaseSpanToPageCache(span);
		}
		start = next;
	}
	spanlist.Unlock();
}