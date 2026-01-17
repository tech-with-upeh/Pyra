#include "websocketserver.hpp"

#include <iostream>

/* ======================= WebSocket Manager (DEFINITIONS) ======================= */

std::set<websocket_session_ptr> websocket_manager::sessions_;
std::mutex websocket_manager::mutex_;

void websocket_manager::add(websocket_session_ptr session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(session);
}

void websocket_manager::remove(websocket_session_ptr session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session);
}

void websocket_manager::broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& session : sessions_) {
        session->send(msg);
    }
}

/* ======================= WebSocket Session ======================= */

websocket_session::websocket_session(tcp::socket socket)
    : helios_(std::move(socket)) {}

void websocket_session::run(
    beast::http::request<beast::http::string_body> req)
{
    helios_.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server));

    helios_.async_accept(req,
        [self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                websocket_manager::add(self);
                self->read();
            }
        });
}

void websocket_session::send(const std::string& text) {
    helios_.text(true);
    helios_.async_write(
        net::buffer(text),
        [](beast::error_code, std::size_t) {});
}

void websocket_session::read() {
    helios_.async_read(buffer_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (ec) {
                websocket_manager::remove(self);
                return;
            }

            std::string msg =
                beast::buffers_to_string(self->buffer_.data());
            self->buffer_.consume(self->buffer_.size());

            if (msg[0] == '#') {
                std::cout << "[helios] "
                          << msg.substr(1) << "\n";
            } else {
                std::cout << msg << "\n";
                self->send("Echo: " + msg);
            }

            self->read();
        });
}
