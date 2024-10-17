#include "../include/session.hpp"
#include <future>
#include <chrono>


std::optional<json::object> longRunningTask() {
    try{
        net::io_context ioc;
        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        std::string host = "app-numbers";
        std::string port = "8081";
        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);
        std::cerr <<  "удалось подключиться к number\n";

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::post, "/numbers", 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.body() = "12345"; //просто типо пароль
        req.prepare_payload();

        // Send the HTTP request to the remote host
        http::write(stream, req);
        std::cerr << "запроса отправлен\n";
        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        boost::beast::error_code er;
        stream.socket().shutdown(tcp::socket::shutdown_both, er);

        std::cerr << "данные с сервера number приняты\n";
        // Вывод статуса ответа
        boost::system::error_code ec;
        std::stringstream ss;
        auto data = boost::beast::buffers_to_string(res.body().data());
        auto value = json::parse(data, ec);
        if (ec || !value.is_object()){
            return {};
        }
        auto ans = value.as_object();
        std::cerr << ans << "\n";
        return {ans};
    }
    catch (const boost::system::system_error& e){
        std::cerr << e.what() << "\n";
        return {};
    }

}


std::optional<json::object> Session::request_by_number(){
  std::future<std::optional<json::object>> taskFuture = std::async(std::launch::async, longRunningTask);
  // Устанавливаем таймаут для задачи
  std::chrono::milliseconds timeout(1000); // 3 секунды

  // Ждем завершения задачи или таймаута
  if (taskFuture.wait_for(timeout) == std::future_status::timeout) {
    return {}; //время истекло
  } 
  else {
    // Получаем и выводим результат задачи
    return taskFuture.get();
    //std::cout << "Результат задачи: " << result << std::endl;
  }
}

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
    auto data = request_by_number();
    std::cerr << "идем дальше по созданию 1\n";
    if (!data.has_value()){
        std::cerr << "сервер number не отвечает\n";
        return server_error("сервер не отвечает");
    }
    std::cerr << "идем дальше по созданию 2\n";
    if (executor_->query_created(body, std::move(data.value()))){
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