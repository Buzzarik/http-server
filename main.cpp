#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "include/listener.hpp"
#include "include/database.hpp"

int main(int argc, char* argv[]){

auto db = std::make_shared<PostgresDataBase>("host=app-postgres port=5432 dbname=postgres user=user password=1065");

std::cerr << "wfwrf";

try{
if (!db->is_open()){
    std::cerr << "Нет бд\n";
    return 0;
}

std::cerr << "wfwrf";
auto const threads = std::max<int>(1, 1);

net::io_context ioc{threads};

std::make_shared<Listener>(ioc, tcp::endpoint(net::ip::address::from_string("0.0.0.0"), 8080), db)->run();

std::cerr << "wfwrf";
std::vector<std::thread> v;
v.reserve(threads - 1);
for(auto i = threads - 1; i > 0; --i)
	v.emplace_back([&ioc]{
		ioc.run();
	});
ioc.run();
}
catch(const std::exception& e){
	std::cerr << e.what();
}
return EXIT_SUCCESS;
}