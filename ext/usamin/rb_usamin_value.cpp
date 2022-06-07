#include "default_parse_flags.hpp"
#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <ruby.h>
#include "generator.hpp"
#include "usamin_value.hpp"
#include "parser_helper.hpp"

static void usamin_free(void *p) {
    if (!p)
        return;
    UsaminValue **ptr = (UsaminValue **)p;
    if (*ptr)
        delete *ptr;
    ruby_xfree(p);
}

static void usamin_mark(void *p) {
    if (!p)
        return;
    UsaminValue **ptr = (UsaminValue **)p;
    if (*ptr && !((*ptr)->free_flag))
        rb_gc_mark((*ptr)->root_document);
}

static size_t usamin_size(const void *p) {
    if (!p)
        return 0;
    UsaminValue **ptr = (UsaminValue **)p;
    size_t s = 0;
    if (*ptr) {
        s += sizeof(UsaminValue);
        if ((*ptr)->free_flag)
            s += ((RubynizedDocument *)(*ptr)->value)->GetAllocator().Capacity() +
              ((RubynizedDocument *)(*ptr)->value)->GetStackCapacity();
    }
    return s;
}

static const rb_data_type_t rb_usamin_value_type = {
  "UsaminValue", {usamin_mark, usamin_free, usamin_size}, nullptr, nullptr, RUBY_TYPED_FREE_IMMEDIATELY};

VALUE usamin_alloc(const VALUE klass) {
    UsaminValue **ptr;
    VALUE ret = TypedData_Make_Struct(klass, UsaminValue *, &rb_usamin_value_type, ptr);
    *ptr = nullptr;
    return ret;
}

UsaminValue *get_value(VALUE value) {
    UsaminValue **ptr;
    TypedData_Get_Struct(value, UsaminValue *, &rb_usamin_value_type, ptr);
    return *ptr;
}

void set_value(VALUE value, UsaminValue *usamin) {
    UsaminValue **ptr;
    TypedData_Get_Struct(value, UsaminValue *, &rb_usamin_value_type, ptr);
    *ptr = usamin;
}

static bool eql_array(RubynizedValue &, RubynizedValue &);
static bool eql_object(RubynizedValue &, RubynizedValue &);

static bool eql_value(RubynizedValue &self, RubynizedValue &other) {
    if (self.GetType() != other.GetType())
        return false;
    switch (self.GetType()) {
    case rapidjson::kObjectType:
        return eql_object(self, other);
    case rapidjson::kArrayType:
        return eql_array(self, other);
    case rapidjson::kStringType:
        return self == other;
    case rapidjson::kNumberType:
        return self == other && self.IsInt() == other.IsInt() && self.IsUint() == other.IsUint() &&
          self.IsInt64() == other.IsInt64() && self.IsUint64() == other.IsUint64() &&
          self.IsDouble() == other.IsDouble();
    default:
        return true;
    }
}

static bool eql_object(RubynizedValue &self, RubynizedValue &other) {
    if (self.MemberCount() != other.MemberCount())
        return false;
    for (auto &m : self.GetObject()) {
        if (!other.HasMember(m.name))
            return false;
        if (!eql_value(m.value, other[m.name]))
            return false;
    }
    return true;
}

static bool eql_array(RubynizedValue &self, RubynizedValue &other) {
    if (self.Size() != other.Size())
        return false;
    for (rapidjson::SizeType i = 0; i < self.Size(); i++) {
        if (!eql_value(self[i], other[i]))
            return false;
    }
    return true;
}

/*
 * Returns whether the value is equals to the other value.
 *
 * @return [Boolean]
 */
VALUE w_value_equal(const VALUE self, const VALUE other) {
    extern VALUE rb_cUsaminValue;
    if (self == other)
        return Qtrue;
    UsaminValue *value_self = get_value(self);
    check_value(value_self);
    if (rb_obj_is_kind_of(other, rb_cUsaminValue)) {
        UsaminValue *value_other = get_value(other);
        check_value(value_other);
        return value_self->value->operator==(*value_other->value) ? Qtrue : Qfalse;
    } else if (value_self->value->IsArray() && RB_TYPE_P(other, T_ARRAY)) {
        if (value_self->value->Size() != RARRAY_LEN(other))
            return Qfalse;
        return rb_equal(other, eval_array(*value_self->value, value_self->root_document));
    } else if (value_self->value->IsObject() && RB_TYPE_P(other, T_HASH)) {
        if (value_self->value->MemberCount() != RHASH_SIZE(other))
            return Qfalse;
        return rb_equal(other, eval_object(*value_self->value, value_self->root_document, 0));
    }
    return Qfalse;
}

/*
 * Returns whether the value is array.
 */
VALUE w_value_isarray(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return value->value->IsArray() ? Qtrue : Qfalse;
}

/*
 * Returns whether the value is object.
 */
VALUE w_value_isobject(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return value->value->IsObject() ? Qtrue : Qfalse;
}

/*
 * Convert to Ruby data structures.
 *
 * @overload eval(opts = {})
 *   @option opts :symbolize_names symbolize keys of all Hash objects
 *   @return [Object]
 */
VALUE w_value_eval(const int argc, const VALUE *argv, const VALUE self) {
    extern VALUE sym_symbolize_names;
    VALUE options;
    rb_scan_args(argc, argv, ":", &options);
    UsaminValue *value = get_value(self);
    check_value(value);
    if (value->value->IsObject()) {
        return eval_object(*(value->value), value->root_document, test_option(options, sym_symbolize_names));
    } else if (value->value->IsArray()) {
        return eval_array(*(value->value), value->root_document);
    } else {
        return Qnil;
    }
}

/*
 * Convert to Ruby data structures recursively.
 *
 * @overload eval_r(opts = {})
 *   @option opts :symbolize_names symbolize keys of all Hash objects
 *   @return [Object]
 */
VALUE w_value_eval_r(const int argc, const VALUE *argv, const VALUE self) {
    extern VALUE sym_symbolize_names;
    VALUE options;
    rb_scan_args(argc, argv, ":", &options);
    UsaminValue *value = get_value(self);
    check_value(value);
    return eval_r(*(value->value), test_option(options, sym_symbolize_names));
}

/*
 * @return [Boolean]
 */
VALUE w_value_eql(const VALUE self, const VALUE other) {
    extern VALUE rb_cUsaminValue;
    if (self == other)
        return Qtrue;
    if (!rb_obj_is_kind_of(other, rb_cUsaminValue))
        return Qfalse;
    UsaminValue *value_self = get_value(self);
    UsaminValue *value_other = get_value(other);
    check_value(value_self);
    check_value(value_other);
    return eql_value(*value_self->value, *value_other->value) ? Qtrue : Qfalse;
}

/*
 * Always true.
 */
VALUE w_value_isfrozen(const VALUE) {
    return Qtrue;
}

static st_index_t hash_object(RubynizedValue &value);
static st_index_t hash_array(RubynizedValue &value);

static st_index_t hash_value(RubynizedValue &value) {
    auto type = value.GetType();
    st_index_t h = rb_hash_start(type);
    rb_hash_uint(h, reinterpret_cast<st_index_t>(hash_value));
    switch (type) {
    case rapidjson::kNullType:
        h = rb_hash_uint(h, NUM2LONG(rb_hash(Qnil)));
        break;
    case rapidjson::kFalseType:
        h = rb_hash_uint(h, NUM2LONG(rb_hash(Qfalse)));
        break;
    case rapidjson::kTrueType:
        h = rb_hash_uint(h, NUM2LONG(rb_hash(Qtrue)));
        break;
    case rapidjson::kObjectType:
        h = rb_hash_uint(h, hash_object(value));
        break;
    case rapidjson::kArrayType:
        h = rb_hash_uint(h, hash_array(value));
        break;
    case rapidjson::kStringType:
        h = rb_hash_uint(h, rb_str_hash(eval_str(value)));
        break;
    case rapidjson::kNumberType:
        if (value.IsInt())
            h = rb_hash_uint(h, 0x770);
        if (value.IsUint())
            h = rb_hash_uint(h, 0x771);
        if (value.IsInt64())
            h = rb_hash_uint(h, 0x772);
        if (value.IsUint64())
            h = rb_hash_uint(h, 0x773);
        if (value.IsDouble())
            h = rb_hash_uint(h, 0x774);
        h = rb_hash_uint(h, NUM2LONG(rb_hash(eval_num(value))));
    }
    return rb_hash_end(h);
}

static st_index_t hash_object(RubynizedValue &value) {
    st_index_t h = rb_hash_start(value.MemberCount());
    h = rb_hash_uint(h, reinterpret_cast<st_index_t>(hash_object));
    for (auto &m : value.GetObject()) {
        h = rb_hash_uint(h, rb_str_hash(eval_str(m.name)));
        h = rb_hash_uint(h, hash_value(m.value));
    }
    return rb_hash_end(h);
}

static st_index_t hash_array(RubynizedValue &value) {
    st_index_t h = rb_hash_start(value.Size());
    h = rb_hash_uint(h, reinterpret_cast<st_index_t>(hash_array));
    for (auto &v : value.GetArray())
        h = rb_hash_uint(h, hash_value(v));
    return rb_hash_end(h);
}

/*
 * @return [Integer]
 */
VALUE w_value_hash(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return ST2FIX(hash_value(*value->value));
}

/*
 * Returns root object.
 *
 * @return [UsaminValue]
 */
VALUE w_value_root(const VALUE self) {
    UsaminValue *value = get_value(self);
    check_value(value);
    return value->root_document;
}

/*
 * Dumps data in JSON.
 *
 * @return [String]
 */
VALUE w_value_marshal_dump(const VALUE self) {
    extern VALUE rb_mUsamin;
    return w_generate(rb_mUsamin, self);
}

/*
 * Loads marshal data.
 */
VALUE w_value_marshal_load(const VALUE self, const VALUE source) {
    extern VALUE rb_cUsaminHash, rb_cUsaminArray, rb_eUsaminError, rb_eParserError;
    Check_Type(source, T_STRING);
    RubynizedDocument *doc = new RubynizedDocument();
    rapidjson::ParseResult result =
      doc->Parse<RAPIDJSON_PARSE_FLAGS_FOR_MARSHAL>(RSTRING_PTR(source), RSTRING_LEN(source));
    if (!result) {
        delete doc;
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    }
    if ((rb_obj_is_instance_of(self, rb_cUsaminHash) && doc->IsObject()) ||
        (rb_obj_is_instance_of(self, rb_cUsaminArray) && doc->IsArray())) {
        UsaminValue *value = new UsaminValue(doc, true);
        set_value(self, value);
        value->root_document = self;
    } else {
        auto type = doc->GetType();
        delete doc;
        rb_raise(rb_eUsaminError, "type mismatch in marshal load: %d", type);
    }
    return Qnil;
}
