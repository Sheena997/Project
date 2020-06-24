#include "PageCache.h"

//central cache向page cache要numpage个span
Span* PageCache::_NewSpan(size_t numpage)
{
	/*
	当central cache向page cache申请内存时， page cache先检查对应位置有没有span，
	如果没有，则想大页寻找一个span分成两个。
	如申请的是5页的，但是5页后面没有挂span，则向后面寻找更大的页数，如8页的位置
	有span，则将该span分裂成一个5page大小的span和一个3page大小的span。
	*/

	if (!_spanLists[numpage].Empty())
	{
		//该页数对应的位置的spanlist中有对象，则直接取
		Span* span = _spanLists[numpage].Begin();
		_spanLists[numpage].PopFront();
		return span;
	}
	//废弃了0这个位置，是从1这个位置开始的
	for (size_t i = numpage + 1; i < MAXPAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			//后面挂了有span
			Span* span = _spanLists[i].Begin();
			_spanLists[i].PopFront();

			//分裂成两个span,一个给central cache，一个挂回page cache对应大小的spanlist中
			Span* splitspan = new Span;
			splitspan->_pageid = span->_pageid + span->_pagesize - numpage;//要一页加到228页，然后减一页就是227页
			splitspan->_pagesize = numpage;
			for (PAGE_ID j = 0; j < numpage; ++j)
			{
				_idSpanMap[splitspan->_pageid + j] = splitspan;
			}
			span->_pagesize -= numpage;

			//把不要的前面的挂到central cache的对应的span的freelist下
			_spanLists[span->_pagesize].PushFront(span);

			return splitspan;//返回切出来的页
		}
	}

    //如果page cache中没有span
	void* ptr = SystemAllocate(MAXPAGES - 1);//向系统要内存
	//第一次进来时，所有pagesize对应的spanlist中的span对象都是空的
	//则page cache向系统申请空间
	//切尾，为了不用每次都要更新映射很多关系
	Span* bigspan = new Span;      //--------对象池 （ObjectPool<Span>）
	bigspan->_pageid = ((PAGE_ID)ptr >> PAGESHIFT);//地址/4k = 页号
	bigspan->_pagesize = MAXPAGES - 1;
	for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;
	}
	_spanLists[bigspan->_pagesize].PushFront(bigspan);

	return _NewSpan(numpage);
	//递归调用，递归值递归一次，下次一定会返回的，
	//分裂span，如此时需要3页的一个span,然后page cache中没有
	//向系统申请128页大小的内存，然后分割，成一个3page的和一个125page的
	//125page的挂到page cache对应位置的spanlist中
}

Span* PageCache::NewSpan(size_t numpage)
{
	_mtx.lock();//防止递归锁
	Span* span = _NewSpan(numpage);
	_mtx.unlock();

	return span;
}
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	///页的合并
	while (1)
	{
		PAGE_ID prevSpanId = span->_pageid - 1;//前一页的页号
		auto pit = _idSpanMap.find(prevSpanId);
		//前面的一页不存在
		if (pit == _idSpanMap.end())
			break;
		//前一页存在
		Span* prevSpan = pit->second;
		if (prevSpan->_usecount != 0)
		{
			//说明前一页还在使用中，centralcache没有还给pagecache
			break;
		}

		//合并
		//前一页存在且，usecount = 0
		//如果合并出超过128页的span，那就不要合并
		if (span->_pagesize + prevSpan->_pagesize >= MAXPAGES)
		{
			break;
		}
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; ++i)
			_idSpanMap[prevSpan->_pageid + i] = span;
		
		_spanLists[prevSpan->_pagesize].Erase(prevSpan);
		delete prevSpan;
	}


	//向后合并
	while (1)
	{
		PAGE_ID nextPageId = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageId);
		if (nextIt == _idSpanMap.end())
			break;


		
		Span* nextSpan = nextIt->second;
		if (nextSpan->_usecount != 0)
			break;

		//合并
		//后一页存在且，usecount = 0
		//如果合并出超过128页的span，那就不要合并
		if (span->_pagesize + nextSpan->_pagesize >= MAXPAGES)
		{
			break;
		}
		span->_pagesize += nextSpan->_pagesize;
		for (PAGE_ID i = 0; i < nextSpan->_pagesize; ++i)
			_idSpanMap[nextSpan->_pageid + i] = span;
		
		_spanLists[span->_pagesize].Erase(nextSpan);
		delete nextSpan;
	}

	_spanLists[span->_pagesize].PushFront(span);
}

Span* PageCache::GetIdToSpan(PAGE_ID id)
{
	std::unordered_map<PAGE_ID, Span*>::iterator it = _idSpanMap.find(id);
	if (it != _idSpanMap.end())
		return it->second;
	else
		return nullptr;
}