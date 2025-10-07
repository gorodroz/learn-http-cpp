#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>

// ======================
// Utility: Get formatted time string
// ======================
std::string formatTime(std::time_t t) {
    char buf[80];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&t));
    return std::string(buf);
}

// ======================
// Get MIME type by file extension
// ======================
std::string getMimeType(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css"))  return "text/css";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".txt"))  return "text/plain";
    return "application/octet-stream";
}

// ======================
// Read file content
// ======================
std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// ======================
// Extract POST body
// ======================
std::string extractPostBody(const std::string& request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return request.substr(pos + 4);
}

// ======================
// Parse header value (simple)
// ======================
std::string getHeaderValue(const std::string& req, const std::string& header) {
    size_t pos = req.find(header);
    if (pos == std::string::npos) return "";
    size_t start = pos + header.size();
    size_t end = req.find("\r\n", start);
    if (end == std::string::npos) end = req.size();
    return req.substr(start, end - start);
}

// ======================
// Handle client connection
// ======================
void handleClient(int clientSocket) {
    char buffer[8192];
    std::memset(buffer, 0, sizeof(buffer));
    read(clientSocket, buffer, sizeof(buffer) - 1);
    std::string request(buffer);

    std::istringstream reqStream(request);
    std::string method, path;
    reqStream >> method >> path;
    std::cout << "[INFO] " << method << " " << path << "\n";

    std::string response;
    std::string body;
    std::string status;
    std::string mime;
    std::string headers;

    // Map URL path to file
    std::string filePath = "www" + path;
    if (path == "/") filePath = "www/index.html";

    struct stat fileStat {};
    bool fileExists = (stat(filePath.c_str(), &fileStat) == 0);

    // Handle GET
    if (method == "GET") {
        if (fileExists) {
            // Get file modification time
            std::time_t mtime = fileStat.st_mtime;
            std::string lastModified = formatTime(mtime);

            // Check for If-Modified-Since
            std::string ims = getHeaderValue(request, "If-Modified-Since: ");
            if (!ims.empty()) {
                // Compare modification times
                std::tm tm{};
                strptime(ims.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm);
                std::time_t imsTime = timegm(&tm);
                if (std::difftime(mtime, imsTime) <= 0) {
                    // File not modified
                    status = "HTTP/1.1 304 Not Modified";
                    headers = "Cache-Control: max-age=60\r\n"
                        "Last-Modified: " + lastModified + "\r\n"
                        "Connection: close\r\n\r\n";
                    response = status + "\r\n" + headers;
                    send(clientSocket, response.c_str(), response.size(), 0);
                    close(clientSocket);
                    return;
                }
            }

            // Read file
            body = readFile(filePath);
            mime = getMimeType(filePath);

            status = "HTTP/1.1 200 OK";
            headers = "Content-Type: " + mime + "\r\n" +
                "Content-Length: " + std::to_string(body.size()) + "\r\n" +
                "Cache-Control: max-age=60\r\n" +
                "Last-Modified: " + lastModified + "\r\n" +
                "Connection: close\r\n\r\n";
        }
        else {
            status = "HTTP/1.1 404 Not Found";
            body = "<html><body><h1>404 Not Found</h1><p>" + path + "</p></body></html>";
            headers = "Content-Type: text/html\r\n"
                "Content-Length: " + std::to_string(body.size()) + "\r\n"
                "Connection: close\r\n\r\n";
        }
    }
    // Handle POST
    else if (method == "POST") {
        std::string postBody = extractPostBody(request);
        body = "<html><body><h1>POST Received</h1><pre>" + postBody + "</pre></body></html>";
        status = "HTTP/1.1 200 OK";
        headers = "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n";
    }
    // Other methods
    else {
        status = "HTTP/1.1 405 Method Not Allowed";
        body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        headers = "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n";
    }

    // Send response
    response = status + "\r\n" + headers + body;
    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

// ======================
// Main function
// ======================
int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error: could not create socket\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: could not bind port\n";
        return 1;
    }

    listen(serverSocket, 5);
    std::cout << "Mini HTTP server with caching started on port 8080...\n";

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Error: failed to accept connection\n";
            continue;
        }
        std::thread(handleClient, clientSocket).detach();
    }

    close(serverSocket);
    return 0;
}
