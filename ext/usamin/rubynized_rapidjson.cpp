#include "rubynized_rapidjson.hpp"
#include <ruby.h>

void *RubyCrtAllocator::Malloc(size_t size) {
    if (size)
        return ruby_xmalloc(size);
    else
        return nullptr;
}

void *RubyCrtAllocator::Realloc(void *originalPtr, size_t, size_t newSize) {
    if (newSize == 0) {
        ruby_xfree(originalPtr);
        return nullptr;
    }
    return ruby_xrealloc(originalPtr, newSize);
}

void RubyCrtAllocator::Free(void *ptr) {
    ruby_xfree(ptr);
}
