#pragma once
//放置公共的头文件、代码和模块

#include <iostream>
#include <assert.h>
#include <windows.h>
#include <unordered_map>
#include <thread>
#include <mutex>

//用const或者枚举替代宏，因为宏不支持调试
const size_t MAXSIZE = 64 * 1024;
const size_t NFREELIST = MAXSIZE / 8;//以8对齐
const size_t MAXPAGES = 129;
const size_t PAGESHIFT = 12;//4K为页的移位


							//该函数被频繁调用，给成内联
inline void*& NextObj(void* obj)
{
	//取obj对象该空间的头四个字节或这个头八个字节
	return *((void**)obj);
}

class FreeList
{
public:
	void Push(void* obj)
	{
		//头插
		NextObj(obj) = _freelist;
		_freelist = obj;
		++_num;
	}
	//插入一段范围
	void PushRange(void* start, void* end, size_t num)
	{
		NextObj(end) = _freelist;
		_freelist = start;
		_num += num;
	}

	void* Pop()
	{
		//头删
		//_freelist里存的就是第二个空间的地址，把第二个空间地址放入obj中存储
		void* obj = _freelist;
		_freelist = NextObj(obj);//取obj头上四个字节/八个字节
		--_num;
		return obj;
	}

	//返回实际给了多少个
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
	void* _freelist = nullptr;//指针,给了缺省值，没有写构造函数，所以会调用缺省值进行默认的构造
	size_t _num = 0;
};




//算数据的类
class SizeClass
{
public:
	//thread cache中的链表中的对齐数：align
	/*
	如果align = 8 则align_shift = 3 即8 = 2^3
	算出你是第几个再减一
	即freelist链表第一个下面挂的全是大小8的内存块，第二个下面挂的全是大小为16的内存块...
	即是求链表下标
	*/
	static size_t _FreeListIndex(size_t size, int align_shift)
	{
		//<<：是乘
		//>>：是除
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	//判断属于第几个自由链表
	static size_t FreeListIndex(size_t size)
	{
		assert(size <= MAXSIZE);

		//每个区间有多少个链
		static int group_array[4] = { 16, 56, 56, 56 };
		if (size <= 128)//128范围内是以2 ^ 3 = 8字节为单位对齐的
			return _FreeListIndex(size, 3);
		else if (size <= 1024)//129-1024是范围内是以2 ^ 4 = 16字节为单位对齐的
			return  _FreeListIndex(size - 128, 4) + group_array[0];//-128是因为0-128byte是以8字节为单位对齐的
		else if (size <= 8192)//1025=8192的范围是以2 ^7 = 128字节为单位对齐的
			return _FreeListIndex(size - 1024, 7) + group_array[1] + group_array[0];
		else if (size <= 65536)//8192-65536的范围是以2 ^ 10 = 1024字节为单位对齐的
			return _FreeListIndex(size - 8192, 10) + group_array[2] + group_array[1] + group_array[0];

		return -1;
	}


	//字节
	//RoundUp(size)是ThreadCache向CentralCache申请内存的时候用的
	//原码中的该部分的代码
	//align：对齐数
	/*
	如要是align = 8
	[9 - 16] + 7 = [16 - 23](这个范围的值16这位为1，剩下的四位按照不同数的值是1) -> 16 8 4 2 1 ===》要想都变成16，则16该位是1不变，其余位都变成0，就是将低三位都变成0
	如果要是align = 16
	[17 - 32] + 15 = [32 - 47](这个范围的值32这个位为1，剩下的五位依次组合为1构成数) -> 32 16 8 4 2 1  ===》要想让低四位都为0
	*/
	static size_t _RoundUp(size_t size, int align)
	{
		return (size + align - 1) & (~(align - 1));
	}
	/*
	控制在[1%, 11%]左右的内碎片浪费
	//第一个如果是给4byte的话，，64位下就会出问题存不了
	[1, 128]以8byte对齐（1byte内存到128byte的内存）， freeList[0，16）      即有16个自由链表----->浪费率 7 / 16 =
	[129, 1024]以16byte对齐， freeList[16，72） 56个   即有56个自由链表----->浪费率 15 / 144（128 + 16） = 10.41%
	[1025, 8*1024]以128byte对齐， freeList[72，128） 即有56个自由链表->浪费率  127 / 1152（1024 + 128）= 11.02%
	[8*024+1, 64*1024]以1024byte对齐， free List[128，184） 则有56个自由链表
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


	//个数
	//放到了[2,512]个之间，如果你向central cache申请的比较小，一次就给你比较大的内存
	//如果向central cache申请的内存比较大，则会控制到512的内存（这样刚好central cache向page cache申请了4K一个页）
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		int num = MAXSIZE / size;//size越大，num越小；size越小，num越大
		if (num < 2)
			num = 2;

		if (num > 512)//最少是4k一页
			num = 512;

		return num;
	}
	//几页 个数
	//计算central cache一次向page cache要几页的span
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num * size;//总字节数

								  //一页4k
		npage >>= PAGESHIFT;//右移12位 = 除以4k = 多少页即页数
		if (npage == 0)
			npage = 1;//最少要一页的

		return npage;
	}
};





//span跨度   管理页为单位的内存对象，本质是方便做合并。解决内存碎片问题
//64位  64 / 2^12 == 2^52
//针对Windows
#ifdef _WIN32
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;
#endif

struct Span
{
	//缺省值初始化（默认生成的构造函数会调用缺省值初始化）
	PAGE_ID _pageid = 0; //页号
	PAGE_ID _pagesize = 0; //页的数量

	FreeList _freelist; //对象自由链表
	int _usecount = 0; //内存块对象使用计数

	size_t _objsize = 0; //对象大小

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
		//头插
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

//向系统申请一个128页的内存
inline static void* SystemAllocate(size_t numpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, numpage * (1 << PAGESHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//brk mmap等
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();//申请失败抛异常
	return ptr;
}

//将内存释放给系统
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	//brk mmap等
#endif 
}