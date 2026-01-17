#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>

#include "websocketserver.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

extern const std::string WEB_ROOT;

// mime types
std::string mime_type(const std::string& path);

// helpers
bool read_file(const std::string& path, std::string& out);

// ======================= http session =======================

class http_session : public std::enable_shared_from_this<http_session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

public:
    explicit http_session(tcp::socket socket);

    void run();

private:
    void handle();

    void send_method_not_allowed();
    void send_not_found();
};
