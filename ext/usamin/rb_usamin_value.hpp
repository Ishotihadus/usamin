#ifndef USAMIN_RB_USAMIN_VALUE_HPP
#define USAMIN_RB_USAMIN_VALUE_HPP

#include <ruby.h>
#include "usamin_value.hpp"

VALUE usamin_alloc(const VALUE klass);

UsaminValue *get_value(VALUE value);
void set_value(VALUE value, UsaminValue *usamin);

VALUE w_value_equal(const VALUE self, const VALUE other);
VALUE w_value_isarray(const VALUE self);
VALUE w_value_isobject(const VALUE self);
VALUE w_value_eval(const int argc, const VALUE *argv, const VALUE self);
VALUE w_value_eval_r(const int argc, const VALUE *argv, const VALUE self);
VALUE w_value_eql(const VALUE self, const VALUE other);
VALUE w_value_isfrozen(const VALUE self);
VALUE w_value_hash(const VALUE self);
VALUE w_value_root(const VALUE self);
VALUE w_value_marshal_dump(const VALUE self);
VALUE w_value_marshal_load(const VALUE self, const VALUE source);

#endif
