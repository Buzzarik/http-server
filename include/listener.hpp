#pragma once
#include "session.hpp"
#include "database.hpp"

class Listener : public std::enable_shared_from_this<Listener>
{
public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<IDataBase> db);
    void run();

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<IDataBase> db_;
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
};