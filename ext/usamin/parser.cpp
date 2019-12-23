#include "default_parse_flags.hpp"
#include "parser.hpp"
#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>
#include <ruby.h>
#include "rb_common.hpp"
#include "rubynized_rapidjson.hpp"
#include "usamin_value.hpp"
#include "parser_helper.hpp"

static inline rapidjson::ParseResult parse(RubynizedDocument &doc, const VALUE str, bool fast = false) {
    volatile VALUE v = get_utf8_str(str);
    return fast ? doc.Parse<RAPIDJSON_PARSE_FAST_FLAGS>(RSTRING_PTR(v), RSTRING_LEN(v))
                : doc.Parse(RSTRING_PTR(v), RSTRING_LEN(v));
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
VALUE w_load(const int argc, const VALUE *argv, const VALUE) {
    extern VALUE rb_eUsaminError, rb_eParserError, sym_fast;

    VALUE source, options;
    rb_scan_args(argc, argv, "1:", &source, &options);
    RubynizedDocument *doc = new RubynizedDocument;
    auto result = parse(*doc, source, test_option(options, sym_fast));
    if (!result) {
        delete doc;
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    }

    VALUE ret;
    switch (doc->GetType()) {
    case rapidjson::kNullType:
        delete doc;
        return Qnil;
    case rapidjson::kFalseType:
        delete doc;
        return Qfalse;
    case rapidjson::kTrueType:
        delete doc;
        return Qtrue;
    case rapidjson::kObjectType:
        return make_hash(new UsaminValue(doc, true), true);
    case rapidjson::kArrayType:
        return make_array(new UsaminValue(doc, true), true);
    case rapidjson::kStringType:
        ret = eval_str(*doc);
        delete doc;
        return ret;
    case rapidjson::kNumberType:
        ret = eval_num(*doc);
        delete doc;
        return ret;
    default:
        rb_raise(rb_eUsaminError, "unknown value type: %d", doc->GetType());
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
 *   @option opts :symbolize_names symbolize keys of all Hash objects
 *   @return [Object]
 */
VALUE w_parse(const int argc, const VALUE *argv, const VALUE) {
    extern VALUE rb_eParserError, sym_fast, sym_symbolize_names;

    VALUE source, options;
    rb_scan_args(argc, argv, "1:", &source, &options);
    RubynizedDocument doc;
    auto result = parse(doc, source, test_option(options, sym_fast));
    if (!result)
        rb_raise(rb_eParserError, "%s Offset: %lu", GetParseError_En(result.Code()), result.Offset());
    return eval_r(doc, test_option(options, sym_symbolize_names));
}
