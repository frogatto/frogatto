/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
	if(host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS) {
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
