#pragma once
#include <string>



enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE_,
    PATCH,
    OPTIONS,
    HEAD,
    UNKNOWN
};

inline std::string to_string(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE_: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::HEAD: return "HEAD";
        default: return "UNKNOWN";
    }
}

inline HttpMethod from_string(const std::string& method) {
    if (method == "GET") return HttpMethod::GET;
    if (method == "POST") return HttpMethod::POST;
    if (method == "PUT") return HttpMethod::PUT;
    if (method == "DELETE") return HttpMethod::DELETE_;
    if (method == "PATCH") return HttpMethod::PATCH;
    if (method == "OPTIONS") return HttpMethod::OPTIONS;
    if (method == "HEAD") return HttpMethod::HEAD;
    return HttpMethod::UNKNOWN;
}

