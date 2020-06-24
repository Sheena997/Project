#pragma once

#include "ThreadCache.h"
#include "PageCache.h"


static void* ConcurrentMallocate(size_t size)
{
	//[1byte, 64kb] --- ��central cache�����ڴ�
	if (size <= MAXSIZE)//������64K
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
		//[64kb, 128*4kb] ---- ��page cache �����ڴ�
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGESHIFT);
		size_t pagenum = (align_size >> PAGESHIFT);
		Span* span = PageCache::GetInstance().NewSpan(pagenum);
		span->_objsize = align_size;
		void* ptr = (void*)(span->_pageid >> PAGESHIFT);
		return ptr;
	}
	else
	{
		//[128*4kb, -] ---- ֱ����ϵͳ�����ڴ�
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
	if (size <= MAXSIZE)//������64K[1byte, 64kb]ȥ��central cache
	{
		tlsThreadCache->Deallocate(ptr, size);
	}
	else if (size <= (MAXPAGES - 1) << PAGESHIFT)
	{
		PageCache::GetInstance().ReleaseSpanToPageCache(span);
	}


}