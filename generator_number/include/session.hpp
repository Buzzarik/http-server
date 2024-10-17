#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <memory>
#include <stack>


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

void fail(beast::error_code ec, char const* what);

class Session : public std::enable_shared_from_this<Session>{
public:
	Session(tcp::socket&& socket, std::stack<size_t>& numbers);
	void run();
private:
	void do_read();
	void on_read(beast::error_code ec, size_t bytes_transferred);
	void send_response(http::message_generator&& res);
	void on_write(bool keep_alive, beast::error_code ec, size_t bytes_transferred);
	void do_close();
	
	template <class Body, class Allocator>
	http::message_generator handle_request(http::request<Body, http::basic_fields<Allocator>>&& req);
	http::message_generator bad_request(std::string_view message);
	http::message_generator not_found(std::string_view message);

	beast::tcp_stream stream_;
	beast::flat_buffer buffer_;
	http::request<http::string_body> req_;
    std::stack<size_t>& numbers_;
};