#include "../include/listener.hpp"
#include "../include/database.hpp"

Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<IDataBase> db)
	: ioc_(ioc)
	, acceptor_(net::make_strand(ioc))
	, db_(db)
{
	beast::error_code ec;
	acceptor_.open(endpoint.protocol(), ec);
	if(ec){
		fail(ec, "open");
		return;
	}
	acceptor_.set_option(net::socket_base::reuse_address(true), ec);
	if(ec){
		fail(ec, "set_option");
		return;
	}
	acceptor_.bind(endpoint, ec);
	if(ec)
	{
		fail(ec, "bind");
		return;
	}
	acceptor_.listen(
	net::socket_base::max_listen_connections, ec);
	if(ec)
	{
		fail(ec, "listen");
		return;
	}
}
	
void Listener::run(){
	do_accept();
}

void Listener::do_accept(){
	acceptor_.async_accept(
		net::make_strand(ioc_),
		beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(beast::error_code ec, tcp::socket socket)
{
	if(ec)
	{
		fail(ec, "accept");
		return; // To avoid infinite loop
	}
	else{
	std::make_shared<Session>(std::move(socket), db_)->run();
	}
	do_accept();
}