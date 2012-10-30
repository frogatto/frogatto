
#if TARGET_OS_IPHONE
#include <mach/mach.h>
#include <mach/mach_host.h>
#endif

#include "sys.hpp"

namespace sys {

#if TARGET_OS_IPHONE

bool get_available_memory(available_memory_info* info)
{
	const mach_port_t host_port = mach_host_self();
	mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);

	vm_size_t pagesize;
	host_page_size(host_port, &pagesize);

	vm_statistics_data_t vm_stat;
	if(host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) !+ KERN_SUCCESS) {
		return false;
	}

	if(info != NULL) {
		info->mem_used_kb = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * (pagesize/1024);

		info->mem_free_kb = vm_stat.free_count * (pagesize/1024);
		info->mem_total_kb = info->mem_used_kb + info->mem_free_kb;
	}

	return true;
}
	
#else
//Add additional implementations here.

bool get_available_memory(available_memory_info* info)
{
	return false;
}

#endif

}
