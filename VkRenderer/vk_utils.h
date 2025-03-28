#pragma once

#include <iostream>
#include <stddef.h>

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

/* 
# Function `align_up<integral>(x, a)`
Rounds `x` up to a multiple of `a`. `a` must be a power of two.
 */
template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
	return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

namespace {
	template <typename Fn> void run_with_mapped_memory(VmaAllocator allocator, VmaAllocation allocation, Fn&& fn)
	{
		void* mapped_memory;
		VK_CHECK(vmaMapMemory(allocator, allocation, &mapped_memory));
		fn(mapped_memory);
		vmaUnmapMemory(allocator, allocation);
	}
}

