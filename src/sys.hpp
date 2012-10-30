#ifndef SYS_HPP_INCLUDED
#define SYS_HPP_INCLUDED

namespace sys {
struct available_memory_info {
	int mem_used_kb, mem_free_kb, mem_total_kb;
};

bool get_available_memory(available_memory_info* info);
}

#endif
