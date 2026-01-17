#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
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

class websocket_session
    : public std::enable_shared_from_this<websocket_session>
{
    websocket::stream<tcp::socket> helios_;
    beast::flat_buffer buffer_;

public:
    explicit websocket_session(tcp::socket socket);

    void run(beast::http::request<beast::http::string_body> req);
    void send(const std::string& text);

private:
    void read();
};
