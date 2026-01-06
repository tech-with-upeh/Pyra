// server.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <string>

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #define INVALID_SOCKET -1
    #define SOCKET int
    #define closesocket close
#endif

namespace fs = std::filesystem;

// MIME types
std::unordered_map<std::string, std::string> mime_types = {
    {".html","text/html"},
    {".htm","text/html"},
    {".js","application/javascript"},
    {".css","text/css"},
    {".wasm","application/wasm"},
    {".png","image/png"},
    {".jpg","image/jpeg"},
    {".jpeg","image/jpeg"},
    {".gif","image/gif"},
    {".svg","image/svg+xml"},
    {".json","application/json"}
};

std::string get_mime(const std::string& path) {
    auto ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (mime_types.count(ext)) return mime_types[ext];
    return "application/octet-stream";
}

// Read file into string
std::string read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    return std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
}

// Send HTTP response
void send_response(SOCKET client, const std::string& content, const std::string& content_type, int status_code=200) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code << " OK\r\n";
    oss << "Content-Length: " << content.size() << "\r\n";
    oss << "Content-Type: " << content_type << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << content;

    std::string response = oss.str();
    send(client, response.c_str(), static_cast<int>(response.size()), 0);
}

// Handle single client
void handle_client(SOCKET client, const std::string& web_root) {
    char buffer[4096];
    int bytes_received = recv(client, buffer, sizeof(buffer)-1, 0);
    if (bytes_received <= 0) {
        closesocket(client);
        return;
    }
    buffer[bytes_received] = '\0';
    std::istringstream request(buffer);
    std::string method, path, version;
    request >> method >> path >> version;

    // Only handle GET
    if (method != "GET") {
        send_response(client, "Method Not Allowed", "text/plain", 405);
        closesocket(client);
        return;
    }

    // Remove query string
    size_t qpos = path.find('?');
    if (qpos != std::string::npos) path = path.substr(0, qpos);

    // Map to filesystem
    fs::path file_path = fs::path(web_root) / path.substr(1); // remove leading '/'
    if (!fs::exists(file_path) || fs::is_directory(file_path)) {
        // SPA fallback
        file_path = fs::path(web_root) / "index.html";
    }

    std::string content = read_file(file_path);
    if (content.empty()) {
        send_response(client, "File Not Found", "text/plain", 404);
    } else {
        send_response(client, content, get_mime(file_path.string()), 200);
    }

    closesocket(client);
}

int main() {
    const std::string web_root = "web"; // your SPA folder
    const int port = 8000;

#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Serving on http://0.0.0.0:" << port << "\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client == INVALID_SOCKET) continue;

        std::thread(handle_client, client, web_root).detach();
    }

#if defined(_WIN32)
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif

    return 0;
}
