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
	printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
		useWork, rounds, useTime, malloc_costtime);
	printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
		useWork, rounds, useTime, free_costtime);
	printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
		useWork, useWork*rounds*useTime, malloc_costtime + free_costtime);
}
// 单轮次申请释放次数 线程数 轮次
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
	printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 耗时：%u ms\n",
		useWork, rounds, useTime, malloc_costtime);
	printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 耗时：%u ms\n",
		useWork, rounds, useTime, free_costtime);
	printf("%u个线程并发concurrent alloc&dealloc %u次，总计耗时：%u ms\n",
		useWork, useWork*rounds*useTime, malloc_costtime + free_costtime);
}
int main()
{
	std::cout << std::endl << std::endl;
	std::cout << "库中的malloc：" << std::endl;
	MallocateTest(10000, 4, 10);
	std::cout << std::endl << std::endl;
	std::cout << "本项目的ConcurrentMalloc：" << std::endl;
	ConcurrentMallocateTest(10000, 4, 10);
	std::cout << std::endl << std::endl;
	return 0;
}
