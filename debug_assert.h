
#ifndef SubmatrixQueries_debug_assert_h
#define SubmatrixQueries_debug_assert_h

#include <cassert>

#ifdef DEBUG
#define DEBUG_ASSERT(e) assert(e)
#else
#define DEBUG_ASSERT(e) 
#endif

#endif
