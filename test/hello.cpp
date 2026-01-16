#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/* ======================= WebSocket Session ======================= */

class websocket_session : public std::enable_shared_from_this<websocket_session> {
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;

public:
    explicit websocket_session(tcp::socket socket)
        : ws_(std::move(socket)) {}

    void run(http::request<http::string_body> req) {
        ws_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::server));

        ws_.async_accept(req,
            [self = shared_from_this()](beast::error_code ec) {
                if (!ec) self->read();
            });
    }

private:
    void read() {
        ws_.async_read(buffer_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) return;

                std::string msg = beast::buffers_to_string(self->buffer_.data());
                self->buffer_.consume(self->buffer_.size());

                if (msg == "r") {
                    std::cout << "[WS] reload\n";
                    self->send("reload");
                }
                else if (msg == "s") {
                    std::cout << "[WS] restart\n";
                    self->send("restart");
                }
                else {
                    self->send("unknown command");
                }

                self->read();
            });
    }

    void send(const std::string& text) {
        ws_.text(true);
        ws_.async_write(net::buffer(text),
            [](beast::error_code, std::size_t) {});
    }
};

/* ======================= HTTP Session ======================= */

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
        if (websocket::is_upgrade(req_)) {
            std::make_shared<websocket_session>(
                std::move(socket_))->run(std::move(req_));
            return;
        }

        http::response<http::string_body> res{
            http::status::ok, req_.version()
        };

        res.set(http::field::content_type, "text/html");
        res.body() = R"(
<!DOCTYPE html>
<html>
<body>
<h1>Boost.Beast Dev Server</h1>
<p>Press R to reload, S to restart</p>

<script>
const ws = new WebSocket("ws://10.199.174.116:9000");

ws.onmessage = e => console.log("server:", e.data);

document.addEventListener("keydown", e => {
    if (e.key === "r") ws.send("r");
    if (e.key === "s") ws.send("s");
});
</script>
</body>
</html>
)";
        res.prepare_payload();

        http::async_write(socket_, res,
            [self = shared_from_this()](beast::error_code, std::size_t) {
                self->socket_.shutdown(tcp::socket::shutdown_send);
            });
    }
};

/* ======================= Listener ======================= */

class listener : public std::enable_shared_from_this<listener> {
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    listener(net::io_context& ioc, tcp::endpoint ep)
        : acceptor_(ioc), socket_(ioc) {
        acceptor_.open(ep.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();
    }

    void run() {
        accept();
    }

private:
    void accept() {
        acceptor_.async_accept(socket_,
            [self = shared_from_this()](beast::error_code ec) {
                if (!ec)
                    std::make_shared<http_session>(
                        std::move(self->socket_))->run();
                self->accept();
            });
    }
};

/* ======================= main ======================= */

int main() {
    try {
        net::io_context ioc{1};

        std::make_shared<listener>(
            ioc,
            tcp::endpoint{tcp::v4(), 9000}
        )->run();

        std::cout << "Server running at http://localhost:8080\n";
        ioc.run();
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
