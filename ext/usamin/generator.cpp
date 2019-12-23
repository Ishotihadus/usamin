#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <ruby.h>
#include "rb_common.hpp"
#include "rb_usamin_value.hpp"
#include "rubynized_rapidjson.hpp"

template <class Writer>
static inline void write_hash(Writer &, const VALUE);
template <class Writer>
static inline void write_array(Writer &, const VALUE);
template <class Writer>
static inline void write_struct(Writer &, const VALUE);
template <class Writer>
static inline void write_usamin(Writer &, const VALUE);

template <class Writer>
static inline void write_str(Writer &writer, const VALUE value) {
    VALUE v = get_utf8_str(value);
    writer.String(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)));
}

template <class Writer>
static inline void write_to_s(Writer &writer, const VALUE value) {
    extern ID id_to_s;
    write_str(writer, rb_funcall(value, id_to_s, 0));
}

template <class Writer>
static void write(Writer &writer, const VALUE value) {
    extern VALUE rb_cUsaminValue;
    switch (TYPE(value)) {
    case RUBY_T_NONE:
    case RUBY_T_NIL:
    case RUBY_T_UNDEF:
        writer.Null();
        break;
    case RUBY_T_TRUE:
        writer.Bool(true);
        break;
    case RUBY_T_FALSE:
        writer.Bool(false);
        break;
    case RUBY_T_FIXNUM:
        writer.Int64(FIX2LONG(value));
        break;
    case RUBY_T_FLOAT:
    case RUBY_T_RATIONAL:
        writer.Double(NUM2DBL(value));
        break;
    case RUBY_T_STRING:
        write_str(writer, value);
        break;
    case RUBY_T_ARRAY:
        write_array(writer, value);
        break;
    case RUBY_T_HASH:
        write_hash(writer, value);
        break;
    case RUBY_T_STRUCT:
        write_struct(writer, value);
        break;
    case RUBY_T_BIGNUM: {
        VALUE v = rb_big2str(value, 10);
        writer.RawValue(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)), rapidjson::kNumberType);
    } break;
    default:
        if (rb_obj_is_kind_of(value, rb_cUsaminValue))
            write_usamin(writer, value);
        else
            write_to_s(writer, value);
        break;
    }
}

template <class Writer>
static inline void write_key_str(Writer &writer, const VALUE value) {
    VALUE v = get_utf8_str(value);
    writer.Key(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)));
}

template <class Writer>
static inline void write_key_to_s(Writer &writer, const VALUE value) {
    extern ID id_to_s;
    write_key_str(writer, rb_funcall(value, id_to_s, 0));
}

template <class Writer>
static inline int write_hash_each(const VALUE key, const VALUE value, Writer *writer) {
    if (RB_TYPE_P(key, T_STRING))
        write_key_str(*writer, key);
    else if (RB_TYPE_P(key, T_SYMBOL))
        write_key_str(*writer, rb_sym_to_s(key));
    else
        write_key_to_s(*writer, key);
    write(*writer, value);
    return ST_CONTINUE;
}

template <class Writer>
static inline void write_hash(Writer &writer, const VALUE hash) {
    writer.StartObject();
    rb_hash_foreach(hash, (int (*)(ANYARGS))write_hash_each<Writer>, reinterpret_cast<VALUE>((&writer)));
    writer.EndObject();
}

template <class Writer>
static inline void write_array(Writer &writer, const VALUE value) {
    writer.StartArray();
    const VALUE *ptr = rb_array_const_ptr(value);
    for (long i = 0; i < rb_array_len(value); i++, ptr++)
        write(writer, *ptr);
    writer.EndArray();
}

template <class Writer>
static inline void write_struct(Writer &writer, const VALUE value) {
    writer.StartObject();
    VALUE members = rb_struct_members(value);
    const VALUE *ptr = rb_array_const_ptr(members);
    for (long i = 0; i < rb_array_len(members); i++, ptr++) {
        if (RB_TYPE_P(*ptr, T_SYMBOL))
            write_key_str(writer, rb_sym_to_s(*ptr));
        else if (RB_TYPE_P(*ptr, T_STRING))
            write_key_str(writer, *ptr);
        else
            write_key_to_s(writer, *ptr);
        write(writer, rb_struct_aref(value, *ptr));
    }
    writer.EndObject();
}

template <class Writer>
static inline void write_usamin(Writer &writer, const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    value->value->Accept(writer);
}

/*
 * Generate the JSON string from Ruby data structures.
 *
 * @overload generate(obj)
 *   @param [Object] obj an object to serialize
 *   @return [String]
 */
VALUE w_generate(const VALUE, const VALUE value) {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    write(writer, value);
    return new_utf8_str(buf.GetString(), buf.GetSize());
}

/*
 * Generate the prettified JSON string from Ruby data structures.
 *
 * @overload pretty_generate(obj, opts = {})
 *   @param [Object] obj an object to serialize
 *   @param [::Hash] opts options
 *   @option opts [String] :indent ('  ') a string used to indent
 *   @option opts [Boolean] :single_line_array (false)
 *   @return [String]
 */
VALUE w_pretty_generate(const int argc, const VALUE *argv, const VALUE) {
    extern VALUE rb_eUsaminError;
    extern VALUE sym_indent, sym_single_line_array;

    VALUE value, options;
    rb_scan_args(argc, argv, "1:", &value, &options);
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

    char indent_char = ' ';
    unsigned int indent_count = 2;
    if (!NIL_P(options)) {
        VALUE v_indent = rb_hash_lookup(options, sym_indent);
        if (RTEST(v_indent)) {
            if (RB_FIXNUM_P(v_indent)) {
                long l = FIX2LONG(v_indent);
                indent_count = l > 0 ? static_cast<unsigned int>(l) : 0;
            } else {
                long vlen = RSTRING_LEN(v_indent);
                if (vlen == 0) {
                    indent_count = 0;
                } else {
                    const char *indent_str = RSTRING_PTR(v_indent);
                    switch (indent_str[0]) {
                    case ' ':
                    case '\t':
                    case '\r':
                    case '\n':
                        indent_char = indent_str[0];
                        break;
                    default:
                        rb_raise(rb_eUsaminError,
                                 ":indent must be a repetation of \" \", \"\\t\", \"\\r\" or \"\\n\".");
                    }
                    for (long i = 1; i < vlen; i++)
                        if (indent_str[0] != indent_str[i])
                            rb_raise(rb_eUsaminError,
                                     ":indent must be a repetation of \" \", \"\\t\", \"\\r\" or \"\\n\".");
                    indent_count = static_cast<unsigned int>(vlen);
                }
            }
        }
        if (RTEST(rb_hash_lookup(options, sym_single_line_array)))
            writer.SetFormatOptions(rapidjson::kFormatSingleLineArray);
    }
    writer.SetIndent(indent_char, indent_count);

    write(writer, value);
    return new_utf8_str(buf.GetString(), buf.GetSize());
}
