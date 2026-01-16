#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include "httpserver.hpp"
#include "websocketserver.hpp"
#include "heliosfilewatcher.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;



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

        auto watcher = std::make_shared<FileWatcher>(
            ioc,
            "./web",
            ".ink",
            [&]() {
                std::cout << "[Watcher] .ink file changed → reload WS clients\n";
                websocket_manager::broadcast("reload");  // send "reload" to all connected WebSocket clients
            }
        );

        watcher->start();


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
