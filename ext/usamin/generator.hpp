#ifndef USAMIN_GENERATOR_HPP
#define USAMIN_GENERATOR_HPP

#include <rapidjson/writer.h>
#include <ruby.h>
#include "rubynized_rapidjson.hpp"

template <class Writer>
void write_value(Writer &writer, RubynizedValue &value);

VALUE w_generate(const VALUE self, const VALUE value);
VALUE w_pretty_generate(const int argc, const VALUE *argv, const VALUE self);

#endif
