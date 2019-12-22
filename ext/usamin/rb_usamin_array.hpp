#ifndef USAMIN_RB_USAMIN_ARRAY_HPP
#define USAMIN_RB_USAMIN_ARRAY_HPP

#include <ruby.h>
#include "rubynized_rapidjson.hpp"

void flatten_array(RubynizedValue &value, VALUE array, int level, VALUE root_document);

VALUE w_array_operator_indexer(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_at(const VALUE self, const VALUE nth);
VALUE w_array_compact(const VALUE self, const VALUE nth);
VALUE w_array_dig(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_each(const VALUE self);
VALUE w_array_each_index(const VALUE self);
VALUE w_array_isempty(const VALUE self);
VALUE w_array_fetch(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_find_index(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_flatten(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_index(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_first(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_include(const VALUE self, const VALUE val);
VALUE w_array_inspect(const VALUE self);
VALUE w_array_last(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_length(const VALUE self);
VALUE w_array_reverse(const VALUE self);
VALUE w_array_rindex(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_rotate(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_slice(const int argc, const VALUE *argv, const VALUE self);
VALUE w_array_eval(const VALUE self);

#endif
