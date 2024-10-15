#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <boost/beast.hpp>


namespace beast = boost::beast;
namespace http = beast::http;

class Endpoints{
    public:
    static bool contains_enpoint(http::verb v, const std::string& endpoint){
        return endpoints.contains(v) && endpoints.at(v).contains(endpoint);
    }
    static bool contains_type_body(const std::string& type){
        return mime_types.contains(type);
    }
    static inline const std::unordered_map<http::verb, std::unordered_set<std::string>> endpoints = {
        {
            http::verb::get,
            {
                "/order"
            }
        },
        {
            http::verb::post,
            {
                "/order"
            }
        }
    };
    static inline const std::unordered_set<std::string> mime_types = {
        "application/json"
    };
};