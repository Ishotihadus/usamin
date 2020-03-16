#include "rb270_fix.hpp"
#include <rapidjson/document.h>
#include <ruby.h>
#include "usamin_value.hpp"
#include "parser_helper.hpp"

/*
 * @overload [](nth)
 *   @param [Integer] nth
 *   @return [Object | nil]
 *
 * @overload [](start, length)
 *   @param [Integer] start
 *   @param [Integer] length
 *   @return [::Array<Object> | nil]
 *
 * @overload [](range)
 *   @param [Range] range
 *   @return [::Array<Object> | nil]
 */
VALUE w_array_operator_indexer(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, 2);
    UsaminValue *value = get_value(self);
    check_array(value);
    rapidjson::SizeType sz = value->value->Size();
    if (argc == 2) {
        int beg = FIX2INT(argv[0]);
        int len = FIX2INT(argv[1]);
        if (len < 0)
            return Qnil;
        if (beg < 0)
            beg += sz;
        if (beg < 0 || static_cast<rapidjson::SizeType>(beg) > sz)
            return Qnil;
        rapidjson::SizeType end = beg + len;
        if (end > sz)
            end = sz;
        VALUE ret = rb_ary_new2(end - beg);
        for (rapidjson::SizeType i = beg; i < end; i++)
            rb_ary_push(ret, eval((*value->value)[i], value->root_document));
        return ret;
    } else if (rb_obj_is_kind_of(argv[0], rb_cRange)) {
        long beg, len;
        if (rb_range_beg_len(argv[0], &beg, &len, sz, 0) == Qtrue) {
            VALUE ret = rb_ary_new2(len);
            unsigned int beg_i = rb_long2int(beg);
            unsigned int len_i = rb_long2int(len);
            for (rapidjson::SizeType i = beg_i; i < beg_i + len_i; i++)
                rb_ary_push(ret, eval((*value->value)[i], value->root_document));
            return ret;
        }
    } else {
        int l = FIX2INT(argv[0]);
        if (l < 0)
            l += sz;
        if (0 <= l && static_cast<rapidjson::SizeType>(l) < sz)
            return eval((*value->value)[l], value->root_document);
    }
    return Qnil;
}

/*
 * @param [Integer] nth
 * @return [Object]
 */
VALUE w_array_at(const VALUE self, const VALUE nth) {
    UsaminValue *value = get_value(self);
    check_array(value);
    int l = FIX2INT(nth);
    rapidjson::SizeType sz = value->value->Size();
    if (l < 0)
        l += sz;
    if (0 <= l && static_cast<rapidjson::SizeType>(l) < sz)
        return eval((*value->value)[l], value->root_document);
    return Qnil;
}

/*
 * @return [::Array]
 */
VALUE w_array_compact(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    VALUE ret = rb_ary_new2(value->value->Size());
    for (auto &v : value->value->GetArray())
        if (!v.IsNull())
            rb_ary_push(ret, eval(v, value->root_document));
    return ret;
}

/*
 * @return [Object | nil]
 */
VALUE w_array_dig(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, UNLIMITED_ARGUMENTS);
    VALUE value = w_array_at(self, argv[0]);
    if (argc == 1)
        return value;
    else if (value == Qnil)
        return Qnil;
    extern ID id_dig;
    return rb_funcall3(value, id_dig, argc - 1, argv + 1);
}

static VALUE array_enum_size(const VALUE self, const VALUE, const VALUE) {
    return UINT2NUM(get_value(self)->value->Size());
}

/*
 * @yield [value]
 * @return [Enumerator | self]
 */
VALUE w_array_each(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, array_enum_size);
    for (auto &v : value->value->GetArray())
        rb_yield(eval(v, value->root_document));
    return self;
}

/*
 * @yield [index]
 * @yieldparam index [Integer]
 * @return [Enumerator | self]
 */
VALUE w_array_each_index(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, array_enum_size);
    for (rapidjson::SizeType i = 0; i < value->value->Size(); i++)
        rb_yield(UINT2NUM(i));
    return self;
}

VALUE w_array_isempty(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return value->value->Size() == 0 ? Qtrue : Qfalse;
}

/*
 * @overload fetch(nth)
 *   @param [Integer] nth
 *   @return [Object]
 *   @raise [IndexError] if nth is out of array bounds
 *
 * @overload fetch(nth, ifnone)
 *   @param [Integer] nth
 *   @param [Object] ifnone
 *   @return [Object]
 *
 * @overload fetch(nth)
 *   @param [Integer] nth
 *   @yield [nth]
 *   @return [Object]
 */
VALUE w_array_fetch(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, 2);
    UsaminValue *value = get_value(self);
    check_array(value);
    rapidjson::SizeType sz = value->value->Size();

    if (argc == 2 && rb_block_given_p())
        rb_warn("block supersedes default value argument");

    int l = FIX2INT(argv[0]);
    if (l < 0)
        l += sz;
    if (0 <= l && static_cast<rapidjson::SizeType>(l) < sz)
        return eval((*value->value)[l], value->root_document);

    if (rb_block_given_p())
        return rb_yield(argv[0]);
    else if (argc == 2)
        return argv[1];
    else
        rb_raise(rb_eIndexError, "index %ld outside of array bounds: %ld...%u", FIX2LONG(argv[0]),
                 -static_cast<long>(sz), sz);
    return Qnil;
}

/*
 * @overload find_index(val)
 *   @param [Object] val
 *   @return [Integer | nil]
 *
 * @overload find_index
 *   @yield [item]
 *   @yieldparam item [Object]
 *   @return [Integer | nil]
 */
VALUE w_array_find_index(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);

    if (argc == 1) {
        if (rb_block_given_p())
            rb_warn("given block not used");
        for (rapidjson::SizeType i = 0; i < value->value->Size(); i++) {
            if (rb_equal(argv[0], eval_r((*value->value)[i], 0)) == Qtrue)
                return UINT2NUM(i);
        }
        return Qnil;
    }

    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, array_enum_size);
    for (rapidjson::SizeType i = 0; i < value->value->Size(); i++) {
        if (RTEST(rb_yield(eval((*value->value)[i], value->root_document))))
            return UINT2NUM(i);
    }
    return Qnil;
}

void flatten_array(RubynizedValue &value, VALUE array, int level, VALUE root_document) {
    if (level == 0)
        for (auto &v : value.GetArray())
            rb_ary_push(array, eval(v, root_document));
    else
        for (auto &v : value.GetArray())
            if (v.IsArray())
                flatten_array(v, array, level - 1, root_document);
            else
                rb_ary_push(array, eval(v, root_document));
}

/*
 *  @overload flatten(lv = nil)
 *   @return [::Array<Object>]
 */
VALUE w_array_flatten(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);
    int level = -1;
    if (argc == 1 && !NIL_P(argv[0]))
        level = NUM2INT(argv[0]);
    VALUE ret = rb_ary_new2(value->value->Size());
    flatten_array(*value->value, ret, level, value->root_document);
    return ret;
}

/*
 * @overload first
 *   @return [Object | nil]
 *
 * @overload first(n)
 *   @return [::Array<Object>]
 */
VALUE w_array_first(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);
    rapidjson::SizeType sz = value->value->Size();

    if (argc == 0) {
        if (sz == 0)
            return Qnil;
        return eval(*value->value->Begin(), value->root_document);
    } else {
        long l = FIX2LONG(argv[0]);
        if (l > sz)
            l = sz;
        VALUE ret = rb_ary_new2(l);
        for (auto v = value->value->Begin(); v < value->value->Begin() + l; v++)
            rb_ary_push(ret, eval(*v, value->root_document));
        return ret;
    }
}

/*
 * @return [Boolean]
 */
VALUE w_array_include(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_array(value);
    for (auto &v : value->value->GetArray())
        if (rb_equal(val, eval_r(v, 0)))
            return Qtrue;
    return Qfalse;
}

/*
 * @return [String]
 */
VALUE w_array_inspect(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    VALUE ret = rb_str_new2("[");
    bool first = true;
    for (auto &v : value->value->GetArray()) {
        if (!first)
            ret = rb_str_cat2(ret, ", ");
        switch (v.GetType()) {
        case rapidjson::kObjectType:
            ret = rb_str_cat2(ret, "{...}");
            break;
        case rapidjson::kArrayType:
            ret = rb_str_cat2(ret, "[...]");
            break;
        default:
            ret = rb_str_append(ret, rb_inspect(eval(v, value->root_document)));
            break;
        }
        first = false;
    }
    ret = rb_str_cat2(ret, "]");
    return ret;
}

/*
 * @overload last
 *   @return [Object | nil]
 *
 * @overload last(n)
 *   @return [::Array<Object>]
 */
VALUE w_array_last(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);
    rapidjson::SizeType sz = value->value->Size();

    if (argc == 0) {
        return sz > 0 ? eval(*(value->value->End() - 1), value->root_document) : Qnil;
    } else {
        long l = FIX2LONG(argv[0]);
        if (l > sz)
            l = sz;
        VALUE ret = rb_ary_new2(l);
        for (auto v = value->value->End() - l; v < value->value->End(); v++)
            rb_ary_push(ret, eval(*v, value->root_document));
        return ret;
    }
}

/*
 * @return [Integer]
 */
VALUE w_array_length(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return UINT2NUM(value->value->Size());
}

/*
 * @return [::Array<Object>]
 */
VALUE w_array_reverse(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);

    VALUE ret = rb_ary_new2(value->value->Size());
    for (rapidjson::SizeType i = 0, j = value->value->Size() - 1; i < value->value->Size(); i++, j--)
        rb_ary_push(ret, eval((*value->value)[j], value->root_document));
    return ret;
}

/*
 * @overload rindex(val)
 *   @param [Object] val
 *   @return [Integer | nil]
 *
 * @overload rindex
 *   @yield [item]
 *   @yieldparam item [Object]
 *   @return [Integer | nil]
 */
VALUE w_array_rindex(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);

    if (argc == 1) {
        for (rapidjson::SizeType i = 0, j = value->value->Size() - 1; i < value->value->Size(); i++, j--) {
            if (rb_equal(argv[0], eval_r((*value->value)[j], 0)) == Qtrue)
                return UINT2NUM(j);
        }
        return Qnil;
    }

    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, array_enum_size);
    for (rapidjson::SizeType i = 0, j = value->value->Size() - 1; i < value->value->Size(); i++, j--) {
        if (RTEST(rb_yield(eval((*value->value)[j], value->root_document))))
            return UINT2NUM(j);
    }
    return Qnil;
}

/*
 * @overload rotate(cnt = 1)
 *   @return [::Array<Object>]
 */
VALUE w_array_rotate(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);

    switch (value->value->Size()) {
    case 0:
        return rb_ary_new();
    case 1:
        return rb_ary_new3(1, eval(value->value->operator[](0), value->root_document));
    }

    int cnt = argc == 1 ? NUM2INT(argv[0]) : 1;
    if (cnt >= 0) {
        cnt = cnt % value->value->Size();
    } else {
        cnt = -cnt % value->value->Size();
        if (cnt)
            cnt = value->value->Size() - cnt;
    }
    if (cnt == 0)
        return eval_array(*(value->value), value->root_document);

    rapidjson::SizeType ucnt = cnt;
    VALUE ret = rb_ary_new2(value->value->Size());
    for (rapidjson::SizeType i = ucnt; i < value->value->Size(); i++)
        rb_ary_push(ret, eval((*value->value)[i], value->root_document));
    for (rapidjson::SizeType i = 0; i < ucnt; i++)
        rb_ary_push(ret, eval((*value->value)[i], value->root_document));
    return ret;
}

/*
 * @overload slice(nth)
 *   @param [Integer] nth
 *   @return [Object | nil]
 *
 * @overload slice(start, length)
 *   @param [Integer] start
 *   @param [Integer] length
 *   @return [::Array<Object> | nil]
 *
 * @overload slice(range)
 *   @param [Range] range
 *   @return [::Array<Object> | nil]
 */
VALUE w_array_slice(const int argc, const VALUE *argv, const VALUE self) {
    return w_array_operator_indexer(argc, argv, self);
}

/*
 * Convert to Ruby data structures. Same as {Value#eval}.
 *
 * @return [::Array<Object>]
 */
VALUE w_array_eval(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return eval_array(*(value->value), value->root_document);
}
