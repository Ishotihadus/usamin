#include <ruby.h>
#include <ruby/encoding.h>
#include "rubynized_rapidjson.hpp"
#include "usamin_value.hpp"

VALUE get_utf8_str(VALUE str) {
    extern int utf8index;
    extern VALUE utf8value;
    extern rb_encoding *utf8;

    Check_Type(str, T_STRING);
    int encoding = rb_enc_get_index(str);
    if (encoding == utf8index || rb_enc_compatible(str, utf8value) == utf8)
        return str;
    else
        return rb_str_conv_enc(str, rb_enc_from_index(encoding), utf8);
}

VALUE new_utf8_str(const char *cstr, const long len) {
    extern int utf8index;
    VALUE ret = rb_str_new(cstr, len);
    rb_enc_set_index(ret, utf8index);
    return ret;
}

static inline bool str_compare(const char *str1, const long len1, const char *str2, const long len2) {
    if (len1 != len2 || len1 < 0)
        return false;
    return memcmp(str1, str2, len1) == 0;
}

bool str_compare_xx(VALUE str1, const RubynizedValue &str2) {
    if (RB_TYPE_P(str1, T_STRING))
        str1 = get_utf8_str(str1);
    else if (SYMBOL_P(str1))
        str1 = get_utf8_str(rb_sym_to_s(str1));
    else
        str1 = get_utf8_str(StringValue(str1));
    return str_compare(RSTRING_PTR(str1), RSTRING_LEN(str1), str2.GetString(), str2.GetStringLength());
}

void check_value(UsaminValue *ptr) {
    extern VALUE rb_eUsaminError;
    if (!ptr || !ptr->value)
        rb_raise(rb_eUsaminError, "null reference");
}

void check_object(UsaminValue *ptr) {
    extern VALUE rb_eUsaminError;
    check_value(ptr);
    if (!ptr->value->IsObject())
        rb_raise(rb_eUsaminError, "unexpected type of value (expected: object)");
}

void check_array(UsaminValue *ptr) {
    extern VALUE rb_eUsaminError;
    check_value(ptr);
    if (!ptr->value->IsArray())
        rb_raise(rb_eUsaminError, "unexpected type of value (expected: array)");
}
