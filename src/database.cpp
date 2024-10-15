#include "../include/database.hpp"
#include <boost/json/src.hpp>
#include <iostream>

PostgresDataBase::PostgresDataBase(std::string_view config)
    : conn(PQconnectdb(config.data()), [](PGconn* c){
        PQfinish(c);
    })
{}

std::optional<json::array>PostgresDataBase::sql_query(std::string_view sql){
    if (!is_open()){
        std::cerr << "Server DataBase fault\n";
        return {};
    }
    
    std::unique_ptr<PGresult, void(*)(PGresult*)> result(PQexec(conn.get(), sql.data()), [](PGresult* c){
        PQclear(c);
    });
    
    if (PQresultStatus(result.get()) != PGRES_COMMAND_OK && PQresultStatus(result.get()) != PGRES_TUPLES_OK){
        std::cerr << PQerrorMessage(conn.get()) << "\n";
        return {};
    }
    json::array array;
    if (PQresultStatus(result.get()) == PGRES_COMMAND_OK){
        return {array};
    }
    int rows = PQntuples(result.get());
    int cols = PQnfields(result.get());

    for (int i = 0; i < rows; i++){
        json::object order;
        for (int j = 0; j < cols; j++){
            std::string name = PQfname(result.get(), j);
            order[name] = std::string(PQgetvalue(result.get(), i, j));
        }
        array.push_back(order);
    }
    // std::cerr << array << "\n";
    return {array};
}

bool PostgresDataBase::is_open(){
    return conn && PQstatus(conn.get()) == CONNECTION_OK;
}

IDataBase::~IDataBase() {}