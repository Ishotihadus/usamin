#ifndef USAMIN_RB_USAMIN_HASH_HPP
#define USAMIN_RB_USAMIN_HASH_HPP

#include <ruby.h>

VALUE w_hash_operator_indexer(const VALUE self, const VALUE key);
VALUE w_hash_assoc(const VALUE self, const VALUE key);
VALUE w_hash_compact(const VALUE self);
VALUE w_hash_dig(const int argc, const VALUE *argv, const VALUE self);
VALUE w_hash_each(const VALUE self);
VALUE w_hash_each_key(const VALUE self);
VALUE w_hash_each_value(const VALUE self);
VALUE w_hash_isempty(const VALUE self);
VALUE w_hash_fetch(const int argc, const VALUE *argv, const VALUE self);
VALUE w_hash_fetch_values(const int argc, VALUE *argv, const VALUE self);
VALUE w_hash_flatten(const int argc, const VALUE *argv, const VALUE self);
VALUE w_hash_haskey(const VALUE self, const VALUE key);
VALUE w_hash_hasvalue(const VALUE self, const VALUE val);
VALUE w_hash_key(const VALUE self, const VALUE val);
VALUE w_hash_inspect(const VALUE self);
VALUE w_hash_keys(const VALUE self);
VALUE w_hash_length(const VALUE self);
VALUE w_hash_rassoc(const VALUE self, const VALUE val);
VALUE w_hash_select(const VALUE self);
VALUE w_hash_reject(const VALUE self);
VALUE w_hash_slice(const int argc, const VALUE *argv, const VALUE self);
VALUE w_hash_eval(const VALUE self);
VALUE w_hash_merge(const int argc, const VALUE *argv, const VALUE self);
VALUE w_hash_values(const VALUE self);
VALUE w_hash_transform_keys(const VALUE self);
VALUE w_hash_transform_values(const VALUE self);
VALUE w_hash_values_at(const int argc, const VALUE *argv, const VALUE self);

#endif
