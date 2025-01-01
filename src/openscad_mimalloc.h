#pragma once

#ifdef USE_MIMALLOC

#if 0 // defined(_WIN32) && defined(MI_LINK_SHARED)
  // mimalloc doesn't support static override of malloc on Windows.
  // This include causes crashes if mimalloc is statically linked.
  #include <mimalloc-new-delete.h>
#else
  #include <mimalloc.h>
#endif

#if defined(ENABLE_CGAL)
#include <cstddef>
// gmp requires function signature with extra oldsize parameters for some reason.
inline void *gmp_realloc(void *ptr, size_t /*oldsize*/, size_t newsize) { return mi_realloc(ptr, newsize); }
inline void gmp_free(void *ptr, size_t /*oldsize*/) { mi_free(ptr); }
  #include <gmp.h>
inline void init_mimalloc() { mp_set_memory_functions(mi_malloc, gmp_realloc, gmp_free); }
#endif // ENABLE_CGAL

#endif // USE_MIMALLOC
