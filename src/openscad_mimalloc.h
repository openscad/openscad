#pragma once

#ifdef USE_MIMALLOC

#include <mimalloc-new-delete.h>

#ifdef ENABLE_CGAL
// gmp requires function signature with extra oldsize parameters for some reason.
inline void *gmp_realloc (void *ptr, size_t /*oldsize*/, size_t newsize) { return mi_realloc(ptr, newsize); }
inline void gmp_free (void *ptr, size_t /*oldsize*/) { mi_free(ptr); }
#include <gmp.h>
inline void init_mimalloc() { mp_set_memory_functions(mi_malloc, gmp_realloc, gmp_free); }
#endif // ENABLE_CGAL

#endif // USE_MIMALLOC
