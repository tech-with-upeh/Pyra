#pragma once

// #include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <iostream>
#include <string>

namespace test_boost {

inline void run_sample_server()
{
    std::cout << "Boost Asio/Beast headers found!" << std::endl;

    // Minimal Asio I/O context just to test compilation
    boost::asio::io_context ioc;
    std::cout << "Boost Asio io_context created." << std::endl;
}




} // namespace test_boost
