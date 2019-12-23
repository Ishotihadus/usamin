#include <rapidjson/document.h>
#include <ruby.h>
#include "rb_usamin_array.hpp"
#include "usamin_value.hpp"
#include "parser_helper.hpp"

/*
 * @return [Object | nil]
 *
 * @note This method has linear time complexity.
 */
VALUE w_hash_operator_indexer(const VALUE self, const VALUE key) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (str_compare_xx(key, m.name))
            return eval(m.value, value->root_document);
    return Qnil;
}

/*
 * @return [::Array | nil]
 *
 * @note This method has linear time complexity.
 */
VALUE w_hash_assoc(const VALUE self, const VALUE key) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (str_compare_xx(key, m.name))
            return rb_assoc_new(eval_str(m.name), eval(m.value, value->root_document));
    return Qnil;
}

/*
 * @return [::Hash]
 */
VALUE w_hash_compact(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE hash = rb_hash_new();
    for (auto &m : value->value->GetObject())
        if (!m.value.IsNull())
            rb_hash_aset(hash, eval(m.name, value->root_document), eval(m.value, value->root_document));
    return hash;
}

/*
 * @return [::Hash]
 */
VALUE w_hash_deconstruct_keys(const VALUE self, const VALUE keys) {
    UsaminValue *value = get_value(self);
    check_object(value);
    if (NIL_P(keys))
        return eval_object(*(value->value), value->root_document, 1);
    VALUE hash_keys = rb_hash_new();
    VALUE *key_iter = RARRAY_PTR(keys);
    const VALUE *key_iter_end = key_iter + RARRAY_LEN(keys);
    for (VALUE *k = key_iter; k < key_iter_end; k++)
        rb_hash_aset(hash_keys, *k, Qtrue);
    VALUE hash = rb_hash_new();
    for (auto &m : value->value->GetObject()) {
        VALUE name = rb_to_symbol(new_utf8_str(m.name.GetString(), m.name.GetStringLength()));
        if (!NIL_P(rb_hash_lookup(hash_keys, name)))
            rb_hash_aset(hash, name, eval(m.value, value->root_document));
    }
    return hash;
}

/*
 * @return [Object | nil]
 */
VALUE w_hash_dig(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, UNLIMITED_ARGUMENTS);
    VALUE value = w_hash_operator_indexer(self, argv[0]);
    if (argc == 1)
        return value;
    else if (value == Qnil)
        return Qnil;
    extern ID id_dig;
    return rb_funcall3(value, id_dig, argc - 1, argv + 1);
}

static VALUE hash_enum_size(const VALUE self, const VALUE, const VALUE) {
    return UINT2NUM(get_value(self)->value->MemberCount());
}

/*
 * @yield [key, value]
 * @yieldparam key [String]
 * @yieldparam value [Object]
 * @return [Enumerator | self]
 */
VALUE w_hash_each(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    if (rb_proc_arity(rb_block_proc()) > 1) {
        for (auto &m : value->value->GetObject()) {
            VALUE args[] = {eval_str(m.name), eval(m.value, value->root_document)};
            rb_yield_values2(2, args);
        }
    } else {
        for (auto &m : value->value->GetObject())
            rb_yield(rb_assoc_new(eval_str(m.name), eval(m.value, value->root_document)));
    }
    return self;
}

/*
 * @yield [key]
 * @yieldparam key [String]
 * @return [Enumerator | self]
 */
VALUE w_hash_each_key(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    for (auto &m : value->value->GetObject())
        rb_yield(eval_str(m.name));
    return self;
}

/*
 * @yield [value]
 * @return [Enumerator | self]
 */
VALUE w_hash_each_value(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    for (auto &m : value->value->GetObject())
        rb_yield(eval(m.value, value->root_document));
    return self;
}

VALUE w_hash_isempty(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return value->value->MemberCount() == 0 ? Qtrue : Qfalse;
}

/*
 * @overload fetch(key, default = nil)
 *   @param [String] key
 *   @param [Object] default
 *   @return [Object]
 *
 * @overload fetch(key)
 *   @param [String] key
 *   @yield [key]
 *   @return [Object]
 */
VALUE w_hash_fetch(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, 2);
    if (argc == 2 && rb_block_given_p())
        rb_warn("block supersedes default value argument");
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (str_compare_xx(argv[0], m.name))
            return eval(m.value, value->root_document);
    return rb_block_given_p() ? rb_yield(argv[0]) : argc == 2 ? argv[1] : Qnil;
}

/*
 * @param [String] key
 * @return [::Array<Object>]
 */
VALUE w_hash_fetch_values(const int argc, VALUE *argv, const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_ary_new2(argc);
    for (int i = 0; i < argc; i++) {
        bool found = false;
        for (auto &m : value->value->GetObject()) {
            if (str_compare_xx(argv[i], m.name)) {
                rb_ary_push(ret, eval(m.value, value->root_document));
                found = true;
                break;
            }
        }
        if (!found) {
            if (rb_block_given_p()) {
                rb_ary_push(ret, rb_yield(argv[i]));
            } else {
                rb_ary_free(ret);
                rb_raise(rb_eKeyError, "key not found: \"%s\"", StringValueCStr(argv[i]));
                return Qnil;
            }
        }
    }
    return ret;
}

/*
 *  @overload flatten(level = 1)
 *   @return [::Array<Object>]
 */
VALUE w_hash_flatten(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_object(value);

    int level = 1;
    if (argc == 1 && !NIL_P(argv[0])) {
        level = NUM2INT(argv[0]);
        if (level < 0)
            level = -1;
    }

    if (level == 0) {
        VALUE ret = rb_ary_new2(value->value->MemberCount());
        for (auto &m : value->value->GetObject())
            rb_ary_push(ret, rb_ary_new3(2, eval_str(m.name), eval(m.value, value->root_document)));
        return ret;
    }

    VALUE ret = rb_ary_new2(value->value->MemberCount() * 2);
    if (level == 1) {
        for (auto &m : value->value->GetObject()) {
            rb_ary_push(ret, eval_str(m.name));
            rb_ary_push(ret, eval(m.value, value->root_document));
        }
    } else {
        for (auto &m : value->value->GetObject()) {
            rb_ary_push(ret, eval_str(m.name));
            if (m.value.IsArray())
                flatten_array(m.value, ret, level > 0 ? level - 2 : level, value->root_document);
            else
                rb_ary_push(ret, eval(m.value, value->root_document));
        }
    }
    return ret;
}

/*
 * @note This method has linear time complexity.
 */
VALUE w_hash_haskey(const VALUE self, const VALUE key) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (str_compare_xx(key, m.name))
            return Qtrue;
    return Qfalse;
}

VALUE w_hash_hasvalue(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (rb_equal(val, eval_r(m.value, 0)))
            return Qtrue;
    return Qfalse;
}

/*
 * @return [String | nil]
 */
VALUE w_hash_key(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (rb_equal(val, eval_r(m.value, 0)))
            return eval_str(m.name);
    return Qnil;
}

/*
 * @return [String]
 */
VALUE w_hash_inspect(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_str_new2("{");
    bool first = true;
    for (auto &m : value->value->GetObject()) {
        if (!first)
            ret = rb_str_cat2(ret, ", ");
        ret = rb_str_append(ret, rb_inspect(eval_str(m.name)));
        ret = rb_str_cat2(ret, "=>");
        switch (m.value.GetType()) {
        case rapidjson::kObjectType:
            ret = rb_str_cat2(ret, "{...}");
            break;
        case rapidjson::kArrayType:
            ret = rb_str_cat2(ret, "[...]");
            break;
        default:
            ret = rb_str_append(ret, rb_inspect(eval(m.value, value->root_document)));
            break;
        }
        first = false;
    }
    ret = rb_str_cat2(ret, "}");
    return ret;
}

/*
 * @return [::Hash]
 */
VALUE w_hash_invert(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_hash_new();
    for (auto &m : value->value->GetObject())
        rb_hash_aset(ret, eval(m.value, value->root_document), eval_str(m.name));
    return ret;
}

/*
 * @return [::Array<String>]
 */
VALUE w_hash_keys(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_ary_new2(value->value->MemberCount());
    for (auto &m : value->value->GetObject())
        rb_ary_push(ret, eval_str(m.name));
    return ret;
}

/*
 * @return [Integer]
 */
VALUE w_hash_length(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return UINT2NUM(value->value->MemberCount());
}

/*
 * @return [::Array | nil]
 */
VALUE w_hash_rassoc(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (rb_funcall(val, rb_intern("=="), 1, eval_r(m.value, 0)))
            return rb_assoc_new(eval_str(m.name), eval(m.value, value->root_document));
    return Qnil;
}

/*
 * @yield [key, value]
 * @yieldparam key [String]
 * @yieldparam value [Object]
 * @return [Enumerator | ::Hash]
 */
VALUE w_hash_select(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    VALUE hash = rb_hash_new();
    if (rb_proc_arity(rb_block_proc()) > 1) {
        for (auto &m : value->value->GetObject()) {
            VALUE args[] = {eval_str(m.name), eval(m.value, value->root_document)};
            if (RTEST(rb_yield_values2(2, args)))
                rb_hash_aset(hash, args[0], args[1]);
        }
    } else {
        for (auto &m : value->value->GetObject()) {
            VALUE key = eval_str(m.name);
            VALUE val = eval(m.value, value->root_document);
            if (RTEST(rb_yield(rb_assoc_new(key, val))))
                rb_hash_aset(hash, key, val);
        }
    }
    return hash;
}

/*
 * @yield [key, value]
 * @yieldparam key [String]
 * @yieldparam value [Object]
 * @return [Enumerator | ::Hash]
 */
VALUE w_hash_reject(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    VALUE hash = rb_hash_new();
    if (rb_proc_arity(rb_block_proc()) > 1) {
        for (auto &m : value->value->GetObject()) {
            VALUE args[] = {eval_str(m.name), eval(m.value, value->root_document)};
            if (!RTEST(rb_yield_values2(2, args)))
                rb_hash_aset(hash, args[0], args[1]);
        }
    } else {
        for (auto &m : value->value->GetObject()) {
            VALUE key = eval_str(m.name);
            VALUE val = eval(m.value, value->root_document);
            if (!RTEST(rb_yield(rb_assoc_new(key, val))))
                rb_hash_aset(hash, key, val);
        }
    }
    return hash;
}

/*
 * @yield [key, value]
 * @yieldparam key [String]
 * @yieldparam value [Object]
 * @return [Enumerator | ::Hash]
 */
VALUE w_hash_slice(const int argc, const VALUE *argv, const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE hash = rb_hash_new();
    for (int i = 0; i < argc; i++)
        for (auto &m : value->value->GetObject())
            if (str_compare_xx(argv[i], m.name))
                rb_hash_aset(hash, eval_str(m.name), eval(m.value, value->root_document));
    return hash;
}

/*
 * Convert to Ruby Hash. Same as {Value#eval}.
 *
 * @return [::Hash]
 */
VALUE w_hash_eval(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return eval_object(*(value->value), value->root_document, 0);
}

/*
 * @return [::Hash]
 */
VALUE w_hash_merge(const int argc, const VALUE *argv, const VALUE self) {
    static ID s = rb_intern("merge");
    VALUE args, block;
    rb_scan_args(argc, argv, "*&", &args, &block);
    return rb_funcall_with_block(w_hash_eval(self), s, (int)RARRAY_LEN(args), RARRAY_CONST_PTR(args), block);
}

/*
 * @return [::Array<Object>]
 */
VALUE w_hash_values(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_ary_new2(value->value->MemberCount());
    for (auto &m : value->value->GetObject())
        rb_ary_push(ret, eval(m.value, value->root_document));
    return ret;
}

static VALUE hash_proc_call(RB_BLOCK_CALL_FUNC_ARGLIST(key, self)) {
    rb_check_arity(argc, 1, 1);
    return w_hash_operator_indexer(self, key);
}

/*
 * @return [Proc]
 */
VALUE w_hash_to_proc(const VALUE self) {
    return rb_proc_new(RUBY_METHOD_FUNC(hash_proc_call), self);
}

/*
 * @yield [key]
 * @yieldparam key [String]
 * @return [Enumerator | ::Hash]
 */
VALUE w_hash_transform_keys(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    VALUE hash = rb_hash_new();
    for (auto &m : value->value->GetObject())
        rb_hash_aset(hash, rb_yield(eval_str(m.name)), eval(m.value, value->root_document));
    return hash;
}

/*
 * @yield [value]
 * @yieldparam value [Object]
 * @return [Enumerator | ::Hash]
 */
VALUE w_hash_transform_values(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    VALUE hash = rb_hash_new();
    for (auto &m : value->value->GetObject())
        rb_hash_aset(hash, eval_str(m.name), rb_yield(eval(m.value, value->root_document)));
    return hash;
}

/*
 * @param [String] keys
 * @return [::Array<Object>]
 */
VALUE w_hash_values_at(const int argc, const VALUE *argv, const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_ary_new2(argc);
    for (int i = 0; i < argc; i++) {
        VALUE data = Qnil;
        for (auto &m : value->value->GetObject()) {
            if (str_compare_xx(argv[i], m.name)) {
                data = eval(m.value, value->root_document);
                break;
            }
        }
        rb_ary_push(ret, data);
    }
    return ret;
}
