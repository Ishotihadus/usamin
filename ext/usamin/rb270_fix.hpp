#ifndef RB270_FIX_HPP
#define RB270_FIX_HPP

#include <cstring>
#include <ruby/version.h>

#if RUBY_API_VERSION_CODE >= 20700

namespace std {
static inline void *ruby_nonempty_memcpy(void *dest, const void *src, size_t n) {
    return (n ? ::memcpy(dest, src, n) : dest);
}
}  // namespace std

#endif

#endif
