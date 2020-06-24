#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t size)
{
	size_t index = SizeClass::FreeListIndex(size);
	FreeList& freelist = _freeLists[index];
	if (!freelist.Empty())
	{
		//该链表下有内存
		return freelist.Pop();
	}
	else
	{
		//如果threadcache中对应的freelists下没有对象
		//则会去centralcache申请内存，使用需要的，剩下的挂到freelist链表下
		//[1, 128]以8byte对齐（1byte内存到128byte的内存）控制内存碎片
		//字节大小
		return FetchFromCentralCache(SizeClass::RoundUp(size));
	}
}
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	//释放内存，就是把挂回链表中
	size_t index = SizeClass::FreeListIndex(size);
	FreeList& freeList = _freeLists[index];
	freeList.Push(ptr);

	//对象个数满足一定条件 或者 内存大小满足条件，就可以释放到central cache中
	size_t num = SizeClass::NumMoveSize(size);
	if (freeList.Num() >= num)
	{
		//如果链表过长就释放回中心缓存
		ListTooLong(freeList,num, size);
	}
	
}
 
void ThreadCache::ListTooLong(FreeList& freeList, size_t num, size_t size)
{
	void* start = nullptr, *end = nullptr;
	freeList.PopRange(start, end, num);

	NextObj(end) = nullptr;
	CentralCache::GetInstance().ReleaseListToSpans(start, size);
}

//申请num个内存，返回一个（使用一个），剩下的num-1个挂在ThreadCache对应的链表下
//申请size大小的内存， 但是要进行对齐，就是如果你实际想要5个内存，向central cache要8个内存
//如果想要8个大小内存，则向central cache要8个大小内存
void* ThreadCache::FetchFromCentralCache(size_t size)
{
	//放到了[2,512]个之间，如果你向central cache申请的比较小，一次就给你比较大的内存
	//num个数，申请多少个span对象
	size_t num = SizeClass::NumMoveSize(size);

	//想要多少个，不一定给你，所以有个实际给你的个数actualNum
	void* start = nullptr, *end = nullptr;
	size_t actualNum = CentralCache::GetInstance().FetchRangeObj(start, end, num, size);

	if (actualNum == 1)
	{
		//如果只有一个对象
		return start;
	}
	else
	{
		//centralcache里有一个或一个以上的 对象
		size_t index = SizeClass::FreeListIndex(size);
		FreeList& list = _freeLists[index];
		//有多个返回一个给用户，剩下的挂起来，挂到threadcache中对应的index的链表下
		list.PushRange(NextObj(start), end, actualNum - 1);

		return start;
	}


}