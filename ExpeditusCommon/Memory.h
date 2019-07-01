#pragma once

#include <memory>

class Memory
{
public:
	static const uint64_t LogPageSize = 16;
	static const uint64_t PageSize = 1 << LogPageSize;
	static Memory& instance();

	// Set memory limit in pages (default: 8GB)
	void set_memory_limit(uint32_t limit = 1 << 17);

	void* alloc(size_t size);
	void free(void* data);

private:
	Memory();
	~Memory();

	struct Impl;
	std::unique_ptr<Impl> mPImpl;
};

