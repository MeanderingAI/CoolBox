#pragma once
#include <string>
#include <map>
#include <vector>

struct Request {
    std::string method;
    std::string uri;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct Response {
    int status_code;
    std::map<std::string, std::string> headers;
    std::string body;
};

