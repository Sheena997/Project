#include "PageCache.h"

//central cache��page cacheҪnumpage��span
Span* PageCache::_NewSpan(size_t numpage)
{
	/*
	��central cache��page cache�����ڴ�ʱ�� page cache�ȼ���Ӧλ����û��span��
	���û�У������ҳѰ��һ��span�ֳ�������
	���������5ҳ�ģ�����5ҳ����û�й�span���������Ѱ�Ҹ����ҳ������8ҳ��λ��
	��span���򽫸�span���ѳ�һ��5page��С��span��һ��3page��С��span��
	*/

	if (!_spanLists[numpage].Empty())
	{
		//��ҳ����Ӧ��λ�õ�spanlist���ж�����ֱ��ȡ
		Span* span = _spanLists[numpage].Begin();
		_spanLists[numpage].PopFront();
		return span;
	}
	//������0���λ�ã��Ǵ�1���λ�ÿ�ʼ��
	for (size_t i = numpage + 1; i < MAXPAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			//���������span
			Span* span = _spanLists[i].Begin();
			_spanLists[i].PopFront();

			//���ѳ�����span,һ����central cache��һ���һ�page cache��Ӧ��С��spanlist��
			Span* splitspan = new Span;
			splitspan->_pageid = span->_pageid + span->_pagesize - numpage;//Ҫһҳ�ӵ�228ҳ��Ȼ���һҳ����227ҳ
			splitspan->_pagesize = numpage;
			for (PAGE_ID j = 0; j < numpage; ++j)
			{
				_idSpanMap[splitspan->_pageid + j] = splitspan;
			}
			span->_pagesize -= numpage;

			//�Ѳ�Ҫ��ǰ��Ĺҵ�central cache�Ķ�Ӧ��span��freelist��
			_spanLists[span->_pagesize].PushFront(span);

			return splitspan;//�����г�����ҳ
		}
	}

    //���page cache��û��span
	void* ptr = SystemAllocate(MAXPAGES - 1);//��ϵͳҪ�ڴ�
	//��һ�ν���ʱ������pagesize��Ӧ��spanlist�е�span�����ǿյ�
	//��page cache��ϵͳ����ռ�
	//��β��Ϊ�˲���ÿ�ζ�Ҫ����ӳ��ܶ��ϵ
	Span* bigspan = new Span;      //--------����� ��ObjectPool<Span>��
	bigspan->_pageid = ((PAGE_ID)ptr >> PAGESHIFT);//��ַ/4k = ҳ��
	bigspan->_pagesize = MAXPAGES - 1;
	for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;
	}
	_spanLists[bigspan->_pagesize].PushFront(bigspan);

	return _NewSpan(numpage);
	//�ݹ���ã��ݹ�ֵ�ݹ�һ�Σ��´�һ���᷵�صģ�
	//����span�����ʱ��Ҫ3ҳ��һ��span,Ȼ��page cache��û��
	//��ϵͳ����128ҳ��С���ڴ棬Ȼ��ָ��һ��3page�ĺ�һ��125page��
	//125page�Ĺҵ�page cache��Ӧλ�õ�spanlist��
}

Span* PageCache::NewSpan(size_t numpage)
{
	_mtx.lock();//��ֹ�ݹ���
	Span* span = _NewSpan(numpage);
	_mtx.unlock();

	return span;
}
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	///ҳ�ĺϲ�
	while (1)
	{
		PAGE_ID prevSpanId = span->_pageid - 1;//ǰһҳ��ҳ��
		auto pit = _idSpanMap.find(prevSpanId);
		//ǰ���һҳ������
		if (pit == _idSpanMap.end())
			break;
		//ǰһҳ����
		Span* prevSpan = pit->second;
		if (prevSpan->_usecount != 0)
		{
			//˵��ǰһҳ����ʹ���У�centralcacheû�л���pagecache
			break;
		}

		//�ϲ�
		//ǰһҳ�����ң�usecount = 0
		//����ϲ�������128ҳ��span���ǾͲ�Ҫ�ϲ�
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


	//���ϲ�
	while (1)
	{
		PAGE_ID nextPageId = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageId);
		if (nextIt == _idSpanMap.end())
			break;


		
		Span* nextSpan = nextIt->second;
		if (nextSpan->_usecount != 0)
			break;

		//�ϲ�
		//��һҳ�����ң�usecount = 0
		//����ϲ�������128ҳ��span���ǾͲ�Ҫ�ϲ�
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