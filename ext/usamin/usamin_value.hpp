#ifndef USAMIN_USAMIN_VALUE_HPP
#define USAMIN_USAMIN_VALUE_HPP

#include <ruby.h>
#include "rubynized_rapidjson.hpp"

class UsaminValue {
public:
    RubynizedValue *value;
    VALUE root_document;
    bool free_flag;

    UsaminValue(RubynizedValue *value = nullptr, const bool free_flag = false, const VALUE root_document = Qnil);
    ~UsaminValue();
};

#endif
