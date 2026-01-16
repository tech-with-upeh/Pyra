#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <set>
#include <mutex>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/* ======================= Forward Decls ======================= */

class websocket_session;
using websocket_session_ptr = std::shared_ptr<websocket_session>;

/* ======================= WebSocket Manager (DECLARATION ONLY) ======================= */

class websocket_manager {
public:
    static void add(websocket_session_ptr session);
    static void remove(websocket_session_ptr session);
    static void broadcast(const std::string& msg);

private:
    static std::set<websocket_session_ptr> sessions_;
    static std::mutex mutex_;
};

/* ======================= WebSocket Session ======================= */

class websocket_session : public std::enable_shared_from_this<websocket_session> {
    websocket::stream<tcp::socket> helios_;
    beast::flat_buffer buffer_;

public:
    explicit websocket_session(tcp::socket socket)
        : helios_(std::move(socket)) {}

    void run(beast::http::request<beast::http::string_body> req) {
        helios_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::server));

        helios_.async_accept(req, [self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                websocket_manager::add(self);
                self->read();
            }
        });
    }

    void send(const std::string& text) {
        helios_.text(true);
        helios_.async_write(net::buffer(text),
            [](beast::error_code, std::size_t) {});
    }

private:
    void read() {
        helios_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (ec) {
                websocket_manager::remove(self);
                return;
            }

            std::string msg = beast::buffers_to_string(self->buffer_.data());
            self->buffer_.consume(self->buffer_.size());

            if (msg[0] == '#') {
                std::cout << "[helios] " << msg.substr(1) <<"\n";
            } else {
                self->send("unknown command");
            }

            self->read();
        });
    }
};

/* ======================= WebSocket Manager (DEFINITIONS) ======================= */

inline std::set<websocket_session_ptr> websocket_manager::sessions_;
inline std::mutex websocket_manager::mutex_;

inline void websocket_manager::add(websocket_session_ptr session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(session);
}

inline void websocket_manager::remove(websocket_session_ptr session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session);
}

inline void websocket_manager::broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& session : sessions_) {
        session->send(msg);
    }
}
