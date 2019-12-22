#ifndef USAMIN_RUBYNIZED_RAPIDJSON_HPP
#define USAMIN_RUBYNIZED_RAPIDJSON_HPP

#include <rapidjson/document.h>

class RubyCrtAllocator {
public:
    static const bool kNeedFree = true;
    void *Malloc(size_t size);
    void *Realloc(void *originalPtr, size_t, size_t newSize);
    static void Free(void *ptr);
};

typedef rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<RubyCrtAllocator> > RubynizedValue;
typedef rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<RubyCrtAllocator> >
  RubynizedDocument;

#endif
