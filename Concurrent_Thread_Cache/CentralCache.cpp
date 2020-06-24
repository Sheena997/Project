#include "CentralCache.h"
#include "PageCache.h"

//��ȡspan���ֿ��ܣ�����������span������û��span��ʱҪ��page cache����
Span* CentralCache::GetOneSpan(size_t size)
{
	//��ȡһ���ж����span
	size_t index = SizeClass::FreeListIndex(size);
	SpanList& spanlist = _spanLists[index];
	Span* it = spanlist.Begin();
	while (it != spanlist.End())
	{
		if (!it->_freelist.Empty())
		{
			//��ȡ����
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	//����central cache��û��span������page cacheȥ�����ڴ�
	size_t numpage = SizeClass::NumMovePage(size);
	Span* span = PageCache::GetInstance().NewSpan(numpage);
	//��span�����гɶ�Ӧ��С�ҵ�spanlist��
	char* start = (char*)(span->_pageid << PAGESHIFT); //id * 4k������ʼ��ַ
	char* end = start + (span->_pagesize << PAGESHIFT);//ҳ��*һҳ��С��4k�������м����еĴ�С
	while (start < end)
	{
		char* obj = start;
		start += size;
		//����֮���г����ڴ�obj�ҵ�span�е�_freelist��
		span->_freelist.Push(obj);
	}
	span->_objsize = size;
	spanlist.PushFront(span);//��span�ҵ�spanlist��

	return span;
}
//���ظ�thread cache��ȡ�Ķ��ٸ�span
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size)
{ 
	size_t index = SizeClass::FreeListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	Span* span = GetOneSpan(size);
	FreeList& freelist = span->_freelist;
	size_t actualNum = freelist.PopRange(start, end, num);
	span->_usecount += actualNum;
	//�Ż������һ��span�е�freelistΪ�գ��򽫸�span�ŵ�spanlist�����
	
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
		span->_freelist.Push(start);//ͷ��
		--span->_usecount;

		//��ʾ��ǰspan�Ҹ�threadcache�Ķ���ȫ�����أ�����span����page cache�����кϲ��������ڴ���Ƭ����
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