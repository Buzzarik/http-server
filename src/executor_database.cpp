#include "../include/executor_database.hpp"
#include <iostream>
#include <format>
#include <sstream>
#include <string>
#include <memory>
#include <boost/algorithm/string.hpp>

ExecutorDataBase::ExecutorDataBase(std::shared_ptr<IDataBase> db)
    : db_(db){}

std::optional<std::string> ExecutorDataBase::query_by_number(std::string_view url){
    auto params = parse_query_params(url);
    if (!params.has_value()){
        std::cerr << "некорректный запрос\n";
        return {};
    }
    //провреям все необходимые данные
    const auto& query = *params;
    if (!(query.size() == 1 && query.contains("number"))){
        std::cerr << "некорректный запрос. нужен только number\n";
        return {};
    }
        //id | number | cost | date | receiver | address | type_pay | type_delivered | article | name | count | price
    std::string sql = std::format("SELECT a.id, \"number\", cost, date, receiver, address, type_pay, type_delivered, article, name, count, price \
FROM \"Orders\" as a INNER JOIN \"DetailOrder\" as b \
ON a.id = b.id_order \
WHERE \"number\" = '{}' \
ORDER BY a.id ASC", 
query.at("number"));
    auto res = db_->sql_query(sql);
    //проверяем ответ
    if (!res.has_value()){
        std::cerr << "ошибка SQL запросе выборка 1\n";
        return {};
    }
    return output_result(std::move(*res));
}

std::optional<std::string> ExecutorDataBase::query_by_date_and_cost(std::string_view url){
    auto params = parse_query_params(url);
    if (!params.has_value()){
        std::cerr << "некорректный запрос\n";
        return {};
    }
    const auto& query = *params;
    //провреям все необходимые данные
    if (!(query.size() == 2 && 
        query.contains("date") && 
        query.contains("cost"))){
        std::cerr << "некорректный запрос. нужен только number\n";
        return {};
    }
        std::string sql = std::format("SELECT a.id, \"number\", cost, date, receiver, address, type_pay, type_delivered, article, name, count, price \
FROM \"Orders\" as a INNER JOIN \"DetailOrder\" as b \
ON a.id = b.id_order \
WHERE date = '{}' AND cost > {} \
ORDER BY a.id ASC", 
query.at("date"), query.at("cost"));
    auto res = db_->sql_query(sql);
    //проверяем ответ
    if (!res.has_value()){
        std::cerr << "ошибка SQL запросе выборка 2\n";
        return {};
    }
    return output_result(std::move(*res));
}

std::optional<std::string> ExecutorDataBase::query_by_product_and_interval(std::string_view url){
    auto params = parse_query_params(url);
    if (!params.has_value()){
        std::cerr << "некорректный запрос\n";
        return {};
    }
    const auto& query = *params;
    //провреям все необходимые данные
    if (!(query.size() == 3 && 
        query.contains("to") && 
        query.contains("from") &&
        query.contains("name"))){
        std::cerr << "некорректный запрос. нужен только number\n";
        return {};
    }
    std::string sql = std::format("SELECT a.id, \"number\", cost, date, receiver, address, type_pay, type_delivered, article, name, count, price \
FROM \"Orders\" as a INNER JOIN \"DetailOrder\" as b \
ON a.id = b.id_order \
WHERE date BETWEEN '{}' AND '{}' AND a.id NOT IN ( \
SELECT id_order FROM \"DetailOrder\" WHERE name = '{}') \
ORDER BY a.id ASC", 
query.at("from"), query.at("to"), query.at("name"));
    auto res = db_->sql_query(sql);
    //проверяем ответ
    if (!res.has_value()){
        std::cerr << "ошибка SQL запросе выборка 3\n";
        return {};
    }
    return output_result(std::move(*res));
}

bool ExecutorDataBase::query_created(std::string_view body){
    boost::system::error_code ec;
    auto value = boost::json::parse(body, ec);
    if (ec || !value.is_object()){
        std::cerr << "некорректный json формат\n";
    }
    auto order = value.as_object();

    if (!(order.contains("number") && order["number"].is_string() &&
        order.contains("date") && order["date"].is_string() &&
        order.contains("receiver") && order["receiver"].is_string() &&
        order.contains("address") && order["address"].is_string() &&
        order.contains("type_pay") && order["type_pay"].is_string() &&
        order.contains("type_delivered") && order["type_delivered"].is_string() &&
        order.contains("detail_orders") && order["detail_orders"].is_array())){

        std::cerr << "неверные параметры json\n";
        return false;
    }
    size_t cost = 0;
    std::string query2;
    for (const auto& d : order["detail_orders"].as_array()){
        if (!d.is_object()){
            std::cerr << "неверные параметры json\n";
            return false;
        }
        boost::json::object detail = d.as_object();
        if (!(detail.contains("article") && detail["article"].is_string() &&
            detail.contains("name") && detail["name"].is_string() &&
            detail.contains("count") && detail["count"].is_int64() && detail["count"].as_int64() > 0 &&
            detail.contains("price") && detail["price"].is_int64() && detail["price"].as_int64() > 0)){
            std::cerr << "неверные детали заказа\n";
            return false;
        }
        size_t price = detail["price"].as_int64();
        size_t count = detail["count"].as_int64();
        cost += price * count; 
        query2 += std::format("INSERT INTO \"DetailOrder\"( \
article, name, count, price, id_order) \
VALUES ('qqq', '{}', {}, {}, identifier);\n",
std::string(detail["name"].as_string()), count, price);
    }
    std::string query = std::format("BEGIN; \
do \
$$ \
declare \
 identifier int; \
begin \
WITH inserted AS( \
INSERT INTO public.\"Orders\"( \
\"number\", cost, date, receiver, address, type_pay, type_delivered) \
VALUES ('{}', {}, CURRENT_DATE, '{}', '{}', '{}', '{}') \
RETURNING id) \
SELECT id INTO identifier FROM inserted;\n", std::string(order["number"].as_string()), cost, 
                //std::string(order["date"].as_string()), пока ее нет
                std::string(order["receiver"].as_string()),
                std::string(order["address"].as_string()), 
                std::string(order["type_pay"].as_string()), 
                std::string(order["type_delivered"].as_string()));
    query += query2 + "end; \
$$; \
COMMIT;";
    std::cerr << "check created " << query << "\n"; 
    auto res = db_->sql_query(query);

    if (!res.has_value()){
        std::cerr << "ошибка SQL запросе вставка\n";
        return {};
    }
    
    return true;
}

std::optional<std::unordered_map<std::string, std::string>> ExecutorDataBase::parse_query_params(std::string_view url){
    std::unordered_map<std::string, std::string> keys_values;
    size_t pos = url.find('?');
    url = url.substr(pos + 1);
    if (url.empty()){
        return {};
    }
    std::vector<std::string_view> params;
    boost::algorithm::split(params, url, boost::algorithm::is_any_of("&"));
    for (auto param : params){
        size_t pos_equal = param.find('=');
        if (pos_equal == 0 || pos_equal == param.size() - 1 || pos_equal == std::string_view::npos){
            std::cerr << "некорректные query параметры в url";
            return {};
        }
        std::string key = std::string(param.substr(0, pos_equal));
        std::string value = std::string(param.substr(pos_equal + 1));
        keys_values[key] = value;
    }
    return {keys_values};
}

std::optional<std::string> ExecutorDataBase::output_result(json::array&& array){
    //парсим json::array
    json::array detail_orders;
    json::object order;
    json::array output;
    // size_t size = array.size();
    std::string id = "-1";
    bool flag = false;
    for (const auto& element : array){
        if (!element.is_object()){
            std::cerr << "получены некоррекнтые данные";
            return {};
        }
        json::object obj = element.as_object();
        // std::cerr << std::string(obj["id"].as_string()) << "\n";
        // id = std::string(obj["id"].as_string());
        // std::cerr << id << "\n";
//id | number | cost | date | receiver | address |
// type_pay | type_delivered | article | name | count | price
        json::object detail_order;
        detail_order["article"] = obj["article"];
        detail_order["name"] = obj["name"];
        detail_order["count"] = std::stoull(obj["count"].as_string().c_str());
        detail_order["price"] = std::stoull(obj["price"].as_string().c_str());

        if (!flag){ //первые значения заказа
            order["number"] = obj["number"];
            order["cost"] = std::stoull(obj["cost"].as_string().c_str());
            order["date"] = obj["date"];
            order["receiver"] = obj["receiver"];
            order["address"] = obj["address"];
            order["type_pay"] = obj["type_pay"];
            order["type_delivered"] = obj["type_delivered"];
            flag = true;
        }

        if (std::string(obj["id"].as_string()) == id){
            //если равны, то надо дособирать detail_order
            detail_orders.push_back(std::move(detail_order));
            std::cerr << detail_orders << "\n";
        }
        else{
            //если это уже другой заказ
            if (id != "-1"){
                //собираем предыдущий
                std::cerr << detail_orders << "\n\n";
                order["detail"] = std::move(detail_orders);
                output.push_back(std::move(order));
                // order.clear();
                order["number"] = obj["number"];
                order["cost"] = std::stoull(obj["cost"].as_string().c_str());
                order["date"] = obj["date"];
                order["receiver"] = obj["receiver"];
                order["address"] = obj["address"];
                order["type_pay"] = obj["type_pay"];
                order["type_delivered"] = obj["type_delivered"];
                // detail_orders.clear();
            }
            //надо запомнить новый id
            id = std::string(obj["id"].as_string());
            detail_orders.push_back(std::move(detail_order));
        }
    }
    //всегда остается последний заказ, надо просто проверить его состояние на пустоту
    if (!order.empty()){
        order["detail"] = detail_orders;
        output.push_back(order);
    }
    std::stringstream oss;
    oss << output;
    return {oss.str()};
}