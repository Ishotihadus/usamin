#include <string.h>
#include <stdint.h>
#include <ruby.h>
#include <ruby/encoding.h>
#define RAPIDJSON_PARSE_DEFAULT_FLAGS \
    rapidjson::kParseFullPrecisionFlag | rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag
#define kParseFastFlags (rapidjson::kParseDefaultFlags & ~rapidjson::kParseFullPrecisionFlag)
#define RAPIDJSON_WRITE_DEFAULT_FLAGS rapidjson::kWriteNanAndInfFlag
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>

#if SIZEOF_VALUE < SIZEOF_VOIDP
#error SIZEOF_VOIDP must not be greater than SIZEOF_VALUE.
#endif


rb_encoding *utf8;
int utf8index;
ID id_to_s;
VALUE rb_cUsaminValue, rb_cUsaminHash, rb_cUsaminArray, rb_eUsaminError, rb_eParserError;
VALUE utf8value, sym_fast, sym_indent, sym_single_line_array;

class UsaminValue {
public:
    rapidjson::Value *value;
    bool free_flag;

    UsaminValue(rapidjson::Value *value = nullptr, bool free_flag = false);
    ~UsaminValue();
};

static inline VALUE get_utf8_str(VALUE str) {
    Check_Type(str, T_STRING);
    int encoding = rb_enc_get_index(str);
    if (encoding == utf8index || rb_enc_compatible(str, utf8value) == utf8)
        return str;
    else
        return rb_str_conv_enc(str, rb_enc_from_index(encoding), utf8);
}

static inline VALUE new_utf8_str(const char* cstr, const long len) {
    VALUE ret = rb_str_new(cstr, len);
    rb_enc_set_index(ret, utf8index);
    return ret;
}

static inline bool str_compare(const char* str1, const long len1, const char* str2, const long len2) {
    if (len1 != len2 || len1 < 0)
        return false;
    return memcmp(str1, str2, len1) == 0;
}

static inline void check_value(UsaminValue *ptr) {
    if (!ptr || !ptr->value)
        rb_raise(rb_eUsaminError, "Null Reference.");
}

static inline void check_object(UsaminValue *ptr) {
    check_value(ptr);
    if (!ptr->value->IsObject())
        rb_raise(rb_eUsaminError, "Value is not an object.");
}

static inline void check_array(UsaminValue *ptr) {
    check_value(ptr);
    if (!ptr->value->IsArray())
        rb_raise(rb_eUsaminError, "Value is not an array.");
}


static VALUE usamin_free(UsaminValue **ptr) {
    if (*ptr)
        delete *ptr;
    ruby_xfree(ptr);
    return Qnil;
}

static VALUE usamin_alloc(const VALUE klass) {
    UsaminValue** ptr = (UsaminValue**)ruby_xmalloc(sizeof(UsaminValue*));
    *ptr = nullptr;
    return Data_Wrap_Struct(klass, NULL, usamin_free, ptr);
}


static inline UsaminValue* get_value(VALUE value) {
    UsaminValue** ptr;
    Data_Get_Struct(value, UsaminValue*, ptr);
    return *ptr;
}

static inline void set_value(VALUE value, UsaminValue *usamin) {
    UsaminValue **ptr;
    Data_Get_Struct(value, UsaminValue*, ptr);
    *ptr = usamin;
}

static inline VALUE make_hash(UsaminValue *value) {
    VALUE ret = rb_obj_alloc(rb_cUsaminHash);
    set_value(ret, value);
    return ret;
}

static inline VALUE make_array(UsaminValue *value) {
    VALUE ret = rb_obj_alloc(rb_cUsaminArray);
    set_value(ret, value);
    return ret;
}

static inline rapidjson::ParseResult parse(rapidjson::Document &doc, const VALUE str, bool fast = false) {
    VALUE v = get_utf8_str(str);
    return fast ? doc.Parse<kParseFastFlags>(RSTRING_PTR(v), RSTRING_LEN(v)) : doc.Parse(RSTRING_PTR(v), RSTRING_LEN(v));
}


static inline VALUE eval_num(rapidjson::Value &value);
static inline VALUE eval_str(rapidjson::Value &value);
static inline VALUE eval_object(rapidjson::Value &value);
static inline VALUE eval_object_r(rapidjson::Value &value);
static inline VALUE eval_array(rapidjson::Value &value);
static inline VALUE eval_array_r(rapidjson::Value &value);

static VALUE eval(rapidjson::Value &value) {
    switch (value.GetType()) {
        case rapidjson::kObjectType:
            return make_hash(new UsaminValue(&value, false));
        case rapidjson::kArrayType:
            return make_array(new UsaminValue(&value, false));
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
            rb_raise(rb_eUsaminError, "Unknown Value Type: %d", value.GetType());
            return Qnil;
    }
}

static VALUE eval_r(rapidjson::Value &value) {
    switch (value.GetType()) {
        case rapidjson::kObjectType:
            return eval_object_r(value);
        case rapidjson::kArrayType:
            return eval_array_r(value);
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
            rb_raise(rb_eUsaminError, "Unknown Value Type: %d", value.GetType());
            return Qnil;
    }
}

static inline VALUE eval_num(rapidjson::Value &value) {
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

static inline VALUE eval_str(rapidjson::Value &value) {
    return new_utf8_str(value.GetString(), value.GetStringLength());
}

static inline VALUE eval_object(rapidjson::Value &value) {
    VALUE ret = rb_hash_new();
    for (auto &m : value.GetObject())
        rb_hash_aset(ret, eval_str(m.name), eval(m.value));
    return ret;
}

static inline VALUE eval_object_r(rapidjson::Value &value) {
    VALUE ret = rb_hash_new();
    for (auto &m : value.GetObject())
        rb_hash_aset(ret, eval_str(m.name), eval_r(m.value));
    return ret;
}

static inline VALUE eval_array(rapidjson::Value &value) {
    VALUE ret = rb_ary_new2(value.Size());
    for (auto &v : value.GetArray())
        rb_ary_push(ret, eval(v));
    return ret;
}

static inline VALUE eval_array_r(rapidjson::Value &value) {
    VALUE ret = rb_ary_new2(value.Size());
    for (auto &v : value.GetArray())
        rb_ary_push(ret, eval_r(v));
    return ret;
}


UsaminValue::UsaminValue(rapidjson::Value *value, bool free_flag) {
    this->value = value;
    this->free_flag = free_flag;
}

UsaminValue::~UsaminValue() {
    if (value && free_flag)
        delete value;
}

/*
 * Parse the JSON string into UsaminValue.
 * If the document root is not an object or an array, the same value as {#parse} will be returned.
 *
 * @overload load(source, opts = {})
 *   @param [String] source JSON string to parse
 *   @param [::Hash] opts options
 *   @option opts :fast fast mode (but not precise)
 *   @return [Object]
 */
static VALUE w_load(const int argc, VALUE *argv, const VALUE self) {
    VALUE source, options;
    rb_scan_args(argc, argv, "1:", &source, &options);
    rapidjson::Document *doc = new rapidjson::Document;
    auto result = parse(*doc, source, argc > 1 && RTEST(rb_hash_lookup(options, sym_fast)));
    if (!result) {
        delete doc;
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    }

    VALUE ret;
    switch (doc->GetType()) {
        case rapidjson::kObjectType:
            return make_hash(new UsaminValue(doc, true));
        case rapidjson::kArrayType:
            return make_array(new UsaminValue(doc, true));
        case rapidjson::kNullType:
            delete doc;
            return Qnil;
        case rapidjson::kFalseType:
            delete doc;
            return Qfalse;
        case rapidjson::kTrueType:
            delete doc;
            return Qtrue;
        case rapidjson::kNumberType:
            ret = eval_num(*doc);
            delete doc;
            return ret;
        case rapidjson::kStringType:
            ret = eval_str(*doc);
            delete doc;
            return ret;
        default:
            rb_raise(rb_eUsaminError, "Unknown Value Type: %d", doc->GetType());
            return Qnil;
    }
}

/*
 * Parse the JSON string into Ruby data structures.
 *
 * @overload parse(source, opts = {})
 *   @param [String] source a JSON string to parse
 *   @param [::Hash] opts options
 *   @option opts :fast fast mode (but not precise)
 *   @return [Object]
 */
static VALUE w_parse(const int argc, VALUE *argv, const VALUE self) {
    VALUE source, options;
    rb_scan_args(argc, argv, "1:", &source, &options);
    rapidjson::Document doc;
    auto result = parse(doc, source, argc > 1 && RTEST(rb_hash_lookup(options, sym_fast)));
    if (!result)
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    return eval_r(doc);
}

/*
 * Returns whether the value is array.
 */
static VALUE w_value_isarray(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return value->value->IsArray() ? Qtrue : Qfalse;
}

/*
 * Returns whether the value is object.
 */
static VALUE w_value_isobject(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return value->value->IsObject() ? Qtrue : Qfalse;
}

/*
 * Convert to Ruby data structures.
 *
 * @return [Object]
 */
static VALUE w_value_eval(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    if (value->value->IsObject())
        return eval_object(*(value->value));
    else if (value->value->IsArray())
        return eval_array(*(value->value));
    return Qnil;
}

/*
 * Convert to Ruby data structures recursively.
 *
 * @return [Object]
 */
static VALUE w_value_eval_r(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return eval_r(*(value->value));
}

/*
 * Always true.
 */
static VALUE w_value_isfrozen(const VALUE self) {
    return Qtrue;
}

template <class Writer> static void write_value(Writer&, rapidjson::Value&);

/*
 * Dumps data in JSON.
 *
 * @return [String]
 */
static VALUE w_value_marshal_dump(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    write_value(writer, *(value->value));
    return rb_str_new(buf.GetString(), buf.GetSize());
}

/*
 * Loads marshal data.
 *
 * @return [self]
 */
static VALUE w_value_marshal_load(const VALUE self, VALUE source) {
    Check_Type(source, T_STRING);
    rapidjson::Document *doc = new rapidjson::Document();
    rapidjson::ParseResult result = doc->Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag>(RSTRING_PTR(source), RSTRING_LEN(source));
    if (!result) {
        delete doc;
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    }
    if (doc->IsObject() || doc->IsArray()) {
        set_value(self, new UsaminValue(doc, true));
        return self;
    }
    auto type = doc->GetType();
    delete doc;
    rb_raise(rb_eUsaminError, "Invalid Value Type for marshal_load: %d", type);
    return Qnil;
}


/*
 * @overload [](name)
 *   @return [Object | nil]
 *
 * @note This method has linear time complexity.
 */
static VALUE w_hash_operator_indexer(const VALUE self, VALUE key) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE kvalue = get_utf8_str(key);
    for (auto &m : value->value->GetObject())
        if (str_compare(RSTRING_PTR(kvalue), RSTRING_LEN(kvalue), m.name.GetString(), m.name.GetStringLength()))
            return eval(m.value);
    return Qnil;
}

static VALUE hash_enum_size(const VALUE self, VALUE args, VALUE eobj) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return UINT2NUM(value->value->MemberCount());
}

/*
 * @yield [key, value]
 * @yieldparam key [String]
 * @return [Enumerator | self]
 */
static VALUE w_hash_each(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    if (rb_proc_arity(rb_block_proc()) > 1) {
        for (auto &m : value->value->GetObject()) {
            VALUE args[] = { eval_str(m.name), eval(m.value) };
            rb_yield_values2(2, args);
        }
    } else {
        for (auto &m : value->value->GetObject())
            rb_yield(rb_assoc_new(eval_str(m.name), eval(m.value)));
    }
    return self;
}

/*
 * @yield [key]
 * @yieldparam key [String]
 * @return [Enumerator | self]
 */
static VALUE w_hash_each_key(const VALUE self) {
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
static VALUE w_hash_each_value(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    for (auto &m : value->value->GetObject())
        rb_yield(eval(m.value));
    return self;
}

static VALUE w_hash_isempty(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return value->value->MemberCount() == 0 ? Qtrue : Qfalse;
}

/*
 * @note This method has linear time complexity.
 */
static VALUE w_hash_haskey(const VALUE self, VALUE name) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE key = get_utf8_str(name);
    for (auto &m : value->value->GetObject())
        if (str_compare(RSTRING_PTR(key), RSTRING_LEN(key), m.name.GetString(), m.name.GetStringLength()))
            return Qtrue;
    return Qfalse;
}

/*
 * @return [::Array<String>]
 */
static VALUE w_hash_keys(const VALUE self) {
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
static VALUE w_hash_length(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return UINT2NUM(value->value->MemberCount());
}

/*
 * Convert to Ruby Hash. Same as {Value#eval}.
 *
 * @return [::Hash]
 */
static VALUE w_hash_eval(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return eval_object(*(value->value));
}

/*
 * @return [::Array<Object>]
 */
static VALUE w_hash_values(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_ary_new2(value->value->MemberCount());
    for (auto &m : value->value->GetObject())
        rb_ary_push(ret, eval(m.value));
    return ret;
}


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
static VALUE w_array_operator_indexer(const int argc, VALUE* argv, const VALUE self) {
    rb_check_arity(argc, 1, 2);
    UsaminValue *value = get_value(self);
    check_array(value);
    if (argc == 2) {
        long beg = NUM2LONG(argv[0]);
        long len = NUM2LONG(argv[1]);
        if (beg < 0)
            beg = beg + value->value->Size();
        if (beg >= 0 && len >= 0) {
            long end = beg + len;
            if (end > value->value->Size())
                end = value->value->Size();
            VALUE ret = rb_ary_new2(end - beg);
            for (long i = beg; i < end; i++)
                rb_ary_push(ret, eval((*(value->value))[static_cast<unsigned int>(i)]));
            return ret;
        }
    } else if (rb_obj_is_kind_of(argv[0], rb_cRange)) {
        long beg, len;
        if (rb_range_beg_len(argv[0], &beg, &len, value->value->Size(), 0) == Qtrue) {
            VALUE ret = rb_ary_new2(len);
            for (long i = beg; i < beg + len; i++)
                rb_ary_push(ret, eval((*(value->value))[static_cast<unsigned int>(i)]));
            return ret;
        }
    } else {
        long l = NUM2LONG(argv[0]);
        if (l < 0)
            l = l + value->value->Size();
        if (l >= 0) {
            unsigned int i = static_cast<unsigned int>(l);
            if (i < value->value->Size())
                return eval((*(value->value))[i]);
        }
    }
    return Qnil;
}

static VALUE array_enum_size(const VALUE self, VALUE args, VALUE eobj) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return UINT2NUM(value->value->Size());
}

/*
 * @yield [value]
 * @return [Enumerator | self]
 */
static VALUE w_array_each(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, array_enum_size);
    for (auto &v : value->value->GetArray())
        rb_yield(eval(v));
    return self;
}

/*
 * @yield [index]
 * @yieldparam index [Integer]
 * @return [Enumerator | self]
 */
static VALUE w_array_each_index(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, array_enum_size);
    for (rapidjson::SizeType i = 0; i < value->value->Size(); i++)
        rb_yield(UINT2NUM(i));
    return self;
}

static VALUE w_array_isempty(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return value->value->Size() == 0 ? Qtrue : Qfalse;
}

/*
 * @yield [value]
 * @return [Integer]
 */
static VALUE w_array_index(int argc, VALUE* argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);

    if (argc == 1) {
        for (rapidjson::SizeType i = 0; i < value->value->Size(); i++) {
            if (rb_equal(argv[0], eval((*(value->value))[i])) == Qtrue)
                return UINT2NUM(i);
        }
        return Qnil;
    }

    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, array_enum_size);
    for (rapidjson::SizeType i = 0; i < value->value->Size(); i++) {
        if (RTEST(rb_yield(eval((*(value->value))[i]))))
            return UINT2NUM(i);
    }
    return Qnil;
}

/*
 * @overload first
 *   @return [Object | nil]
 *
 * @overload first(n)
 *   @return [::Array<Object>]
 */
static VALUE w_array_first(const int argc, VALUE* argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);

    if (argc == 0) {
        if (value->value->Size() == 0)
            return Qnil;
        return eval((*(value->value))[0]);
    }

    long l = NUM2LONG(argv[0]);
    if (l < 0)
        l = 0;
    else if (l > value->value->Size())
        l = value->value->Size();
    unsigned int li = static_cast<unsigned int>(l);
    VALUE ret = rb_ary_new2(li);
    for (unsigned int i = 0; i < li; i++)
        rb_ary_push(ret, eval((*(value->value))[i]));

    return ret;
}

/*
 * @return [Integer]
 */
static VALUE w_array_length(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return UINT2NUM(value->value->Size());
}

/*
 * Convert to Ruby data structures. Same as {Value#eval}.
 *
 * @return [::Array<Object>]
 */
static VALUE w_array_eval(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return eval_array(*(value->value));
}


template <class Writer> static void write(Writer&, const VALUE);

template <class Writer> static inline void write_str(Writer &writer, const VALUE value) {
    VALUE v = get_utf8_str(value);
    writer.String(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)));
}

template <class Writer> static inline void write_to_s(Writer &writer, const VALUE value) {
    write_str(writer, rb_funcall(value, id_to_s, 0));
}

template <class Writer> static inline void write_key_str(Writer &writer, const VALUE value) {
    VALUE v = get_utf8_str(value);
    writer.Key(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)));
}

template <class Writer> static inline void write_key_to_s(Writer &writer, const VALUE value) {
    write_key_str(writer, rb_funcall(value, id_to_s, 0));
}

template <class Writer> static inline void write_bignum(Writer &writer, const VALUE value) {
    VALUE v = rb_big2str(value, 10);
    writer.RawValue(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)), rapidjson::kNumberType);
}

template <class Writer> static inline int write_hash_each(VALUE key, VALUE value, Writer *writer) {
    if (RB_TYPE_P(key, T_STRING))
        write_key_str(*writer, key);
    else if (RB_TYPE_P(key, T_SYMBOL))
        write_key_str(*writer, rb_sym_to_s(key));
    else
        write_key_to_s(*writer, key);
    write(*writer, value);
    return ST_CONTINUE;
}

template <class Writer> static inline void write_hash(Writer &writer, const VALUE hash) {
    writer.StartObject();
    rb_hash_foreach(hash, (int (*)(ANYARGS))write_hash_each<Writer>, reinterpret_cast<VALUE>((&writer)));
    writer.EndObject();
}

template <class Writer> static inline void write_array(Writer &writer, const VALUE value) {
    writer.StartArray();
    const VALUE *ptr = rb_array_const_ptr(value);
    for (long i = 0; i < rb_array_len(value); i++, ptr++)
        write(writer, *ptr);
    writer.EndArray();
}

template <class Writer> static inline void write_struct(Writer &writer, const VALUE value) {
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

template <class Writer> static void write_value(Writer &writer, rapidjson::Value &value) {
    switch (value.GetType()) {
        case rapidjson::kObjectType:
            writer.StartObject();
            for (auto &m : value.GetObject()) {
                writer.Key(m.name.GetString(), m.name.GetStringLength());
                write_value(writer, m.value);
            }
            writer.EndObject();
            break;
        case rapidjson::kArrayType:
            writer.StartArray();
            for (auto &v : value.GetArray())
                write_value(writer, v);
            writer.EndArray();
            break;
        case rapidjson::kNullType:
            writer.Null();
            break;
        case rapidjson::kFalseType:
            writer.Bool(false);
            break;
        case rapidjson::kTrueType:
            writer.Bool(true);
            break;
        case rapidjson::kNumberType:
            if (value.IsInt())
                writer.Int(value.GetInt());
            else if (value.IsUint())
                writer.Uint(value.GetUint());
            else if (value.IsInt64())
                writer.Int64(value.GetInt64());
            else if (value.IsUint64())
                writer.Uint64(value.GetUint64());
            else
                writer.Double(value.GetDouble());
            break;
        case rapidjson::kStringType:
            writer.String(value.GetString(), value.GetStringLength());
            break;
        default:
            rb_raise(rb_eUsaminError, "Unknown Value Type: %d", value.GetType());
    }
}

template <class Writer> static inline void write_usamin(Writer &writer, const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    write_value(writer, *(value->value));
}

template <class Writer> static void write(Writer &writer, const VALUE value) {
    if (value == Qnil) {
        writer.Null();
    } else if (value == Qfalse) {
        writer.Bool(false);
    } else if (value == Qtrue) {
        writer.Bool(true);
    } else if (RB_FIXNUM_P(value)) {
        writer.Int64(FIX2LONG(value));
    } else if (RB_FLOAT_TYPE_P(value)) {
        writer.Double(NUM2DBL(value));
    } else if (RB_STATIC_SYM_P(value)) {
        write_str(writer, rb_sym_to_s(value));
    } else {
        switch (RB_BUILTIN_TYPE(value)) {
            case T_STRING:
                write_str(writer, value);
                break;
            case T_HASH:
                write_hash(writer, value);
                break;
            case T_ARRAY:
                write_array(writer, value);
                break;
            case T_BIGNUM:
                write_bignum(writer, value);
                break;
            case T_STRUCT:
                write_struct(writer, value);
                break;
            default:
                if (rb_obj_is_kind_of(value, rb_cUsaminValue))
                    write_usamin(writer, value);
                else
                    write_to_s(writer, value);
                break;
        }
    }
}

/*
 * Generate the JSON string from Ruby data structures.
 *
 * @overload generate(obj)
 *   @param [Object] obj an object to serialize
 *   @return [String]
 */
static VALUE w_generate(const VALUE self, VALUE value) {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    write(writer, value);
    return new_utf8_str(buf.GetString(), buf.GetSize());
}

/*
 * Generate the prettified JSON string from Ruby data structures.
 *
 * @overload generate(obj, opts = {})
 *   @param [Object] obj an object to serialize
 *   @param [::Hash] opts options
 *   @option opts [String] :indent ('  ') a string used to indent
 *   @return [String]
 */
static VALUE w_pretty_generate(const int argc, VALUE *argv, const VALUE self) {
    VALUE value, options;
    rb_scan_args(argc, argv, "1:", &value, &options);
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

    char indent_char = ' ';
    unsigned int indent_count = 2;
    if (argc > 1) {
        VALUE v_indent = rb_hash_lookup(options, sym_indent);
        VALUE v_single_line_array = rb_hash_lookup(options, sym_single_line_array);

        if (RTEST(v_indent)) {
            if (RB_FIXNUM_P(v_indent)) {
                long l = FIX2LONG(v_indent);
                indent_count = l > 0 ? static_cast<unsigned int>(l) : 0;
            } else {
                VALUE v = get_utf8_str(v_indent);
                long vlen = RSTRING_LEN(v);
                if (vlen == 0) {
                    indent_count = 0;
                } else {
                    const char *indent_str = RSTRING_PTR(v);
                    switch (indent_str[0]) {
                        case '\0':
                            indent_count = 0;
                            break;
                        case ' ':
                        case '\t':
                        case '\r':
                        case '\n':
                            indent_char = indent_str[0];
                            break;
                        default:
                            rb_raise(rb_eUsaminError, ":indent must be a repetation of \" \", \"\\t\", \"\\r\" or \"\\n\".");
                    }

                    for (long i = 1; i < vlen; i++)
                        if (indent_str[0] != indent_str[i])
                            rb_raise(rb_eUsaminError, ":indent must be a repetation of \" \", \"\\t\", \"\\r\" or \"\\n\".");
                    indent_count = static_cast<unsigned int>(vlen);
                }
            }
        }

        if (RTEST(v_single_line_array))
            writer.SetFormatOptions(rapidjson::kFormatSingleLineArray);
    }
    writer.SetIndent(indent_char, indent_count);

    write(writer, value);
    return new_utf8_str(buf.GetString(), buf.GetSize());
}


extern "C" void Init_usamin(void) {
    utf8 = rb_utf8_encoding();
    utf8index = rb_enc_to_index(utf8);
    utf8value = rb_enc_from_encoding(utf8);
    sym_fast = rb_id2sym(rb_intern("fast"));
    sym_indent = rb_id2sym(rb_intern("indent"));
    sym_single_line_array = rb_id2sym(rb_intern("single_line_array"));
    id_to_s = rb_intern("to_s");

    VALUE rb_mUsamin = rb_define_module("Usamin");
    rb_define_module_function(rb_mUsamin, "load", RUBY_METHOD_FUNC(w_load), -1);
    rb_define_module_function(rb_mUsamin, "parse", RUBY_METHOD_FUNC(w_parse), -1);
    rb_define_module_function(rb_mUsamin, "generate", RUBY_METHOD_FUNC(w_generate), 1);
    rb_define_module_function(rb_mUsamin, "pretty_generate", RUBY_METHOD_FUNC(w_pretty_generate), -1);

    rb_cUsaminValue = rb_define_class_under(rb_mUsamin, "Value", rb_cObject);
    rb_define_alloc_func(rb_cUsaminValue, usamin_alloc);
    rb_undef_method(rb_cUsaminValue, "initialize");
    rb_define_method(rb_cUsaminValue, "array?", RUBY_METHOD_FUNC(w_value_isarray), 0);
    rb_define_method(rb_cUsaminValue, "hash?", RUBY_METHOD_FUNC(w_value_isobject), 0);
    rb_define_method(rb_cUsaminValue, "object?", RUBY_METHOD_FUNC(w_value_isobject), 0);
    rb_define_method(rb_cUsaminValue, "eval", RUBY_METHOD_FUNC(w_value_eval), 0);
    rb_define_method(rb_cUsaminValue, "eval_r", RUBY_METHOD_FUNC(w_value_eval_r), 0);
    rb_define_method(rb_cUsaminValue, "frozen?", RUBY_METHOD_FUNC(w_value_isfrozen), 0);
    rb_define_method(rb_cUsaminValue, "marshal_dump", RUBY_METHOD_FUNC(w_value_marshal_dump), 0);
    rb_define_method(rb_cUsaminValue, "marshal_load", RUBY_METHOD_FUNC(w_value_marshal_load), 1);

    rb_cUsaminHash = rb_define_class_under(rb_mUsamin, "Hash", rb_cUsaminValue);
    rb_include_module(rb_cUsaminHash, rb_mEnumerable);
    rb_define_alloc_func(rb_cUsaminHash, usamin_alloc);
    rb_undef_method(rb_cUsaminHash, "initialize");
    rb_define_method(rb_cUsaminHash, "[]", RUBY_METHOD_FUNC(w_hash_operator_indexer), 1);
    rb_define_method(rb_cUsaminHash, "each", RUBY_METHOD_FUNC(w_hash_each), 0);
    rb_define_method(rb_cUsaminHash, "each_pair", RUBY_METHOD_FUNC(w_hash_each), 0);
    rb_define_method(rb_cUsaminHash, "each_key", RUBY_METHOD_FUNC(w_hash_each_key), 0);
    rb_define_method(rb_cUsaminHash, "each_value", RUBY_METHOD_FUNC(w_hash_each_value), 0);
    rb_define_method(rb_cUsaminHash, "empty?", RUBY_METHOD_FUNC(w_hash_isempty), 0);
    rb_define_method(rb_cUsaminHash, "has_key?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "include?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "key?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "member?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "keys", RUBY_METHOD_FUNC(w_hash_keys), 0);
    rb_define_method(rb_cUsaminHash, "length", RUBY_METHOD_FUNC(w_hash_length), 0);
    rb_define_method(rb_cUsaminHash, "size", RUBY_METHOD_FUNC(w_hash_length), 0);
    rb_define_method(rb_cUsaminHash, "to_h", RUBY_METHOD_FUNC(w_hash_eval), 0);
    rb_define_method(rb_cUsaminHash, "to_hash", RUBY_METHOD_FUNC(w_hash_eval), 0);
    rb_define_method(rb_cUsaminHash, "values", RUBY_METHOD_FUNC(w_hash_values), 0);

    rb_cUsaminArray = rb_define_class_under(rb_mUsamin, "Array", rb_cUsaminValue);
    rb_include_module(rb_cUsaminArray, rb_mEnumerable);
    rb_define_alloc_func(rb_cUsaminArray, usamin_alloc);
    rb_undef_method(rb_cUsaminArray, "initialize");
    rb_define_method(rb_cUsaminArray, "[]", RUBY_METHOD_FUNC(w_array_operator_indexer), -1);
    rb_define_method(rb_cUsaminArray, "each", RUBY_METHOD_FUNC(w_array_each), 0);
    rb_define_method(rb_cUsaminArray, "each_index", RUBY_METHOD_FUNC(w_array_each_index), 0);
    rb_define_method(rb_cUsaminArray, "empty?", RUBY_METHOD_FUNC(w_array_isempty), 0);
    rb_define_method(rb_cUsaminArray, "index", RUBY_METHOD_FUNC(w_array_index), -1);
    rb_define_method(rb_cUsaminArray, "find_index", RUBY_METHOD_FUNC(w_array_index), -1);
    rb_define_method(rb_cUsaminArray, "first", RUBY_METHOD_FUNC(w_array_first), -1);
    rb_define_method(rb_cUsaminArray, "length", RUBY_METHOD_FUNC(w_array_length), 0);
    rb_define_method(rb_cUsaminArray, "size", RUBY_METHOD_FUNC(w_array_length), 0);
    rb_define_method(rb_cUsaminArray, "to_a", RUBY_METHOD_FUNC(w_array_eval), 0);
    rb_define_method(rb_cUsaminArray, "to_ary", RUBY_METHOD_FUNC(w_array_eval), 0);

    rb_eUsaminError = rb_define_class_under(rb_mUsamin, "UsaminError", rb_eStandardError);
    rb_eParserError = rb_define_class_under(rb_mUsamin, "ParserError", rb_eUsaminError);
}
