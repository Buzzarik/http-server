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

int main(int argc, char* argv[]){
try{

auto const threads = std::max<int>(1, 1);

net::io_context ioc{threads};

std::cerr << "fghjk\n";
std::make_shared<Listener>(ioc, tcp::endpoint(net::ip::address::from_string("0.0.0.0"), 8081))->run();
std::cerr << "1234567\n";
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