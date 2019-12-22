#include <ruby.h>
#include "rb_common.hpp"
#include "rb_usamin_value.hpp"
#include "rubynized_rapidjson.hpp"
#include "usamin_value.hpp"

static inline int test_option(VALUE options, VALUE sym) {
    return !NIL_P(options) && RTEST(rb_hash_lookup(options, sym));
}

static inline VALUE make_hash(UsaminValue *value, bool is_root = false) {
    extern VALUE rb_cUsaminHash;
    VALUE ret = rb_obj_alloc(rb_cUsaminHash);
    set_value(ret, value);
    if (is_root)
        value->root_document = ret;
    return ret;
}

static inline VALUE make_array(UsaminValue *value, bool is_root = false) {
    extern VALUE rb_cUsaminArray;
    VALUE ret = rb_obj_alloc(rb_cUsaminArray);
    set_value(ret, value);
    if (is_root)
        value->root_document = ret;
    return ret;
}

static inline VALUE eval_num(RubynizedValue &);
static inline VALUE eval_str(RubynizedValue &);
static inline VALUE eval_object_r(RubynizedValue &, int);
static inline VALUE eval_array_r(RubynizedValue &, int);

static VALUE eval(RubynizedValue &value, VALUE root_document) {
    extern VALUE rb_eUsaminError;
    switch (value.GetType()) {
    case rapidjson::kObjectType:
        return make_hash(new UsaminValue(&value, false, root_document));
    case rapidjson::kArrayType:
        return make_array(new UsaminValue(&value, false, root_document));
    case rapidjson::kNullType:
        return Qnil;
    case rapidjson::kFalseType:
        return Qfalse;
    case rapidjson::kTrueType:
        return Qtrue;
    case rapidjson::kNumberType:
        return eval_num(value);
    case rapidjson::kStringType:
        return eval_str(value);
    default:
        rb_raise(rb_eUsaminError, "unknown value type: %d", value.GetType());
        return Qnil;
    }
}

static VALUE eval_r(RubynizedValue &value, int symbolize) {
    extern VALUE rb_eUsaminError;
    switch (value.GetType()) {
    case rapidjson::kObjectType:
        return eval_object_r(value, symbolize);
    case rapidjson::kArrayType:
        return eval_array_r(value, symbolize);
    case rapidjson::kNullType:
        return Qnil;
    case rapidjson::kFalseType:
        return Qfalse;
    case rapidjson::kTrueType:
        return Qtrue;
    case rapidjson::kNumberType:
        return eval_num(value);
    case rapidjson::kStringType:
        return eval_str(value);
    default:
        rb_raise(rb_eUsaminError, "unknown value type: %d", value.GetType());
        return Qnil;
    }
}

static inline VALUE eval_num(RubynizedValue &value) {
    if (value.IsInt())
        return INT2FIX(value.GetInt());
    else if (value.IsUint())
        return UINT2NUM(value.GetUint());
    else if (value.IsInt64())
        return LL2NUM(value.GetInt64());
    else if (value.IsUint64())
        return ULL2NUM(value.GetUint64());
    else
        return DBL2NUM(value.GetDouble());
}

static inline VALUE eval_str(RubynizedValue &value) {
    return new_utf8_str(value.GetString(), value.GetStringLength());
}

static inline VALUE eval_object(RubynizedValue &value, const VALUE root_document, int symbolize) {
    VALUE ret = rb_hash_new();
    if (symbolize)
        for (auto &m : value.GetObject())
            rb_hash_aset(ret, rb_to_symbol(eval_str(m.name)), eval(m.value, root_document));
    else
        for (auto &m : value.GetObject())
            rb_hash_aset(ret, eval_str(m.name), eval(m.value, root_document));
    return ret;
}

static inline VALUE eval_object_r(RubynizedValue &value, int symbolize) {
    VALUE ret = rb_hash_new();
    if (symbolize)
        for (auto &m : value.GetObject())
            rb_hash_aset(ret, rb_to_symbol(eval_str(m.name)), eval_r(m.value, symbolize));
    else
        for (auto &m : value.GetObject())
            rb_hash_aset(ret, eval_str(m.name), eval_r(m.value, symbolize));
    return ret;
}

static inline VALUE eval_array(RubynizedValue &value, const VALUE root_document) {
    VALUE ret = rb_ary_new2(value.Size());
    for (auto &v : value.GetArray())
        rb_ary_push(ret, eval(v, root_document));
    return ret;
}

static inline VALUE eval_array_r(RubynizedValue &value, int symbolize) {
    VALUE ret = rb_ary_new2(value.Size());
    for (auto &v : value.GetArray())
        rb_ary_push(ret, eval_r(v, symbolize));
    return ret;
}
