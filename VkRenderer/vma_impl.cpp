#define VMA_IMPLEMENTATION

/**/ // For DEBUG MEM LEAKS// /**/

// #ifndef VMA_DEBUG_LOG_FORMAT
// #de fine  VMA_DEBUG_LOG_FORMAT(format, ...) do { \
//       printf((format), __VA_ARGS__); \
//       printf("\n"); \
//   } while(false)
// #endif
//
// #ifndef VMA_DEBUG_LOG
// #ifndef VMA_ENABLE_DEBUG_LOG
///// VMA debug log disabled
// #define VMA_DEBUG_LOG(str)
// #else
///// VMA debug log enabled
// #define VMA_DEBUG_LOG(str)   VMA_DEBUG_LOG_FORMAT("%s", (str))
// #endif
// #endif

#include "vk_mem_alloc.h"
