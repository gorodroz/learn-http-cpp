#include <iostream>
#include <string>
#include <cstring>      // memset
#include <sys/socket.h> // socket, bind, listen, accept
#include <netinet/in.h> // sockaddr_in
#include <unistd.h>     // read, write, close

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error: unable to create socket\n";
        return 1;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: unable to bind port\n";
        return 1;
    }

    listen(serverSocket, 5);
    std::cout << "Server started using port 8080...\n";

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Error: unable recivce connecting\n";
            continue;
        }

        char buffer[1024];
        std::memset(buffer, 0, sizeof(buffer));
        read(clientSocket, buffer, sizeof(buffer) - 1);

        std::cout << "Request received:\n" << buffer << "\n";

        std::string body = "<html><body><h1>Hello from C++!</h1></body></html>";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" +
            body;

        send(clientSocket, response.c_str(), response.size(), 0);

        close(clientSocket);
    }

    close(serverSocket);

    return 0;
}
