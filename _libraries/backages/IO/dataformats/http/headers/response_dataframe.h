#ifndef ML_DATAFORMATS_HTTP_RESPONSE_DATAFRAME_H
#define ML_DATAFORMATS_HTTP_RESPONSE_DATAFRAME_H

#include <string>
#include <vector>
#include <map>

#include "dataformats/json/json.h"
namespace dj = dataformats::json;

namespace dataformats {
namespace http {

class ResponseDataFrame {
public:
    std::vector<std::string> columns;
    std::vector<std::vector<dj::Value>> rows;
    std::map<std::string, std::string> metadata;

    ResponseDataFrame() = default;
    ResponseDataFrame(const std::vector<std::string>& cols) : columns(cols) {}

    void add_row(const std::vector<dj::Value>& row) {
        rows.push_back(row);
    }

    void set_metadata(const std::string& key, const std::string& value) {
        metadata[key] = value;
    }

    dj::Object to_json() const {
        dj::Object obj;
        obj.set("columns", dj::Array(columns.begin(), columns.end()));
        dj::Array rows_json;
        for (const auto& row : rows) {
            dj::Array row_json;
            for (const auto& val : row) row_json.push(val);
            rows_json.push(row_json);
        }
        obj.set("rows", rows_json);
        dj::Object meta_obj;
        for (const auto& kv : metadata) meta_obj.set(kv.first, kv.second);
        obj.set("metadata", meta_obj);
        return obj;
    }
};

} // namespace http
} // namespace dataformats

#endif // ML_DATAFORMATS_HTTP_RESPONSE_DATAFRAME_H
