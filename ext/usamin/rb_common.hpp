#ifndef USAMIN_RB_COMMON_HPP
#define USAMIN_RB_COMMON_HPP

#include <ruby.h>
#include "rubynized_rapidjson.hpp"
#include "usamin_value.hpp"

VALUE get_utf8_str(VALUE str);
VALUE new_utf8_str(const char *cstr, const long len);
bool str_compare_xx(VALUE str1, const RubynizedValue &str2);
void check_value(UsaminValue *ptr);
void check_object(UsaminValue *ptr);
void check_array(UsaminValue *ptr);

#endif
