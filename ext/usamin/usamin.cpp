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

class RubyCrtAllocator {
public:
    static const bool kNeedFree = true;
    void* Malloc(size_t size) {
        if (size)
            return ruby_xmalloc(size);
        else
            return nullptr;
    }
    void* Realloc(void* originalPtr, size_t, size_t newSize) {
        if (newSize == 0) {
            ruby_xfree(originalPtr);
            return nullptr;
        }
        return ruby_xrealloc(originalPtr, newSize);
    }
    static void Free(void *ptr) { ruby_xfree(ptr); }
};

typedef rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<RubyCrtAllocator>> RubynizedValue;
typedef rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<RubyCrtAllocator>> RubynizedDocument;

rb_encoding *utf8;
int utf8index;
ID id_dig, id_to_s;
VALUE rb_cUsaminValue, rb_cUsaminHash, rb_cUsaminArray, rb_eUsaminError, rb_eParserError;
VALUE utf8value, sym_fast, sym_indent, sym_single_line_array;

class UsaminValue {
public:
    RubynizedValue *value;
    VALUE root_document;
    bool free_flag;

    UsaminValue(RubynizedValue *value = nullptr, const bool free_flag = false, const VALUE root_document = Qnil);
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

static inline bool str_compare_xx(VALUE str1, const RubynizedValue &str2) {
    if (RB_TYPE_P(str1, T_STRING))
        str1 = get_utf8_str(str1);
    else if (SYMBOL_P(str1))
        str1 = get_utf8_str(rb_sym_to_s(str1));
    else
        str1 = get_utf8_str(StringValue(str1));
    return str_compare(RSTRING_PTR(str1), RSTRING_LEN(str1), str2.GetString(), str2.GetStringLength());
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


static void usamin_free(void *p) {
    if (!p)
        return;
    UsaminValue **ptr = (UsaminValue**)p;
    if (*ptr)
        delete *ptr;
    ruby_xfree(p);
}

static void usamin_mark(void *p) {
    if (!p)
        return;
    UsaminValue **ptr = (UsaminValue**)p;
    if (*ptr && !((*ptr)->free_flag))
        rb_gc_mark((*ptr)->root_document);
}

static size_t usamin_size(const void *p) {
    if (!p)
        return 0;
    UsaminValue **ptr = (UsaminValue**)p;
    size_t s = 0;
    if (*ptr) {
        s += sizeof(UsaminValue);
        if ((*ptr)->free_flag)
            s += ((RubynizedDocument*)(*ptr)->value)->GetAllocator().Capacity() + ((RubynizedDocument*)(*ptr)->value)->GetStackCapacity();
    }
    return s;
}

static const rb_data_type_t rb_usamin_value_type = { "UsaminValue", { usamin_mark, usamin_free, usamin_size }, nullptr, nullptr, RUBY_TYPED_FREE_IMMEDIATELY };

static VALUE usamin_alloc(const VALUE klass) {
    UsaminValue** ptr;
    VALUE ret = TypedData_Make_Struct(klass, UsaminValue*, &rb_usamin_value_type, ptr);
    *ptr = nullptr;
    return ret;
}


static inline UsaminValue* get_value(VALUE value) {
    UsaminValue **ptr;
    TypedData_Get_Struct(value, UsaminValue*, &rb_usamin_value_type, ptr);
    return *ptr;
}

static inline void set_value(VALUE value, UsaminValue *usamin) {
    UsaminValue **ptr;
    TypedData_Get_Struct(value, UsaminValue*, &rb_usamin_value_type, ptr);
    *ptr = usamin;
}

static inline VALUE make_hash(UsaminValue *value, bool is_root = false) {
    VALUE ret = rb_obj_alloc(rb_cUsaminHash);
    set_value(ret, value);
    if (is_root)
        value->root_document = ret;
    return ret;
}

static inline VALUE make_array(UsaminValue *value, bool is_root = false) {
    VALUE ret = rb_obj_alloc(rb_cUsaminArray);
    set_value(ret, value);
    if (is_root)
        value->root_document = ret;
    return ret;
}

static inline rapidjson::ParseResult parse(RubynizedDocument &doc, const VALUE str, bool fast = false) {
    volatile VALUE v = get_utf8_str(str);
    return fast ? doc.Parse<kParseFastFlags>(RSTRING_PTR(v), RSTRING_LEN(v)) : doc.Parse(RSTRING_PTR(v), RSTRING_LEN(v));
}


static inline VALUE eval_num(RubynizedValue&);
static inline VALUE eval_str(RubynizedValue&);
static inline VALUE eval_object_r(RubynizedValue&);
static inline VALUE eval_array_r(RubynizedValue&);

static VALUE eval(RubynizedValue &value, VALUE root_document) {
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
            rb_raise(rb_eUsaminError, "Unknown Value Type: %d", value.GetType());
            return Qnil;
    }
}

static VALUE eval_r(RubynizedValue &value) {
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

static inline VALUE eval_object(RubynizedValue &value, const VALUE root_document) {
    VALUE ret = rb_hash_new();
    for (auto &m : value.GetObject())
        rb_hash_aset(ret, eval_str(m.name), eval(m.value, root_document));
    return ret;
}

static inline VALUE eval_object_r(RubynizedValue &value) {
    VALUE ret = rb_hash_new();
    for (auto &m : value.GetObject())
        rb_hash_aset(ret, eval_str(m.name), eval_r(m.value));
    return ret;
}

static inline VALUE eval_array(RubynizedValue &value, const VALUE root_document) {
    VALUE ret = rb_ary_new2(value.Size());
    for (auto &v : value.GetArray())
        rb_ary_push(ret, eval(v, root_document));
    return ret;
}

static inline VALUE eval_array_r(RubynizedValue &value) {
    VALUE ret = rb_ary_new2(value.Size());
    for (auto &v : value.GetArray())
        rb_ary_push(ret, eval_r(v));
    return ret;
}


template <class Writer> static inline void write_str(Writer&, const VALUE);
template <class Writer> static inline void write_hash(Writer&, const VALUE);
template <class Writer> static inline void write_array(Writer&, const VALUE);
template <class Writer> static inline void write_struct(Writer&, const VALUE);
template <class Writer> static inline void write_usamin(Writer&, const VALUE);
template <class Writer> static inline void write_to_s(Writer&, const VALUE);

template <class Writer> static void write(Writer &writer, const VALUE value) {
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
        case RUBY_T_BIGNUM:
            {
                VALUE v = rb_big2str(value, 10);
                writer.RawValue(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)), rapidjson::kNumberType);
            }
            break;
        default:
            if (rb_obj_is_kind_of(value, rb_cUsaminValue))
                write_usamin(writer, value);
            else
                write_to_s(writer, value);
            break;
    }
}

template <class Writer> static inline void write_str(Writer &writer, const VALUE value) {
    VALUE v = get_utf8_str(value);
    writer.String(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)));
}

template <class Writer> static inline void write_key_str(Writer &writer, const VALUE value) {
    VALUE v = get_utf8_str(value);
    writer.Key(RSTRING_PTR(v), static_cast<unsigned int>(RSTRING_LEN(v)));
}

template <class Writer> static inline void write_key_to_s(Writer &writer, const VALUE value) {
    write_key_str(writer, rb_funcall(value, id_to_s, 0));
}

template <class Writer> static inline int write_hash_each(const VALUE key, const VALUE value, Writer *writer) {
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

template <class Writer> static void write_value(Writer &writer, RubynizedValue &value) {
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

template <class Writer> static inline void write_to_s(Writer &writer, const VALUE value) {
    write_str(writer, rb_funcall(value, id_to_s, 0));
}


UsaminValue::UsaminValue(RubynizedValue *value, const bool free_flag, const VALUE root_document) {
    this->value = value;
    this->free_flag = free_flag;
    this->root_document = root_document;
}

UsaminValue::~UsaminValue() {
    if (value && free_flag)
        delete (RubynizedDocument*)value;
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
static VALUE w_load(const int argc, const VALUE *argv, const VALUE self) {
    VALUE source, options;
    rb_scan_args(argc, argv, "1:", &source, &options);
    RubynizedDocument *doc = new RubynizedDocument;
    auto result = parse(*doc, source, argc > 1 && RTEST(rb_hash_lookup(options, sym_fast)));
    if (!result) {
        delete doc;
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    }

    VALUE ret;
    switch (doc->GetType()) {
        case rapidjson::kObjectType:
            return make_hash(new UsaminValue(doc, true), true);
        case rapidjson::kArrayType:
            return make_array(new UsaminValue(doc, true), true);
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
static VALUE w_parse(const int argc, const VALUE *argv, const VALUE self) {
    VALUE source, options;
    rb_scan_args(argc, argv, "1:", &source, &options);
    RubynizedDocument doc;
    auto result = parse(doc, source, argc > 1 && RTEST(rb_hash_lookup(options, sym_fast)));
    if (!result)
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    return eval_r(doc);
}

/*
 * Returns whether the value is equals to the other value.
 *
 * @return [Boolean]
 */
static VALUE w_value_equals(const VALUE self, const VALUE other) {
    UsaminValue *value_self = get_value(self);
    UsaminValue *value_other = get_value(other);
    check_value(value_self);
    check_value(value_other);
    return value_self->value->operator==(*value_other->value) ? Qtrue : Qfalse;
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
        return eval_object(*(value->value), value->root_document);
    else if (value->value->IsArray())
        return eval_array(*(value->value), value->root_document);
    else
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

/*
 * Returns root object.
 *
 * @return [UsaminValue]
 */
static VALUE w_value_root(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return value->root_document;
}

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
    write_value(writer, *value->value);
    return rb_str_new(buf.GetString(), buf.GetSize());
}

/*
 * Loads marshal data.
 */
static VALUE w_value_marshal_load(const VALUE self, const VALUE source) {
    Check_Type(source, T_STRING);
    RubynizedDocument *doc = new RubynizedDocument();
    rapidjson::ParseResult result = doc->Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag>(RSTRING_PTR(source), RSTRING_LEN(source));
    if (!result) {
        delete doc;
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    }
    if (doc->IsObject() || doc->IsArray()) {
        UsaminValue *value = new UsaminValue(doc, true);
        set_value(self, value);
        value->root_document = self;
    } else {
        auto type = doc->GetType();
        delete doc;
        rb_raise(rb_eUsaminError, "Invalid Value Type for marshal_load: %d", type);
    }
    return Qnil;
}


/*
 * @return [Object | nil]
 *
 * @note This method has linear time complexity.
 */
static VALUE w_hash_operator_indexer(const VALUE self, const VALUE key) {
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
static VALUE w_hash_assoc(const VALUE self, const VALUE key) {
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
static VALUE w_hash_compact(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE hash = rb_hash_new();
    for (auto &m : value->value->GetObject())
        if (!m.value.IsNull())
            rb_hash_aset(hash, eval(m.name, value->root_document), eval(m.value, value->root_document));
    return hash;
}

/*
 * @return [Object | nil]
 */
static VALUE w_hash_dig(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, UNLIMITED_ARGUMENTS);
    VALUE value = w_hash_operator_indexer(self, argv[0]);
    if (argc == 1)
        return value;
    else if (value == Qnil)
        return Qnil;
    else
        return rb_funcall3(value, id_dig, argc - 1, argv + 1);
}

static VALUE hash_enum_size(const VALUE self, const VALUE args, const VALUE eobj) {
    return UINT2NUM(get_value(self)->value->MemberCount());
}

/*
 * @yield [key, value]
 * @yieldparam key [String]
 * @yieldparam value [Object]
 * @return [Enumerator | self]
 */
static VALUE w_hash_each(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    if (rb_proc_arity(rb_block_proc()) > 1) {
        for (auto &m : value->value->GetObject()) {
            VALUE args[] = { eval_str(m.name), eval(m.value, value->root_document) };
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
        rb_yield(eval(m.value, value->root_document));
    return self;
}

static VALUE w_hash_isempty(const VALUE self) {
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
static VALUE w_hash_fetch(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, 2);
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (str_compare_xx(argv[0], m.name))
            return eval(m.value, value->root_document);
    return argc == 2 ? argv[1] : rb_block_given_p() ? rb_yield(argv[0]) : Qnil;
}

/*
 * @param [String] key
 * @return [::Array<Object>]
 */
static VALUE w_hash_fetch_values(const int argc, VALUE *argv, const VALUE self) {
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
 * @note This method has linear time complexity.
 */
static VALUE w_hash_haskey(const VALUE self, const VALUE key) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (str_compare_xx(key, m.name))
            return Qtrue;
    return Qfalse;
}

static VALUE w_hash_hasvalue(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (rb_equal(val, eval_r(m.value)))
            return Qtrue;
    return Qfalse;
}

/*
 * @return [String | nil]
 */
static VALUE w_hash_key(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (rb_equal(val, eval_r(m.value)))
            return eval_str(m.name);
    return Qnil;
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
 * @return [::Array | nil]
 */
static VALUE w_hash_rassoc(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_object(value);
    for (auto &m : value->value->GetObject())
        if (rb_funcall(val, rb_intern("=="), 1, eval_r(m.value)))
            return rb_assoc_new(eval_str(m.name), eval(m.value, value->root_document));
    return Qnil;
}

/*
 * @yield [key, value]
 * @yieldparam key [String]
 * @yieldparam value [Object]
 * @return [Enumerator | ::Hash]
 */
static VALUE w_hash_select(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    VALUE hash = rb_hash_new();
    if (rb_proc_arity(rb_block_proc()) > 1) {
        for (auto &m : value->value->GetObject()) {
            VALUE args[] = { eval_str(m.name), eval(m.value, value->root_document) };
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
static VALUE w_hash_reject(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    RETURN_SIZED_ENUMERATOR(self, 0, nullptr, hash_enum_size);
    VALUE hash = rb_hash_new();
    if (rb_proc_arity(rb_block_proc()) > 1) {
        for (auto &m : value->value->GetObject()) {
            VALUE args[] = { eval_str(m.name), eval(m.value, value->root_document) };
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
static VALUE w_hash_slice(const int argc, const VALUE *argv, const VALUE self) {
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
static VALUE w_hash_eval(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    return eval_object(*(value->value), value->root_document);
}

/*
 * @return [::Hash]
 */
static VALUE w_hash_merge(const int argc, const VALUE *argv, const VALUE self) {
    static ID s = rb_intern("merge");
    return rb_funcall2(w_hash_eval(self), s, argc, argv);
}

/*
 * @return [::Array<Object>]
 */
static VALUE w_hash_values(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_object(value);
    VALUE ret = rb_ary_new2(value->value->MemberCount());
    for (auto &m : value->value->GetObject())
        rb_ary_push(ret, eval(m.value, value->root_document));
    return ret;
}

/*
 * @yield [key]
 * @yieldparam key [String]
 * @return [Enumerator | ::Hash]
 */
static VALUE w_hash_transform_keys(const VALUE self) {
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
static VALUE w_hash_transform_values(const VALUE self) {
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
static VALUE w_hash_values_at(const int argc, const VALUE *argv, const VALUE self) {
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
static VALUE w_array_operator_indexer(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, 2);
    UsaminValue *value = get_value(self);
    check_array(value);
    rapidjson::SizeType sz = value->value->Size();
    if (argc == 2) {
        long beg = FIX2LONG(argv[0]);
        long len = FIX2LONG(argv[1]);
        if (beg < 0)
            beg += sz;
        if (beg >= 0 && len >= 0) {
            rapidjson::SizeType end = static_cast<rapidjson::SizeType>(beg + len);
            if (end > sz)
                end = sz;
            VALUE ret = rb_ary_new2(end - beg);
            for (rapidjson::SizeType i = static_cast<rapidjson::SizeType>(beg); i < end; i++)
                rb_ary_push(ret, eval((*value->value)[i], value->root_document));
            return ret;
        }
    } else if (rb_obj_is_kind_of(argv[0], rb_cRange)) {
        long beg, len;
        if (rb_range_beg_len(argv[0], &beg, &len, sz, 0) == Qtrue) {
            VALUE ret = rb_ary_new2(len);
            for (rapidjson::SizeType i = static_cast<rapidjson::SizeType>(beg); i < beg + len; i++)
                rb_ary_push(ret, eval((*value->value)[i], value->root_document));
            return ret;
        }
    } else {
        long l = FIX2LONG(argv[0]);
        if (l < 0)
            l += sz;
        if (0 <= l && l < sz)
            return eval((*value->value)[static_cast<rapidjson::SizeType>(l)], value->root_document);
    }
    return Qnil;
}

/*
 * @param [Integer] nth
 * @return [Object]
 */
static VALUE w_array_at(const VALUE self, const VALUE nth) {
    UsaminValue *value = get_value(self);
    check_array(value);
    long l = FIX2LONG(nth);
    rapidjson::SizeType sz = value->value->Size();
    if (l < 0)
        l += sz;
    if (0 <= l && l < sz)
        return eval((*value->value)[static_cast<rapidjson::SizeType>(l)], value->root_document);
    return Qnil;
}

/*
 * @return [::Array]
 */
static VALUE w_array_compact(const VALUE self, const VALUE nth) {
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
static VALUE w_array_dig(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, UNLIMITED_ARGUMENTS);
    VALUE value = w_array_at(self, argv[0]);
    if (argc == 1)
        return value;
    else if (value == Qnil)
        return Qnil;
    else
        return rb_funcall3(value, id_dig, argc - 1, argv + 1);
}

static VALUE array_enum_size(const VALUE self, const VALUE args, const VALUE eobj) {
    return UINT2NUM(get_value(self)->value->Size());
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
        rb_yield(eval(v, value->root_document));
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
static VALUE w_array_fetch(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 1, 2);
    UsaminValue *value = get_value(self);
    check_array(value);
    rapidjson::SizeType sz = value->value->Size();

    long l = FIX2LONG(argv[0]);
    if (l < 0)
        l += sz;
    if (0 <= l && l < sz)
        return eval((*value->value)[static_cast<rapidjson::SizeType>(l)], value->root_document);

    if (argc == 2)
        return argv[1];
    else if (rb_block_given_p())
        return rb_yield(argv[0]);
    else
        rb_raise(rb_eIndexError, "index %ld outside of array bounds: %ld...%u", FIX2LONG(argv[0]), -static_cast<long>(sz), sz);
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
static VALUE w_array_find_index(const int argc, const VALUE *argv, const VALUE self) {
    rb_check_arity(argc, 0, 1);
    UsaminValue *value = get_value(self);
    check_array(value);

    if (argc == 1) {
        for (rapidjson::SizeType i = 0; i < value->value->Size(); i++) {
            if (rb_equal(argv[0], eval_r((*value->value)[i])) == Qtrue)
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

/*
 * @overload index(val)
 *   @param [Object] val
 *   @return [Integer | nil]
 *
 * @overload index
 *   @yield [item]
 *   @yieldparam item [Object]
 *   @return [Integer | nil]
 */
static VALUE w_array_index(const int argc, const VALUE *argv, const VALUE self) {
    return w_array_find_index(argc, argv, self);
}

/*
 * @overload first
 *   @return [Object | nil]
 *
 * @overload first(n)
 *   @return [::Array<Object>]
 */
static VALUE w_array_first(const int argc, const VALUE *argv, const VALUE self) {
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
static VALUE w_array_include(const VALUE self, const VALUE val) {
    UsaminValue *value = get_value(self);
    check_array(value);
    for (auto &v : value->value->GetArray())
        if (rb_equal(val, eval_r(v)))
            return Qtrue;
    return Qfalse;
}

/*
 * @overload last
 *   @return [Object | nil]
 *
 * @overload last(n)
 *   @return [::Array<Object>]
 */
static VALUE w_array_last(const int argc, const VALUE *argv, const VALUE self) {
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
static VALUE w_array_length(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return UINT2NUM(value->value->Size());
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
static VALUE w_array_slice(const int argc, const VALUE *argv, const VALUE self) {
    return w_array_operator_indexer(argc, argv, self);
}

/*
 * Convert to Ruby data structures. Same as {Value#eval}.
 *
 * @return [::Array<Object>]
 */
static VALUE w_array_eval(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_array(value);
    return eval_array(*(value->value), value->root_document);
}


/*
 * Generate the JSON string from Ruby data structures.
 *
 * @overload generate(obj)
 *   @param [Object] obj an object to serialize
 *   @return [String]
 */
static VALUE w_generate(const VALUE self, const VALUE value) {
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
static VALUE w_pretty_generate(const int argc, const VALUE *argv, const VALUE self) {
    VALUE value, options;
    rb_scan_args(argc, argv, "1:", &value, &options);
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

    char indent_char = ' ';
    unsigned int indent_count = 2;
    if (argc > 1) {
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
                            rb_raise(rb_eUsaminError, ":indent must be a repetation of \" \", \"\\t\", \"\\r\" or \"\\n\".");
                    }
                    for (long i = 1; i < vlen; i++)
                        if (indent_str[0] != indent_str[i])
                            rb_raise(rb_eUsaminError, ":indent must be a repetation of \" \", \"\\t\", \"\\r\" or \"\\n\".");
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


extern "C" void Init_usamin(void) {
    utf8 = rb_utf8_encoding();
    utf8index = rb_enc_to_index(utf8);
    utf8value = rb_enc_from_encoding(utf8);
    sym_fast = rb_id2sym(rb_intern("fast"));
    sym_indent = rb_id2sym(rb_intern("indent"));
    sym_single_line_array = rb_id2sym(rb_intern("single_line_array"));
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
    rb_define_method(rb_cUsaminValue, "==", RUBY_METHOD_FUNC(w_value_equals), 1);
    rb_define_method(rb_cUsaminValue, "===", RUBY_METHOD_FUNC(w_value_equals), 1);
    rb_define_method(rb_cUsaminValue, "array?", RUBY_METHOD_FUNC(w_value_isarray), 0);
    rb_define_method(rb_cUsaminValue, "hash?", RUBY_METHOD_FUNC(w_value_isobject), 0);
    rb_define_method(rb_cUsaminValue, "object?", RUBY_METHOD_FUNC(w_value_isobject), 0);
    rb_define_method(rb_cUsaminValue, "eval", RUBY_METHOD_FUNC(w_value_eval), 0);
    rb_define_method(rb_cUsaminValue, "eval_r", RUBY_METHOD_FUNC(w_value_eval_r), 0);
    rb_define_method(rb_cUsaminValue, "frozen?", RUBY_METHOD_FUNC(w_value_isfrozen), 0);
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
    rb_define_method(rb_cUsaminHash, "has_key?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "include?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "key?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "member?", RUBY_METHOD_FUNC(w_hash_haskey), 1);
    rb_define_method(rb_cUsaminHash, "has_value?", RUBY_METHOD_FUNC(w_hash_hasvalue), 1);
    rb_define_method(rb_cUsaminHash, "value?", RUBY_METHOD_FUNC(w_hash_hasvalue), 1);
    rb_define_method(rb_cUsaminHash, "key", RUBY_METHOD_FUNC(w_hash_key), 1);
    rb_define_method(rb_cUsaminHash, "index", RUBY_METHOD_FUNC(w_hash_key), 1);
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
    rb_define_method(rb_cUsaminArray, "index", RUBY_METHOD_FUNC(w_array_index), -1);
    rb_define_method(rb_cUsaminArray, "first", RUBY_METHOD_FUNC(w_array_first), -1);
    rb_define_method(rb_cUsaminArray, "include?", RUBY_METHOD_FUNC(w_array_include), 1);
    rb_define_method(rb_cUsaminArray, "last", RUBY_METHOD_FUNC(w_array_last), -1);
    rb_define_method(rb_cUsaminArray, "length", RUBY_METHOD_FUNC(w_array_length), 0);
    rb_define_method(rb_cUsaminArray, "size", RUBY_METHOD_FUNC(w_array_length), 0);
    rb_define_method(rb_cUsaminArray, "slice", RUBY_METHOD_FUNC(w_array_slice), -1);
    rb_define_method(rb_cUsaminArray, "to_ary", RUBY_METHOD_FUNC(w_array_eval), 0);
    rb_define_method(rb_cUsaminArray, "marshal_dump", RUBY_METHOD_FUNC(w_value_marshal_dump), 0);
    rb_define_method(rb_cUsaminArray, "marshal_load", RUBY_METHOD_FUNC(w_value_marshal_load), 1);

    rb_eUsaminError = rb_define_class_under(rb_mUsamin, "UsaminError", rb_eStandardError);
    rb_eParserError = rb_define_class_under(rb_mUsamin, "ParserError", rb_eUsaminError);
}
