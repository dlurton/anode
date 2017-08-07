
#include "memory.h"

#include <cstdlib>

extern "C" {

void *allocate_object(int size) {
    return std::malloc(size);
}

void free_object(void *pointer) {
    std::free(pointer);
}

}