// MyNew.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "MyNew.h"
#include <ctime>

#define FILENAME	"MemoryLeak.dat"
#define MAX_LEN		0x100

std::mutex g_mutex;
CMemoryLeak::CMemoryFree CMemoryLeak::m_mem_free;

struct MemBlock
{
	void *p_point;
	const char* p_file;
	unsigned int line;
	size_t nsize;
	MemBlock *p_link;
};

CMemoryLeak::CMemoryLeak()
{
}

CMemoryLeak::~CMemoryLeak()
{
	if(nullptr != m_pfp)
		m_pfp = nullptr;
	
	print_out();
}

CMemoryLeak::CMemoryFree::~CMemoryFree()
{
	if (nullptr != m_instance.load())
		free(m_instance.load());
}

std::atomic<CMemoryLeak*> CMemoryLeak::m_instance;
std::mutex CMemoryLeak::m_mutex;

CMemoryLeak * CMemoryLeak::get_instance()
{
	CMemoryLeak *temp = m_instance.load(std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_acquire);
	if (nullptr == temp)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		temp = m_instance.load(std::memory_order_relaxed);
		if (nullptr == temp)
		{
			temp = (CMemoryLeak *)malloc(sizeof(CMemoryLeak));
			temp->init();
			
			std::atomic_thread_fence(std::memory_order_release);
			m_instance.store(temp, std::memory_order_relaxed);
		}
	}

	return temp;
}

void CMemoryLeak::init()
{
	m_pmemory_list = nullptr;
	time_t start_time = time(NULL);
	tm* p_tm = localtime(&start_time);
	m_pfp = fopen(FILENAME, "a+");
	if (nullptr == m_pfp)
		printf("Open file error!\n");

	fputs("启动时间：", m_pfp);
	fputs(asctime(p_tm), m_pfp);
	fclose(m_pfp);
}

void CMemoryLeak::insert(void * pointer, const size_t & nsize, const char * pfile, const unsigned int nline)
{
	g_mutex.lock();

	MemBlock *mem_node = (MemBlock*)malloc(sizeof(MemBlock));
	mem_node->nsize = nsize;
	mem_node->p_file = pfile;
	mem_node->p_point = pointer;
	mem_node->line = nline;
	mem_node->p_link = m_pmemory_list;

	write_malloc_to_file(mem_node);

	m_pmemory_list = mem_node;

	g_mutex.unlock();
}

void CMemoryLeak::remove(void * pointer)
{
	g_mutex.lock();

	MemBlock *node = m_pmemory_list;
	if (node == nullptr)
	{
		char str[MAX_LEN] = { 0 };
		sprintf(str, "Invalid address,%0X\n", pointer);
		write_info(str);

		return;
	}

	MemBlock *pre = nullptr;
	while (node !=nullptr && node->p_point!=pointer)
	{
		pre = node;
		node = node->p_link;
	}

	if (nullptr != node)
	{
		if (nullptr == pre)
			m_pmemory_list = node->p_link;
		else
			pre->p_link = node->p_link;

		write_free_to_file(node);

		free(node);
		node = nullptr;
	}

	g_mutex.unlock();
}

void CMemoryLeak::print_out()
{
	g_mutex.lock();

	if (nullptr == m_pmemory_list)
	{
		write_info("No mempry leaks!\n");
		return;
	}

	write_info("Have memory leaks!\n");
	MemBlock *p_node = m_pmemory_list;
	while (nullptr != p_node)
	{
		write_leak_to_file(p_node);
		p_node = p_node->p_link;
	}

	g_mutex.unlock();
}

void CMemoryLeak::split_file_name(char * &pfile)
{
	int nlength = strlen(pfile);
	for (int i = nlength; i >= 0; i--)
	{
		if (pfile[i] != '\\')
			continue;
		else
		{
			pfile = pfile + i + 1;
			break;
		}
	}
}

void CMemoryLeak::write_malloc_to_file(const MemBlock * node)
{
	m_pfp = fopen(FILENAME, "a+");
	if (nullptr == m_pfp)
		printf("Open file error!\n");

	split_file_name(const_cast<char*&>(node->p_file));

	char str[MAX_LEN] = { 0 };
	sprintf(str, "Memory malloc--address:%0X,size:%d,file:%s,line:%d.\n",
		node->p_point, node->nsize, node->p_file, node->line);

	fputs(str, m_pfp);
	fclose(m_pfp);

}

void CMemoryLeak::write_free_to_file(const MemBlock * node)
{
	m_pfp = fopen(FILENAME, "a+");
	if (nullptr == m_pfp)
		printf("Open file error!\n");
	
	split_file_name(const_cast<char*&>(node->p_file));

	char str[MAX_LEN] = { 0 };
	sprintf(str, "Memory free--address:%0X,size:%d,file:%s,line:%d.\n",
		node->p_point, node->nsize, node->p_file, node->line);

	fputs(str, m_pfp);
	fclose(m_pfp);
}

void CMemoryLeak::write_leak_to_file(const MemBlock * node)
{
	m_pfp = fopen(FILENAME, "a+");
	if (nullptr == m_pfp)
		printf("Open file error!\n");

	split_file_name(const_cast<char*&>(node->p_file));

	char str[MAX_LEN] = { 0 };
	sprintf(str, "Memory leak--address:%0X,size:%d,file:%s,line:%d.\n",
		node->p_point, node->nsize, node->p_file, node->line);

	fputs(str, m_pfp);
	fclose(m_pfp);
}

void CMemoryLeak::write_info(const char * p_str)
{
	m_pfp = fopen(FILENAME, "a+");
	if (nullptr == m_pfp)
		printf("Open file error!\n");

	fputs(p_str, m_pfp);
	fclose(m_pfp);
}

void * operator new(unsigned int nsize, char * file, unsigned int line)
{
	void *p = malloc(nsize);
	CMemoryLeak::get_instance()->insert(p, nsize, file, line);

	return p;
}

void * operator new[](unsigned int nsize, char * file, unsigned int line)
{
	return operator new(nsize,file,line);
}

void operator delete(void * ptr)
{
	free(ptr);
	CMemoryLeak::get_instance()->remove(ptr);
}

void operator delete[](void * ptr)
{
	operator delete(ptr);
}


#define TEST_NEW new(__FILE__,__LINE__)
#define new TEST_NEW

int main()
{

	char *p = new char[20];


	CMemoryLeak::get_instance()->print_out();
	return 0;
}