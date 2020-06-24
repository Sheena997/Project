#pragma once
//���ù�����ͷ�ļ��������ģ��

#include <iostream>
#include <assert.h>
#include <windows.h>
#include <unordered_map>
#include <thread>
#include <mutex>

//��const����ö������꣬��Ϊ�겻֧�ֵ���
const size_t MAXSIZE = 64 * 1024;
const size_t NFREELIST = MAXSIZE / 8;//��8����
const size_t MAXPAGES = 129;
const size_t PAGESHIFT = 12;//4KΪҳ����λ


							//�ú�����Ƶ�����ã���������
inline void*& NextObj(void* obj)
{
	//ȡobj����ÿռ��ͷ�ĸ��ֽڻ����ͷ�˸��ֽ�
	return *((void**)obj);
}

class FreeList
{
public:
	void Push(void* obj)
	{
		//ͷ��
		NextObj(obj) = _freelist;
		_freelist = obj;
		++_num;
	}
	//����һ�η�Χ
	void PushRange(void* start, void* end, size_t num)
	{
		NextObj(end) = _freelist;
		_freelist = start;
		_num += num;
	}

	void* Pop()
	{
		//ͷɾ
		//_freelist���ľ��ǵڶ����ռ�ĵ�ַ���ѵڶ����ռ��ַ����obj�д洢
		void* obj = _freelist;
		_freelist = NextObj(obj);//ȡobjͷ���ĸ��ֽ�/�˸��ֽ�
		--_num;
		return obj;
	}

	//����ʵ�ʸ��˶��ٸ�
	size_t PopRange(void*& start, void*& end, size_t num)
	{
		size_t actualNum = 0;
		void* prev = nullptr;
		void* cur = _freelist;
		for (; actualNum < num && cur != nullptr; ++actualNum)
		{
			prev = cur;
			cur = NextObj(cur);
		}
		start = _freelist;
		end = prev;
		_freelist = cur;
		_num -= actualNum;

		return actualNum;
	}

	bool Empty()
	{
		return _freelist == nullptr;
	}
	size_t Num()
	{
		return _num;
	}
	void Clear()
	{
		_freelist = nullptr;
		_num = 0;
	}
private:
	void* _freelist = nullptr;//ָ��,����ȱʡֵ��û��д���캯�������Ի����ȱʡֵ����Ĭ�ϵĹ���
	size_t _num = 0;
};




//�����ݵ���
class SizeClass
{
public:
	//thread cache�е������еĶ�������align
	/*
	���align = 8 ��align_shift = 3 ��8 = 2^3
	������ǵڼ����ټ�һ
	��freelist�����һ������ҵ�ȫ�Ǵ�С8���ڴ�飬�ڶ�������ҵ�ȫ�Ǵ�СΪ16���ڴ��...
	�����������±�
	*/
	static size_t _FreeListIndex(size_t size, int align_shift)
	{
		//<<���ǳ�
		//>>���ǳ�
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	//�ж����ڵڼ�����������
	static size_t FreeListIndex(size_t size)
	{
		assert(size <= MAXSIZE);

		//ÿ�������ж��ٸ���
		static int group_array[4] = { 16, 56, 56, 56 };
		if (size <= 128)//128��Χ������2 ^ 3 = 8�ֽ�Ϊ��λ�����
			return _FreeListIndex(size, 3);
		else if (size <= 1024)//129-1024�Ƿ�Χ������2 ^ 4 = 16�ֽ�Ϊ��λ�����
			return  _FreeListIndex(size - 128, 4) + group_array[0];//-128����Ϊ0-128byte����8�ֽ�Ϊ��λ�����
		else if (size <= 8192)//1025=8192�ķ�Χ����2 ^7 = 128�ֽ�Ϊ��λ�����
			return _FreeListIndex(size - 1024, 7) + group_array[1] + group_array[0];
		else if (size <= 65536)//8192-65536�ķ�Χ����2 ^ 10 = 1024�ֽ�Ϊ��λ�����
			return _FreeListIndex(size - 8192, 10) + group_array[2] + group_array[1] + group_array[0];

		return -1;
	}


	//�ֽ�
	//RoundUp(size)��ThreadCache��CentralCache�����ڴ��ʱ���õ�
	//ԭ���еĸò��ֵĴ���
	//align��������
	/*
	��Ҫ��align = 8
	[9 - 16] + 7 = [16 - 23](�����Χ��ֵ16��λΪ1��ʣ�µ���λ���ղ�ͬ����ֵ��1) -> 16 8 4 2 1 ===��Ҫ�붼���16����16��λ��1���䣬����λ�����0�����ǽ�����λ�����0
	���Ҫ��align = 16
	[17 - 32] + 15 = [32 - 47](�����Χ��ֵ32���λΪ1��ʣ�µ���λ�������Ϊ1������) -> 32 16 8 4 2 1  ===��Ҫ���õ���λ��Ϊ0
	*/
	static size_t _RoundUp(size_t size, int align)
	{
		return (size + align - 1) & (~(align - 1));
	}
	/*
	������[1%, 11%]���ҵ�����Ƭ�˷�
	//��һ������Ǹ�4byte�Ļ�����64λ�¾ͻ������治��
	[1, 128]��8byte���루1byte�ڴ浽128byte���ڴ棩�� freeList[0��16��      ����16����������----->�˷��� 7 / 16 =
	[129, 1024]��16byte���룬 freeList[16��72�� 56��   ����56����������----->�˷��� 15 / 144��128 + 16�� = 10.41%
	[1025, 8*1024]��128byte���룬 freeList[72��128�� ����56����������->�˷���  127 / 1152��1024 + 128��= 11.02%
	[8*024+1, 64*1024]��1024byte���룬 free List[128��184�� ����56����������
	*/
	static size_t RoundUp(size_t size)
	{
		assert(size <= MAXSIZE);

		if (size <= 128)
			return _RoundUp(size, 8);
		else if (size <= 1024)
			return _RoundUp(size, 16);
		else if (size <= 8192)
			return _RoundUp(size, 128);
		else if (size <= 65536)
			return _RoundUp(size, 1024);

		return -1;
	}


	//����
	//�ŵ���[2,512]��֮�䣬�������central cache����ıȽ�С��һ�ξ͸���Ƚϴ���ڴ�
	//�����central cache������ڴ�Ƚϴ������Ƶ�512���ڴ棨�����պ�central cache��page cache������4Kһ��ҳ��
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		int num = MAXSIZE / size;//sizeԽ��numԽС��sizeԽС��numԽ��
		if (num < 2)
			num = 2;

		if (num > 512)//������4kһҳ
			num = 512;

		return num;
	}
	//��ҳ ����
	//����central cacheһ����page cacheҪ��ҳ��span
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num * size;//���ֽ���

								  //һҳ4k
		npage >>= PAGESHIFT;//����12λ = ����4k = ����ҳ��ҳ��
		if (npage == 0)
			npage = 1;//����Ҫһҳ��

		return npage;
	}
};





//span���   ����ҳΪ��λ���ڴ���󣬱����Ƿ������ϲ�������ڴ���Ƭ����
//64λ  64 / 2^12 == 2^52
//���Windows
#ifdef _WIN32
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;
#endif

struct Span
{
	//ȱʡֵ��ʼ����Ĭ�����ɵĹ��캯�������ȱʡֵ��ʼ����
	PAGE_ID _pageid = 0; //ҳ��
	PAGE_ID _pagesize = 0; //ҳ������

	FreeList _freelist; //������������
	int _usecount = 0; //�ڴ�����ʹ�ü���

	size_t _objsize = 0; //�����С

	Span* _next = nullptr;
	Span* _prev = nullptr;
};
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Insert(Span* pos, Span* newspan)
	{
		//ͷ��
		Span* prev = pos->_prev;

		prev->_next = newspan;
		newspan->_next = pos;
		pos->_prev = newspan;
		newspan->_prev = prev;
	}

	void Erase(Span* pos)
	{
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	void PushFront(Span* newspan)
	{
		Insert(_head->_next, newspan);
	}

	void PopFront()
	{
		Erase(_head->_next);
	}

	void PushBack(Span* newspan)
	{
		Insert(_head, newspan);
	}

	void PopBack()
	{
		Erase(_head->_prev);
	}

	bool Empty()
	{
		return Begin() == End();
	}

	void Lock()
	{
		_mtx.lock();
	}
	void Unlock()
	{
		_mtx.unlock();
	}
private:
	Span* _head;
	std::mutex _mtx;
};

//��ϵͳ����һ��128ҳ���ڴ�
inline static void* SystemAllocate(size_t numpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, numpage * (1 << PAGESHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//brk mmap��
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();//����ʧ�����쳣
	return ptr;
}

//���ڴ��ͷŸ�ϵͳ
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	//brk mmap��
#endif 
}