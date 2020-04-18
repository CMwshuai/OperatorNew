#ifndef _MYNEW_H_
#define _MYNEW_H_

#include <cstdlib>
#include <atomic>
#include <mutex>
#include <cstdio>

extern struct MemBlock;
extern std::mutex g_mutex;

extern void *operator new(unsigned int nsize, char* file, unsigned int line);
extern void *operator new[](unsigned int nsize, char*file, unsigned int line);
extern void operator delete(void *ptr);
extern void operator delete[](void *ptr);

class CMemoryLeak 
{
public:
	CMemoryLeak();
	~CMemoryLeak();
	static CMemoryLeak* get_instance();

	class CMemoryFree
	{
	public:
		~CMemoryFree();
	};

	void init();
	void insert(void *pointer,const size_t &nsize, const char* pfile, const unsigned int nline);
	void remove(void *pointer);
	void print_out();
private:
	void split_file_name(char *&pfile);
	void write_malloc_to_file(const MemBlock *node);
	void write_free_to_file(const MemBlock *node);
	void write_leak_to_file(const MemBlock *node);
	void write_info(const char *p_str);
public:
	static std::atomic<CMemoryLeak*> m_instance;
	static std::mutex m_mutex;

	static CMemoryFree m_mem_free;
private:
	MemBlock *m_pmemory_list;
	FILE	*m_pfp;
};








#endif // 
