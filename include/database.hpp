#pragma once
#include <boost/json.hpp>
#include <string_view>
#include <libpq-fe.h>
#include <memory>
#include <optional>


namespace json = boost::json;

class IDataBase{
public:
    virtual std::optional<json::array> sql_query(std::string_view) = 0;
    virtual bool is_open() = 0;
    virtual ~IDataBase();
};

class PostgresDataBase : public IDataBase{
public:
	PostgresDataBase(std::string_view);
	std::optional<json::array> sql_query(std::string_view) override;
	bool is_open() override;
private:
	std::unique_ptr<PGconn, void(*)(PGconn*)> conn;
};

