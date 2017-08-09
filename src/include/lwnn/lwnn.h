
#pragma once

#include <gc.h>
#include <gc/gc_allocator.h>
#include <gc/gc_cpp.h>


//To disable GC temporarily, comment out the #includes above and uncomment the 4 lines below.
//class gc { };
//class gc_cleanup { };
//#define GC_collect_a_little() (false);
//#define GC_INIT();


//Uncomment to enable GC for everything... currently makes LLVM's assertions fail for some unknown reason.
//inline void * operator new(size_t n)  {
//    return GC_malloc(n);
//}
//
//inline void * operator new[](size_t n) {
//    return GC_malloc(n);
//}
//inline void operator delete(void *) { }
//inline void operator delete(void *, size_t) { }
//inline void operator delete[](void *)  { }
//inline void operator delete[](void *, size_t ) { }

namespace lwnn {

class no_copy {
public:
    // no copy
    no_copy(const no_copy &) = delete;

    no_copy() {}
};

class no_assign {
public:
    // no assign
    no_assign &operator=(const no_assign &) = delete;
};

}
