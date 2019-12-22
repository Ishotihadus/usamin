#define RAPIDJSON_PARSE_DEFAULT_FLAGS                                                                      \
    (rapidjson::kParseIterativeFlag | rapidjson::kParseFullPrecisionFlag | rapidjson::kParseCommentsFlag | \
     rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag)
#define RAPIDJSON_PARSE_FAST_FLAGS \
    (RAPIDJSON_PARSE_DEFAULT_FLAGS & ~rapidjson::kParseFullPrecisionFlag & ~rapidjson::kParseIterativeFlag)
#define RAPIDJSON_PARSE_FLAGS_FOR_MARSHAL \
    (rapidjson::kParseIterativeFlag | rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag)
