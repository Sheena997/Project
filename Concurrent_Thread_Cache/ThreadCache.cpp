#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t size)
{
	size_t index = SizeClass::FreeListIndex(size);
	FreeList& freelist = _freeLists[index];
	if (!freelist.Empty())
	{
		//�����������ڴ�
		return freelist.Pop();
	}
	else
	{
		//���threadcache�ж�Ӧ��freelists��û�ж���
		//���ȥcentralcache�����ڴ棬ʹ����Ҫ�ģ�ʣ�µĹҵ�freelist������
		//[1, 128]��8byte���루1byte�ڴ浽128byte���ڴ棩�����ڴ���Ƭ
		//�ֽڴ�С
		return FetchFromCentralCache(SizeClass::RoundUp(size));
	}
}
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	//�ͷ��ڴ棬���ǰѹһ�������
	size_t index = SizeClass::FreeListIndex(size);
	FreeList& freeList = _freeLists[index];
	freeList.Push(ptr);

	//�����������һ������ ���� �ڴ��С�����������Ϳ����ͷŵ�central cache��
	size_t num = SizeClass::NumMoveSize(size);
	if (freeList.Num() >= num)
	{
		//�������������ͷŻ����Ļ���
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

//����num���ڴ棬����һ����ʹ��һ������ʣ�µ�num-1������ThreadCache��Ӧ��������
//����size��С���ڴ棬 ����Ҫ���ж��룬���������ʵ����Ҫ5���ڴ棬��central cacheҪ8���ڴ�
//�����Ҫ8����С�ڴ棬����central cacheҪ8����С�ڴ�
void* ThreadCache::FetchFromCentralCache(size_t size)
{
	//�ŵ���[2,512]��֮�䣬�������central cache����ıȽ�С��һ�ξ͸���Ƚϴ���ڴ�
	//num������������ٸ�span����
	size_t num = SizeClass::NumMoveSize(size);

	//��Ҫ���ٸ�����һ�����㣬�����и�ʵ�ʸ���ĸ���actualNum
	void* start = nullptr, *end = nullptr;
	size_t actualNum = CentralCache::GetInstance().FetchRangeObj(start, end, num, size);

	if (actualNum == 1)
	{
		//���ֻ��һ������
		return start;
	}
	else
	{
		//centralcache����һ����һ�����ϵ� ����
		size_t index = SizeClass::FreeListIndex(size);
		FreeList& list = _freeLists[index];
		//�ж������һ�����û���ʣ�µĹ��������ҵ�threadcache�ж�Ӧ��index��������
		list.PushRange(NextObj(start), end, actualNum - 1);

		return start;
	}


}