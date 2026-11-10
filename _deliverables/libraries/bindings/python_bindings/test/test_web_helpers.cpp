#include "rest_api/forms.h"
#include "rest_api/server.h"
#include "json.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

int main() {
    using namespace ml::rest_api;
    using dataformats::json::Parser;
    const FormSchema form(R"({"fields":{
        "name":{"type":"string","required":true,"max_length":2},
        "count":{"type":"integer","minimum":1},
        "active":{"type":"boolean"}
    }})");
    const auto result = form.validate(
        R"({"name":"\ud83d\ude00\u00e9","count":"2","active":false})");
    assert(result.valid);
    const auto normalized = Parser::parse(result.values_json).as_object();
    assert(normalized.get("count").as_number() == 2);
    assert(!normalized.get("active").as_bool());
    const auto invalid = form.validate("count=0", "application/x-www-form-urlencoded");
    assert(!invalid.valid);
    assert(invalid.errors.at("name").front() == "required");
    assert(invalid.errors.at("count").front() == "minimum");
    assert(Parser::parse(invalid.to_json()).as_object().get("valid").as_bool() == false);
    for (const auto* json : {R"({"x":"\q"})", R"({"x":trueish})", R"({"x":01})", R"({"x":1}tail)"}) {
        bool rejected = false;
        try { Parser::parse(json); } catch (const std::runtime_error&) { rejected = true; }
        assert(rejected);
    }
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path() / ("coolbox-web-" + std::to_string(unique));
    std::filesystem::create_directory(directory);
    {
        std::ofstream file(directory / "index.html");
        file << "<p>{{name}}</p>";
    }
    const auto response = Response::render(directory.string(), "index.html", {{"name", "<Ada>&\"'"}});
    assert(response.body() == "<p>&lt;Ada&gt;&amp;&quot;&#39;</p>");
    assert(response.headers().at("Content-Type") == "text/html; charset=utf-8");
    bool rejected = false;
    try { Response::render(directory.string(), "index.html", {}); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { Response::html_file(directory.string(), (directory / "index.html").string()); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    Request request(HttpMethod::POST, "/", {{"content-type", "application/json"}}, "{}");
    assert(request.get_header("Content-Type") == "application/json");
    std::filesystem::remove(directory / "index.html");
    std::filesystem::remove(directory);
}
