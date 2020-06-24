#include "ThreadCache.h"
#include "Common.h"
#include "ConcurrentTools.h"
#include <vector>


void UnitThreadCache1()
{
	ThreadCache tc;
	void* ptr1 = tc.Allocate(6);
	printf("[%d]=>%p\n", 1, ptr1);
	void* ptr2 = tc.Allocate(6);
	printf("[%d]=>%p\n", 2, ptr2);
	void* ptr3 = tc.Allocate(6);
	printf("[%d]=>%p\n", 2, ptr2);
	void* ptr4 = tc.Allocate(6);
	printf("[%d]=>%p\n", 2, ptr2);
}
void UnitThreadCache2()
{
	ThreadCache tc;
	std::vector<void*> v;
	size_t size = 7;
	for (size_t i = 0; i < SizeClass::NumMoveSize(size); ++i)
		v.push_back(tc.Allocate(size));
	v.push_back(tc.Allocate(size));//��һҳ��
	for (size_t i = 0; i < v.size(); ++i)
		printf("[%d]=>%p\n", i, v[i]);


	for (auto ptr : v)
		tc.Deallocate(ptr, size);

	v.clear();
	v.push_back(tc.Allocate(size));
}


void UnitTestSizeClass()
{
	/*
	������[1%, 11%]���ҵ�����Ƭ�˷�
	//��һ������Ǹ�4byte�Ļ�����64λ�¾ͻ������治��
	[1, 128]��8byte���룬 freeList[0��16��      16������----->�˷��� 7 / 16 =
	[129, 1024]��16byte���룬 freeList[16��72�� 56������----->�˷��� 15 / 144��128 + 16�� = 10.41%
	[1025, 8*1024]��128byte���룬 freeList[72��128�� 56������->�˷���  127 / 1152��1024 + 128��= 11.02%
	[8*024+1, 64*1024]��1024byte���룬 free List[128��184�� 56������
	*/
	std::cout << "RoundUp���ԣ�" << std::endl;
	std::cout << SizeClass::RoundUp(1) << std::endl;
	std::cout << SizeClass::RoundUp(127) << std::endl;
	std::cout << std::endl;
	std::cout << SizeClass::RoundUp(129) << std::endl;
	std::cout << SizeClass::RoundUp(1023) << std::endl;
	std::cout << std::endl;
	std::cout << SizeClass::RoundUp(1025) << std::endl;
	std::cout << SizeClass::RoundUp(8 * 1024 - 1) << std::endl;
	std::cout << std::endl;
	std::cout << SizeClass::RoundUp(8 * 1024 + 1) << std::endl;
	std::cout << SizeClass::RoundUp(64 * 1024 - 1) << std::endl;
	std::cout << std::endl;


	std::cout << "FreeListIndex���ԣ�" << std::endl;
	std::cout << SizeClass::FreeListIndex(1) << std::endl;
	std::cout << SizeClass::FreeListIndex(128) << std::endl;
	std::cout << std::endl;
	std::cout << SizeClass::FreeListIndex(129) << std::endl;
	std::cout << SizeClass::FreeListIndex(1023) << std::endl;
	std::cout << std::endl;
	std::cout << SizeClass::FreeListIndex(1025) << std::endl;
	std::cout << SizeClass::FreeListIndex(8 * 1024 - 1) << std::endl;
	std::cout << std::endl;
	std::cout << SizeClass::FreeListIndex(8 * 1024 + 1) << std::endl;
	std::cout << SizeClass::FreeListIndex(64 * 1024 - 1) << std::endl;
	std::cout << std::endl;

}



void UnitSystemAllocate()
{
	void* ptr1 = SystemAllocate(1);
	void* ptr2 = SystemAllocate(1);

	PAGE_ID id1 = (PAGE_ID)ptr1 >> PAGESHIFT;//����12λ������4k������ͨ����ַ��ҳ��
	void* ptrshift1 = (void*)(id1 << PAGESHIFT);//����12λ����4k��ͨ��ҳ�����ַ

	PAGE_ID id2 = (PAGE_ID)ptr2 >> PAGESHIFT;//����12λ������4k������ͨ����ַ��ҳ��
	void* ptrshift2 = (void*)(id2 << PAGESHIFT);//����12λ����4k��ͨ��ҳ�����ַ

	char* obj1 = (char*)ptr1;
	char* obj2 = (char*)ptr1 + 8;
	char* obj3 = (char*)ptr1 + 16;
	PAGE_ID id4 = (PAGE_ID)obj1 >> PAGESHIFT;//ͨ��ҳ�ڵĵ�ַ���ҳ��
	PAGE_ID id5 = (PAGE_ID)obj2 >> PAGESHIFT;
	PAGE_ID id6 = (PAGE_ID)obj3 >> PAGESHIFT;
}



void f1(size_t n)
{
	std::vector<void*> v;
	size_t size = 7;
	for (size_t i = 0; i < SizeClass::NumMoveSize(size); ++i)
		v.push_back(ConcurrentMallocate(size));

	for (auto ptr : v)
		ConcurrentFree(ptr);

	v.clear();;

}
void f2(size_t n)
{
	std::vector<void*> v;
	size_t size = 7;
	for (size_t i = 0; i < SizeClass::NumMoveSize(size); ++i)
		v.push_back(ConcurrentMallocate(size));
	
	for (auto ptr : v)
		ConcurrentFree(ptr);

	v.clear();
}
void UnitConcurrent()
{
	std::thread t1(f1, 100);
	std::thread t2(f2, 100);

	t1.join();
	t2.join();
}

void UnitConcurrent2()
{
	void* ptr1 = ConcurrentMallocate(1 << PAGESHIFT);
	void* ptr2 = ConcurrentMallocate(65 << PAGESHIFT);
	void* ptr3 = ConcurrentMallocate(129 << PAGESHIFT);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
	ConcurrentFree(ptr3);
}
//int main()
//{
//	//UnitThreadCache1();
//	//UnitThreadCache2();
//	//UnitTestSizeClass();
//	UnitSystemAllocate();
//	//UnitConcurrent();
//	//UnitConcurrent2();
//	
//
//	return 0;
//}