#include <boost/asio.hpp>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <vector>
#include <memory>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

class FileWatcher : public std::enable_shared_from_this<FileWatcher> {
    fs::path dir_;
    boost::asio::io_context& ioc_;
    boost::asio::steady_timer timer_;
    std::unordered_map<fs::path, fs::file_time_type> last_write_;
    std::function<void()> on_change_;
    std::string extension_;

public:
    FileWatcher(boost::asio::io_context& ioc, fs::path dir,
                const std::string& extension,
                std::function<void()> on_change)
        : dir_(std::move(dir)),
          ioc_(ioc),
          timer_(ioc),
          on_change_(on_change),
          extension_(extension)
    {}

    void start() {
        scan();
        poll();
    }

private:
    void scan() {
        for (auto& entry : fs::recursive_directory_iterator(dir_)) {
            if (fs::is_regular_file(entry) && entry.path().extension() == extension_) {
                last_write_[entry.path()] = fs::last_write_time(entry);
            }
        }
    }

    void poll() {
        timer_.expires_after(500ms); // poll every 500ms
        timer_.async_wait([self = shared_from_this()](const boost::system::error_code&) {
            bool changed = false;
            for (auto& entry : fs::recursive_directory_iterator(self->dir_)) {
                if (!fs::is_regular_file(entry) || entry.path().extension() != self->extension_) continue;

                auto path = entry.path();
                auto write_time = fs::last_write_time(path);

                if (!self->last_write_.contains(path) || self->last_write_[path] != write_time) {
                    self->last_write_[path] = write_time;
                    changed = true;
                }
            }

            if (changed && self->on_change_) self->on_change_();

            self->poll(); // repeat
        });
    }
};
