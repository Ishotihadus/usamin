#include <ruby.h>
#include <ruby/encoding.h>
#include "generator.hpp"
#include "parser.hpp"
#include "rb_usamin_value.hpp"
#include "rb_usamin_hash.hpp"
#include "rb_usamin_array.hpp"

#if SIZEOF_VALUE < SIZEOF_VOIDP
#error SIZEOF_VOIDP must not be greater than SIZEOF_VALUE.
#endif

rb_encoding *utf8;
int utf8index;
ID id_dig, id_to_s;
VALUE rb_cUsaminValue, rb_cUsaminHash, rb_cUsaminArray, rb_eUsaminError, rb_eParserError;
VALUE utf8value, sym_fast, sym_indent, sym_single_line_array, sym_symbolize_names;

extern "C" {
void Init_usamin() {
    utf8 = rb_utf8_encoding();
    utf8index = rb_enc_to_index(utf8);
    utf8value = rb_enc_from_encoding(utf8);
    sym_fast = rb_id2sym(rb_intern("fast"));
    sym_indent = rb_id2sym(rb_intern("indent"));
    sym_single_line_array = rb_id2sym(rb_intern("single_line_array"));
    sym_symbolize_names = rb_id2sym(rb_intern("symbolize_names"));
    id_dig = rb_intern("dig");
    id_to_s = rb_intern("to_s");

    VALUE rb_mUsamin = rb_define_module("Usamin");
    rb_define_module_function(rb_mUsamin, "load", RUBY_METHOD_FUNC(w_load), -1);
    rb_define_module_function(rb_mUsamin, "parse", RUBY_METHOD_FUNC(w_parse), -1);
    rb_define_module_function(rb_mUsamin, "generate", RUBY_METHOD_FUNC(w_generate), 1);
    rb_define_module_function(rb_mUsamin, "pretty_generate", RUBY_METHOD_FUNC(w_pretty_generate), -1);

    rb_cUsaminValue = rb_define_class_under(rb_mUsamin, "Value", rb_cObject);
    rb_undef_alloc_func(rb_cUsaminValue);
    rb_undef_method(rb_cUsaminValue, "initialize");
    rb_define_method(rb_cUsaminValue, "==", RUBY_METHOD_FUNC(w_value_equal), 1);
    rb_define_method(rb_cUsaminValue, "===", RUBY_METHOD_FUNC(w_value_equal), 1);
    rb_define_method(rb_cUsaminValue, "array?", RUBY_METHOD_FUNC(w_value_isarray), 0);
    rb_define_method(rb_cUsaminValue, "hash?", RUBY_METHOD_FUNC(w_value_isobject), 0);
    rb_define_method(rb_cUsaminValue, "object?", RUBY_METHOD_FUNC(w_value_isobject), 0);
    rb_define_method(rb_cUsaminValue, "eval", RUBY_METHOD_FUNC(w_value_eval), -1);
    rb_define_method(rb_cUsaminValue, "eval_r", RUBY_METHOD_FUNC(w_value_eval_r), -1);
    rb_define_method(rb_cUsaminValue, "eql?", RUBY_METHOD_FUNC(w_value_eql), 1);
    rb_define_method(rb_cUsaminValue, "frozen?", RUBY_METHOD_FUNC(w_value_isfrozen), 0);
    rb_define_method(rb_cUsaminValue, "hash", RUBY_METHOD_FUNC(w_value_hash), 0);
    rb_define_method(rb_cUsaminValue, "root", RUBY_METHOD_FUNC(w_value_root), 0);

    rb_cUsaminHash = rb_define_class_under(rb_mUsamin, "Hash", rb_cUsaminValue);
    rb_include_module(rb_cUsaminHash, rb_mEnumerable);
    rb_define_alloc_func(rb_cUsaminHash, usamin_alloc);
    rb_undef_method(rb_cUsaminHash, "initialize");
    rb_define_method(rb_cUsaminHash, "[]", RUBY_METHOD_FUNC(w_hash_operator_indexer), 1);
    rb_define_method(rb_cUsaminHash, "assoc", RUBY_METHOD_FUNC(w_hash_assoc), 1);
    rb_define_method(rb_cUsaminHash, "compact", RUBY_METHOD_FUNC(w_hash_compact), 0);
    rb_define_method(rb_cUsaminHash, "dig", RUBY_METHOD_FUNC(w_hash_dig), -1);
    rb_define_method(rb_cUsaminHash, "each", RUBY_METHOD_FUNC(w_hash_each), 0);
    rb_define_method(rb_cUsaminHash, "each_pair", RUBY_METHOD_FUNC(w_hash_each), 0);
    rb_define_method(rb_cUsaminHash, "each_key", RUBY_METHOD_FUNC(w_hash_each_key), 0);
    rb_define_method(rb_cUsaminHash, "each_value", RUBY_METHOD_FUNC(w_hash_each_value), 0);
    rb_define_method(rb_cUsaminHash, "empty?", RUBY_METHOD_FUNC(w_hash_isempty), 0);
    rb_define_method(rb_cUsaminHash, "fetch", RUBY_METHOD_FUNC(w_hash_fetch), -1);
    rb_define_method(rb_cUsaminHash, "fetch_values", RUBY_METHOD_FUNC(w_hash_fetch_values), -1);
    rb_define_method(rb_cUsaminHash, "flatten", RUBY_METHOD_FUNC(w_hash_flatten), -1);
    rb_define_method(rb_cUsaminHash, "has_key?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "include?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "key?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "member?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "has_value?", RUBY_METHOD_FUNC(w_hash_hasvalue), 1);
    rb_define_method(rb_cUsaminHash, "value?", RUBY_METHOD_FUNC(w_hash_hasvalue), 1);
    rb_define_method(rb_cUsaminHash, "key", RUBY_METHOD_FUNC(w_hash_key), 1);
    rb_define_method(rb_cUsaminHash, "index", RUBY_METHOD_FUNC(w_hash_key), 1);
    rb_define_method(rb_cUsaminHash, "inspect", RUBY_METHOD_FUNC(w_hash_inspect), 0);
    rb_define_method(rb_cUsaminHash, "to_s", RUBY_METHOD_FUNC(w_hash_inspect), 0);
    rb_define_method(rb_cUsaminHash, "keys", RUBY_METHOD_FUNC(w_hash_keys), 0);
    rb_define_method(rb_cUsaminHash, "length", RUBY_METHOD_FUNC(w_hash_length), 0);
    rb_define_method(rb_cUsaminHash, "size", RUBY_METHOD_FUNC(w_hash_length), 0);
    rb_define_method(rb_cUsaminHash, "merge", RUBY_METHOD_FUNC(w_hash_merge), -1);
    rb_define_method(rb_cUsaminHash, "rassoc", RUBY_METHOD_FUNC(w_hash_rassoc), 1);
    rb_define_method(rb_cUsaminHash, "reject", RUBY_METHOD_FUNC(w_hash_reject), 0);
    rb_define_method(rb_cUsaminHash, "select", RUBY_METHOD_FUNC(w_hash_select), 0);
    rb_define_method(rb_cUsaminHash, "slice", RUBY_METHOD_FUNC(w_hash_slice), -1);
    rb_define_method(rb_cUsaminHash, "to_hash", RUBY_METHOD_FUNC(w_hash_eval), 0);
    rb_define_method(rb_cUsaminHash, "transform_keys", RUBY_METHOD_FUNC(w_hash_transform_keys), 0);
    rb_define_method(rb_cUsaminHash, "transform_values", RUBY_METHOD_FUNC(w_hash_transform_values), 0);
    rb_define_method(rb_cUsaminHash, "values", RUBY_METHOD_FUNC(w_hash_values), 0);
    rb_define_method(rb_cUsaminHash, "values_at", RUBY_METHOD_FUNC(w_hash_values_at), -1);
    rb_define_method(rb_cUsaminHash, "marshal_dump", RUBY_METHOD_FUNC(w_value_marshal_dump), 0);
    rb_define_method(rb_cUsaminHash, "marshal_load", RUBY_METHOD_FUNC(w_value_marshal_load), 1);

    rb_cUsaminArray = rb_define_class_under(rb_mUsamin, "Array", rb_cUsaminValue);
    rb_include_module(rb_cUsaminArray, rb_mEnumerable);
    rb_define_alloc_func(rb_cUsaminArray, usamin_alloc);
    rb_undef_method(rb_cUsaminArray, "initialize");
    rb_define_method(rb_cUsaminArray, "[]", RUBY_METHOD_FUNC(w_array_operator_indexer), -1);
    rb_define_method(rb_cUsaminArray, "at", RUBY_METHOD_FUNC(w_array_at), 1);
    rb_define_method(rb_cUsaminArray, "compact", RUBY_METHOD_FUNC(w_array_compact), 0);
    rb_define_method(rb_cUsaminArray, "dig", RUBY_METHOD_FUNC(w_array_dig), -1);
    rb_define_method(rb_cUsaminArray, "each", RUBY_METHOD_FUNC(w_array_each), 0);
    rb_define_method(rb_cUsaminArray, "each_index", RUBY_METHOD_FUNC(w_array_each_index), 0);
    rb_define_method(rb_cUsaminArray, "empty?", RUBY_METHOD_FUNC(w_array_isempty), 0);
    rb_define_method(rb_cUsaminArray, "fetch", RUBY_METHOD_FUNC(w_array_fetch), -1);
    rb_define_method(rb_cUsaminArray, "find_index", RUBY_METHOD_FUNC(w_array_find_index), -1);
    rb_define_method(rb_cUsaminArray, "flatten", RUBY_METHOD_FUNC(w_array_flatten), -1);
    rb_define_method(rb_cUsaminArray, "index", RUBY_METHOD_FUNC(w_array_index), -1);
    rb_define_method(rb_cUsaminArray, "first", RUBY_METHOD_FUNC(w_array_first), -1);
    rb_define_method(rb_cUsaminArray, "include?", RUBY_METHOD_FUNC(w_array_include), 1);
    rb_define_method(rb_cUsaminArray, "inspect", RUBY_METHOD_FUNC(w_array_inspect), 0);
    rb_define_method(rb_cUsaminArray, "to_s", RUBY_METHOD_FUNC(w_array_inspect), 0);
    rb_define_method(rb_cUsaminArray, "last", RUBY_METHOD_FUNC(w_array_last), -1);
    rb_define_method(rb_cUsaminArray, "length", RUBY_METHOD_FUNC(w_array_length), 0);
    rb_define_method(rb_cUsaminArray, "reverse", RUBY_METHOD_FUNC(w_array_reverse), 0);
    rb_define_method(rb_cUsaminArray, "rindex", RUBY_METHOD_FUNC(w_array_rindex), -1);
    rb_define_method(rb_cUsaminArray, "rotate", RUBY_METHOD_FUNC(w_array_rotate), -1);
    rb_define_method(rb_cUsaminArray, "size", RUBY_METHOD_FUNC(w_array_length), 0);
    rb_define_method(rb_cUsaminArray, "slice", RUBY_METHOD_FUNC(w_array_slice), -1);
    rb_define_method(rb_cUsaminArray, "to_ary", RUBY_METHOD_FUNC(w_array_eval), 0);
    rb_define_method(rb_cUsaminArray, "marshal_dump", RUBY_METHOD_FUNC(w_value_marshal_dump), 0);
    rb_define_method(rb_cUsaminArray, "marshal_load", RUBY_METHOD_FUNC(w_value_marshal_load), 1);

    rb_eUsaminError = rb_define_class_under(rb_mUsamin, "UsaminError", rb_eStandardError);
    rb_eParserError = rb_define_class_under(rb_mUsamin, "ParserError", rb_eUsaminError);
}
}
