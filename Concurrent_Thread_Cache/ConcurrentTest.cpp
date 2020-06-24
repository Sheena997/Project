#include "ConcurrentTools.h"
#include <vector>

void MallocateTest(size_t useTime, size_t useWork, size_t rounds)
{
	std::vector<std::thread> vthread(useWork);
	size_t malloc_costtime = 0;
	size_t free_costtime = 0;
	for (size_t k = 0; k < useWork; ++k)
	{
		vthread[k] = std::thread([&, k]() {
			std::vector<void*> v;
			v.reserve(useTime);
			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < useTime; i++)
				{
					v.push_back(malloc(16));
				}
				size_t end1 = clock();
				size_t begin2 = clock();
				for (size_t i = 0; i < useTime; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();
				v.clear();
				malloc_costtime += end1 - begin1;
				free_costtime += end2 - begin2;
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�malloc %u��: ���ѣ�%u ms\n",
		useWork, rounds, useTime, malloc_costtime);
	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�free %u��: ���ѣ�%u ms\n",
		useWork, rounds, useTime, free_costtime);
	printf("%u���̲߳���malloc&free %u�Σ��ܼƻ��ѣ�%u ms\n",
		useWork, useWork*rounds*useTime, malloc_costtime + free_costtime);
}
// ���ִ������ͷŴ��� �߳��� �ִ�
void ConcurrentMallocateTest(size_t useTime, size_t useWork, size_t rounds)
{
	std::vector<std::thread> vthread(useWork);
	size_t malloc_costtime = 0;
	size_t free_costtime = 0;
	for (size_t k = 0; k < useWork; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(useTime);
			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < useTime; i++)
				{
					v.push_back(ConcurrentMallocate(16));
				}
				size_t end1 = clock();
				size_t begin2 = clock();
				for (size_t i = 0; i < useTime; i++)
				{
					ConcurrentFree(v[i]);
				}
				size_t end2 = clock();
			

				v.clear();
				malloc_costtime += end1 - begin1;
				free_costtime += end2 - begin2;
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent alloc %u��: ��ʱ��%u ms\n",
		useWork, rounds, useTime, malloc_costtime);
	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent dealloc %u��: ��ʱ��%u ms\n",
		useWork, rounds, useTime, free_costtime);
	printf("%u���̲߳���concurrent alloc&dealloc %u�Σ��ܼƺ�ʱ��%u ms\n",
		useWork, useWork*rounds*useTime, malloc_costtime + free_costtime);
}
int main()
{
	std::cout << std::endl << std::endl;
	std::cout << "���е�malloc��" << std::endl;
	MallocateTest(10000, 4, 10);
	std::cout << std::endl << std::endl;
	std::cout << "����Ŀ��ConcurrentMalloc��" << std::endl;
	ConcurrentMallocateTest(10000, 4, 10);
	std::cout << std::endl << std::endl;
	return 0;
}
