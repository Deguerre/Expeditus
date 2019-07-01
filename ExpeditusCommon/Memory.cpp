#include "pch.h"
#include "Memory.h"


// Logarithm of the address space size, in pages.
// With 64k pages, 2^24 of these is 1TB.

#define LOG_ADDRESS_SPACE_SIZE	24

enum PageType {
	PAGE_FREE = 0,
	PAGE_RESERVED,
	PAGE_SMALLOBJ,
	PAGE_LARGEOBJ
};

struct Page {
	uint32_t next;
	uint32_t
		page_type : 4;
};

struct Memory::Impl {
	char* vspace = 0;
	Page* coremap = 0;
	uint32_t free_list = ~(uint32_t)0;
	uint32_t total_pages = 0;
	uint32_t pages_free = 0;

	void alloc_pages(uint32_t base_page, uint32_t size, PageType type)
	{
		DWORD oldProtect;
		::VirtualProtect((LPVOID)&vspace[(size_t)base_page << LogPageSize], (SIZE_T)size << LogPageSize, MEM_COMMIT, &oldProtect);
	}

	void free_pages(uint32_t base_page, uint32_t size)
	{
		DWORD oldProtect;
		::VirtualProtect((LPVOID)& vspace[(size_t)base_page << LogPageSize], (SIZE_T)size << LogPageSize, MEM_RESET, &oldProtect);
		for (unsigned i = base_page; i < base_page + size; ++i) {
			coremap[i].page_type = PAGE_FREE;
		}
	}

	Impl()
	{
		vspace = (char*)::VirtualAlloc(0, (SIZE_T)PageSize << LOG_ADDRESS_SPACE_SIZE, MEM_RESERVE, 0);
		set_memory_limit(64);
	}

	void set_memory_limit(uint32_t limit)
	{
		free_pages(0, 1 << LOG_ADDRESS_SPACE_SIZE);
		uint32_t coremap_size = (uint32_t)((sizeof(Page) * limit + PageSize - 1) >> LogPageSize);
		alloc_pages(0, coremap_size, PAGE_RESERVED);
		total_pages = limit;
		coremap = (Page*)vspace;
		pages_free = total_pages - coremap_size;
		for (unsigned i = 0; i < coremap_size; ++i) {
			coremap[i].page_type = PAGE_RESERVED;
		}
		for (unsigned i = coremap_size; i < (unsigned)1 << LOG_ADDRESS_SPACE_SIZE; ++i) {
			coremap[i].page_type = PAGE_FREE;

		}
	}
};


Memory& Memory::instance()
{
	static Memory memory;
	return memory;
}

void Memory::set_memory_limit(uint32_t limit)
{
	mPImpl->set_memory_limit(limit);
}

Memory::Memory()
	: mPImpl(std::make_unique<Impl>())
{
}


Memory::~Memory()
{
}


void*
Memory::alloc(size_t size)
{
	return _aligned_malloc(size, 16);
}


void
Memory::free(void* data)
{
	_aligned_free(data);
}
