#include "../include/session.hpp"
#include <iostream>
#include <chrono>
#include <boost/json/src.hpp>

namespace chrono = std::chrono;


//что-то с сервером, надо заглушки поставить, для просмотра!!!

void fail(beast::error_code ec, char const* what){
	std::cerr << what << ": " << ec.message() << "\n";
}

Session::Session(tcp::socket&& socket, std::stack<size_t>& numbers)
	: stream_(std::move(socket))
	, numbers_(numbers) {}

void Session::run(){
	net::dispatch(stream_.get_executor(),
		beast::bind_front_handler(&Session::do_read, shared_from_this()));
}

void Session::do_read(){
	req_ = {};
	stream_.expires_after(std::chrono::seconds(30));
	http::async_read(stream_, buffer_, req_,
		beast::bind_front_handler(&Session::on_read, shared_from_this()));
}

void Session::on_read(beast::error_code ec, size_t bytes_transferred){
	boost::ignore_unused(bytes_transferred);
	if(ec == http::error::end_of_stream)
		return do_close();
	if(ec)
		return fail(ec, "read");
	send_response(handle_request(std::move(req_)));
}

void Session::send_response(http::message_generator&& msg){
	bool keep_alive = msg.keep_alive();
	
	beast::async_write(stream_,
		std::move(msg),
		beast::bind_front_handler(&Session::on_write, shared_from_this(), keep_alive));
}

void Session::on_write(bool keep_alive, beast::error_code ec, size_t bytes_transferred){
	boost::ignore_unused(bytes_transferred);
	if(ec)
		return fail(ec, "write");
	if(!keep_alive){
		return do_close();
	}
	do_read();
}

void Session::do_close(){
	beast::error_code ec;
	stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}


//сделать получения number в виде хеша по времени + 
template <class Body, class Allocator>
http::message_generator Session::handle_request(http::request<Body, http::basic_fields<Allocator>>&& req){
    //проверка методов
    std::string_view url = req.target();
    size_t pos = url.find('?');
    std::string_view path = url.substr(0, pos);
    std::string_view params = url.substr(pos + 1);
    //будут одна ручка и проверка на нее post запросом и в теле пароль
    std::cerr << "вы попали в обработчик\n";
    if (req.method() == http::verb::post && path == "/numbers"){
        std::string_view body = req.body();
        if (body != "12345"){ //типа пароль
            return bad_request("not auth");
        }

        std::cerr << body << "\n";
        
        boost::json::object obj;
        auto now = std::chrono::system_clock::now();
        // Преобразование в время в секундах
        auto time_since_epoch = std::chrono::system_clock::to_time_t(now);
        // Преобразование в структуру tm
        std::tm* tm = std::localtime(&time_since_epoch);
        // Формирование строки в формате YYYY-MM-DD
        std::stringstream ss, ans;
        ss << (tm->tm_year + 1900) << '-'
            << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1) << '-'
            << std::setw(2) << std::setfill('0') << tm->tm_mday;
        //создаем ответ
        obj["date"] = ss.str();

        //number - время в миллисекундах
        auto now_number = std::chrono::high_resolution_clock::now();
         // Преобразование в миллисекунды
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now_number.time_since_epoch());
        auto time = std::to_string(milliseconds.count());
        time = time.substr(time.size() - 6, 5);
        std::cerr << "mili = " << time << "\n";
        obj["number"] = time;

        ans << obj;
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false); //пока false
        res.body() = ans.str();
        res.prepare_payload();
        return res;
    }
    return not_found("нет такого");
}

http::message_generator Session::bad_request(std::string_view message){
    http::response<http::string_body> res{http::status::bad_request, 11};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(false); //пока false
    res.body() = std::string(message);
    res.prepare_payload();
    return res;
}

http::message_generator Session::not_found(std::string_view message){
    http::response<http::string_body> res{http::status::not_found, 11};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(false);
    res.body() = "The resource '" + std::string(message) + "' was not found.";
    res.prepare_payload();
    return res;
}