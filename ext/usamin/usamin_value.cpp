#include "usamin_value.hpp"
#include <ruby.h>
#include "rubynized_rapidjson.hpp"

UsaminValue::UsaminValue(RubynizedValue *value, const bool free_flag, const VALUE root_document) {
    this->value = value;
    this->free_flag = free_flag;
    this->root_document = root_document;
}

UsaminValue::~UsaminValue() {
    if (value && free_flag)
        delete (RubynizedDocument *)value;
}
