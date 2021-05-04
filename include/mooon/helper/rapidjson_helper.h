// Writed by yijian on 2021/5/3
#ifndef MOOON_HELPER_RAPIDJSON_HELPER_H
#define MOOON_HELPER_RAPIDJSON_HELPER_H
#include <mooon/utils/string_utils.h>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <stdio.h>
#include <memory>
#include <string>
namespace mooon {

// 使用 schema 效验 JSON 格式
// 实践时，可以使用 Linux 自带的工具 xxd 将 schema 记录在专门文件，
// 然后将该文件编译成 cpp 源代码文件，以 cmake 为例：
// # 将 mooon.schema 编译成 cpp 文件
// exec_program(
//     xxd
//     ${CMAKE_CURRENT_SOURCE_DIR}
//     ARGS
//     -i mooon.schema mooon_schema.cpp
//     RETURN_VALUE errcode
// )
// if (errcode)
//     return ()
// endif ()
inline std::shared_ptr<rapidjson::Document>
str2rapidjson(const std::string& json_str, const std::string& schema_str, std::string* errmsg)
{
    std::shared_ptr<rapidjson::Document> doc(new rapidjson::Document);

    if (doc->Parse(json_str.c_str()).HasParseError())
    {
        doc.reset();
        if (errmsg != NULL)
            *errmsg = "parse JSON error, it is not a JSON format";
    }
    else if (!schema_str.empty())
    {
        // 指定了 schema
        rapidjson::Document schema_doc;

        if (schema_doc.Parse(schema_str.c_str()).HasParseError())
        {
            // schema 格式有问题
            doc.reset();
            if (errmsg != NULL)
                *errmsg = "parse SCHEMA error, it is not a JSON format";
        }
        else
        {
            rapidjson::SchemaDocument schema(schema_doc);
            rapidjson::SchemaValidator validator(schema);

            if (!doc->Accept(validator))
            {
                if (errmsg != NULL)
                {
                    // 不满足 schema 规则
                    rapidjson::StringBuffer sb1, sb2;
                    validator.GetInvalidSchemaPointer().StringifyUriFragment(sb1);
                    validator.GetInvalidDocumentPointer().StringifyUriFragment(sb2);
                    *errmsg = mooon::utils::CStringUtils::format_string("validate error (%s, %s, %s)",
                            sb1.GetString(), validator.GetInvalidSchemaKeyword(), sb2.GetString());
                }
                doc.reset();
            }
        }
    }
    return doc;
}

} // namespace mooon {
#endif // MOOON_HELPER_RAPIDJSON_HELPER_H
