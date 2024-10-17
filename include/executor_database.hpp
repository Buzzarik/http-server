#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include <optional>
#include "database.hpp"


class ExecutorDataBase{
public:
    ExecutorDataBase(std::shared_ptr<IDataBase> db);
    std::optional<std::string> query_by_number(std::string_view);
    std::optional<std::string> query_by_date_and_cost(std::string_view);
    std::optional<std::string> query_by_product_and_interval(std::string_view);
    bool query_created(std::string_view body, json::object&& date_and_number);
private:
    std::optional<std::unordered_map<std::string, std::string>> parse_query_params(std::string_view url);
    std::optional<std::string> output_result(json::array&& array);
    std::shared_ptr<IDataBase> db_;
};