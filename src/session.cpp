#include "../include/session.hpp"


void fail(beast::error_code ec, char const* what){
	std::cerr << what << ": " << ec.message() << "\n";
}

Session::Session(tcp::socket&& socket, std::shared_ptr<IDataBase> db)
	: stream_(std::move(socket))
	, executor_(new ExecutorDataBase(db)) {}

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

template <class Body, class Allocator>
http::message_generator Session::handle_request(http::request<Body, http::basic_fields<Allocator>>&& req){
    //проверка методов
    std::string_view url = req.target();
    size_t pos = url.find('?');
    std::string_view path = url.substr(0, pos);
    std::string_view params = url.substr(pos + 1);
    switch (req.method())
    {
    case http::verb::get :
        //проверяем вхождение ручек
        if (!Endpoints::contains_enpoint(http::verb::get, std::string(path))){
            std::cerr << "нет такой ручки GET - " << url << "\n";
            return not_found("нет такого запроса");
        }
        return get_request(url);
        break;
    case http::verb::post :
        //проверяем вхождение ручек
        if (!Endpoints::contains_enpoint(http::verb::post, std::string(path))){
            std::cerr << "нет такой ручки GET - " << url << "\n";
            return not_found("нет такого запроса");
        }
        return post_request(req.body());
        break;
    default:
        return not_found("нет такого запроса");
        break;
    }
}

http::message_generator Session::get_request(std::string_view url){
    auto q1 = executor_->query_by_date_and_cost(url);
    if (q1.has_value()){
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false); //пока false
        res.body() = std::string(*q1);
        res.prepare_payload();
        return res;
    }
    auto q2 = executor_->query_by_number(url);
    if (q2.has_value()){
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false); //пока false
        res.body() = std::string(*q2);
        res.prepare_payload();
        return res;
    }
    auto q3 = executor_->query_by_product_and_interval(url);
    if (q3.has_value()){
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false); //пока false
        res.body() = std::string(*q3);
        res.prepare_payload();
        return res;
    }
    return bad_request("неверные параметры");
}

http::message_generator Session::post_request(std::string_view body){
    if (executor_->query_created(body)){
        http::response<http::string_body> res{http::status::created, 11};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false); //пока false
        res.body() = "";
        res.prepare_payload();
        return res;
    }
    return bad_request("не удалось записать данные");
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

http::message_generator Session::server_error(std::string_view message){
    http::response<http::string_body> res{http::status::internal_server_error, 11};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(false);
    res.body() = "An error occurred: '" + std::string(message) + "'";
    res.prepare_payload();
    return res;
}