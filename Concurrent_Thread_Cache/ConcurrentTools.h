#pragma once

#include "ThreadCache.h"
#include "PageCache.h"


static void* ConcurrentMallocate(size_t size)
{
	//[1byte, 64kb] --- 找central cache申请内存
	if (size <= MAXSIZE)//不超过64K
	{
		if (tlsThreadCache == nullptr)
		{
			tlsThreadCache = new ThreadCache;
			std::cout << std::this_thread::get_id() << " -> " << tlsThreadCache << std::endl;
		}

		return tlsThreadCache->Allocate(size);
	}
	else if(size <= (MAXPAGES - 1)<<PAGESHIFT)
	{
		//[64kb, 128*4kb] ---- 找page cache 申请内存
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGESHIFT);
		size_t pagenum = (align_size >> PAGESHIFT);
		Span* span = PageCache::GetInstance().NewSpan(pagenum);
		span->_objsize = align_size;
		void* ptr = (void*)(span->_pageid >> PAGESHIFT);
		return ptr;
	}
	else
	{
		//[128*4kb, -] ---- 直接找系统申请内存
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGESHIFT);
		size_t pagenum = (align_size >> PAGESHIFT);
		return SystemAllocate(pagenum);
	}
}

static void ConcurrentFree(void* ptr)
{
	size_t pageid = (PAGE_ID)ptr >> PAGESHIFT;
	Span* span = PageCache::GetInstance().GetIdToSpan(pageid);
	if (span == nullptr)
	{
		//[128*4kb, -]
		SystemFree(ptr);
		return;
	}
	size_t size = span->_objsize;
	if (size <= MAXSIZE)//不超过64K[1byte, 64kb]去找central cache
	{
		tlsThreadCache->Deallocate(ptr, size);
	}
	else if (size <= (MAXPAGES - 1) << PAGESHIFT)
	{
		PageCache::GetInstance().ReleaseSpanToPageCache(span);
	}


}