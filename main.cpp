#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <thread>
#include <vector>
#include <queue>
#include <ctime>
#include <mutex>
#include <condition_variable>
#include <filesystem>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <functional>   // for std::function
#include <utility>      // for std::move

// ======================
// Config Loader
// ======================
struct Config {
    int port = 8080;
    int threads = 4;
    std::string webRoot = "www";
    std::string logFile = "server.log";
};

Config loadConfig(const std::string& filename) {
    Config cfg;
    std::ifstream file(filename);
    if (!file) {
        std::ofstream create(filename);
        create << "# Simple HTTP server configuration\n"
            << "PORT = 8080\n"
            << "THREADS = 4\n"
            << "WEB_ROOT = www\n"
            << "LOG_FILE = server.log\n";
        std::cout << "Config file created: " << filename << "\n";
        return cfg;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string key, eq, val;
        if (iss >> key >> eq >> val && eq == "=") {
            if (key == "PORT") cfg.port = std::stoi(val);
            else if (key == "THREADS") cfg.threads = std::stoi(val);
            else if (key == "WEB_ROOT") cfg.webRoot = val;
            else if (key == "LOG_FILE") cfg.logFile = val;
        }
    }
    return cfg;
}

// ======================
// Logger
// ======================
struct Logger {
    std::mutex mtx;
    std::ofstream file;

    void init(const std::string& filename) {
        file.open(filename, std::ios::app);
        if (!file) std::cerr << "Warning: cannot open " << filename << "\n";
    }

    std::string ts() {
        std::time_t t = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return buf;
    }

    void log(const std::string& lvl, const std::string& msg, const std::string& color = "") {
        std::lock_guard<std::mutex> lock(mtx);
        std::string line = "[" + ts() + "] [" + lvl + "] " + msg;
        std::cout << color << line << "\033[0m\n";
        if (file.is_open()) file << line << "\n";
    }
} logger;

#define COLOR_INFO "\033[36m"
#define COLOR_WARN "\033[33m"
#define COLOR_ERR  "\033[31m"
#define COLOR_OK   "\033[32m"

#define LOG_INFO(msg) logger.log("INFO", msg, COLOR_INFO)
#define LOG_WARN(msg) logger.log("WARN", msg, COLOR_WARN)
#define LOG_ERR(msg)  logger.log("ERROR", msg, COLOR_ERR)
#define LOG_OK(msg)   logger.log("OK", msg, COLOR_OK)

// ======================
// ThreadPool
// ======================
class ThreadPool {
public:
    explicit ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] { workerLoop(); });
    }

    template<class F>
    void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queueMtx);
            tasks.emplace(std::forward<F>(task));
        }
        cv.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMtx);
            stop = true;
        }
        cv.notify_all();
        for (auto& w : workers) w.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMtx;
    std::condition_variable cv;
    bool stop;

    void workerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMtx);
                cv.wait(lock, [this] { return stop || !tasks.empty(); });
                if (stop && tasks.empty()) return;
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }
};

// ======================
// Helpers
// ======================
std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string getMime(const std::string& path) {
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() &&
            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        };

    if (endsWith(path, ".html")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".png")) return "image/png";
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    return "text/plain";
}

// ======================
// Handle client request
// ======================
void handleClient(SOCKET sock, std::string ip, const Config& cfg) {
    char buf[8192];
    memset(buf, 0, sizeof(buf));
    recv(sock, buf, sizeof(buf) - 1, 0);
    std::string req(buf);

    if (req.empty()) { close(sock); return; }

    std::istringstream ss(req);
    std::string method, path;
    ss >> method >> path;

    if (path == "/") path = "/index.html";
    std::string filePath = cfg.webRoot + path;

    std::string body, mime, status;
    int code = 200;

    if (method == "GET") {
        if (std::filesystem::exists(filePath)) {
            body = readFile(filePath);
            mime = getMime(filePath);
            status = "HTTP/1.1 200 OK";
        }
        else {
            status = "HTTP/1.1 404 Not Found";
            body = "<h1>404 Not Found</h1>";
            mime = "text/html";
            code = 404;
        }
    }
    else {
        status = "HTTP/1.1 405 Method Not Allowed";
        body = "<h1>405 Method Not Allowed</h1>";
        mime = "text/html";
        code = 405;
    }

    std::ostringstream resp;
    resp << status << "\r\n"
        << "Content-Type: " << mime << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;

    send(sock, resp.str().c_str(), resp.str().size(), 0);
    close(sock);

    std::string color = (code == 200 ? COLOR_OK : (code == 404 ? COLOR_WARN : COLOR_ERR));
    logger.log("HTTP", "[" + ip + "] " + method + " " + path + " -> " + std::to_string(code), color);
}

// ======================
// Main
// ======================
int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    Config cfg = loadConfig("config.txt");

    std::filesystem::create_directory(cfg.webRoot);
    logger.init(cfg.logFile);
    LOG_INFO("Loaded config:");
    LOG_INFO("  PORT=" + std::to_string(cfg.port));
    LOG_INFO("  THREADS=" + std::to_string(cfg.threads));
    LOG_INFO("  WEB_ROOT=" + cfg.webRoot);
    LOG_INFO("  LOG_FILE=" + cfg.logFile);

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, 10);

    ThreadPool pool(cfg.threads);
    LOG_OK("Server started on port " + std::to_string(cfg.port));

    while (true) {
        sockaddr_in client{};
        int len = sizeof(client);
        SOCKET clientSock = accept(serverSock, (sockaddr*)&client, &len);
        if (clientSock == INVALID_SOCKET) continue;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
        pool.enqueue([clientSock, ip, &cfg] {
            handleClient(clientSock, ip, cfg);
            });
    }

#ifdef _WIN32
    WSACleanup();
#endif
    close(serverSock);
}
