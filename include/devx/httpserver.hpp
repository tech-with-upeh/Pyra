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

static const std::string WEB_ROOT = "./web";

// NEW: mime types
inline std::string mime_type(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".css"))  return "text/css";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".jpg"))  return "image/jpeg";
    if (path.ends_with(".svg"))  return "image/svg+xml";
    if (path.ends_with(".wasm")) return "application/wasm";
    return "application/octet-stream";
}



/* ======================= helpers ======================= */

inline bool read_file(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    out.assign(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    return true;
}

/* ======================= http session ======================= */

class http_session : public std::enable_shared_from_this<http_session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

public:
    explicit http_session(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void run() {
        http::async_read(socket_, buffer_, req_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (!ec) self->handle();
            });
    }
private:
    void handle() {
        // ---- WebSocket upgrade ----
        if (websocket::is_upgrade(req_)) {
            std::make_shared<websocket_session>(
                std::move(socket_))->run(std::move(req_));
            return;
        }

        // ---- Allowed methods ----
        auto method = req_.method();
        if (method != http::verb::get &&
            method != http::verb::head &&
            method != http::verb::options)
        {
            send_method_not_allowed();
            return;
        }

        // ---- OPTIONS (CORS / Dev) ----
        if (method == http::verb::options) {
            http::response<http::empty_body> res{
                http::status::ok, req_.version()
            };
            res.set(http::field::allow, "GET, HEAD, OPTIONS");
            res.prepare_payload();
            http::async_write(socket_, res,
                [self = shared_from_this()](beast::error_code, std::size_t) {
                    self->socket_.shutdown(tcp::socket::shutdown_send);
                });
            return;
        }

        // ---- GET or HEAD ----
        std::string target = std::string(req_.target());
        if (target.empty())
            target = "/";

        // prevent directory traversal
        if (target.find("..") != std::string::npos) {
            send_not_found();
            return;
        }


        // normalize
        if (target == "/")
            target = "/index.html";

        std::string full_path = WEB_ROOT + target;
        std::string body;
        std::cout << http::to_string(req_.method()) << " -> " << target << std::endl;
        bool file_exists = read_file(full_path, body);
        if (!file_exists) {
            full_path = WEB_ROOT + "/index.html";
            if (!read_file(full_path, body)) {
                send_not_found();
                return;
            }
        }

        http::response<http::string_body> res{
            http::status::ok, req_.version()
        };
        res.set(http::field::content_type, mime_type(full_path));

        // HEAD request → empty body
        if (method == http::verb::head) {
            res.set("Helios-Dev-Server", "Helios DevServer v1.0");
            res.set("Helios-Node", "localhost:8000");
            res.body() = "";
        }else {
            res.body() = std::move(body);
        }
        res.prepare_payload();
        http::async_write(socket_, res,
            [self = shared_from_this()](beast::error_code, std::size_t) {
                self->socket_.shutdown(tcp::socket::shutdown_send);
            });
    }

    void send_method_not_allowed() {
        http::response<http::string_body> res{
            http::status::method_not_allowed, req_.version()
        };
        res.set(http::field::allow, "GET, HEAD, OPTIONS");
        res.body() = "405 Method Not Allowed";
        res.prepare_payload();
        http::async_write(socket_, res,
            [self = shared_from_this()](beast::error_code, std::size_t) {
                self->socket_.shutdown(tcp::socket::shutdown_send);
            });
    }


    void send_not_found() {
        http::response<http::string_body>{
            http::status::not_found, req_.version()
        };
        // only happens if index.html is missing
    }
};
