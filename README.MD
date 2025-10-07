# Mini HTTP Server (C++17)

A simple HTTP server written in pure C++17 using basic sockets.
It supports GET and POST requests, serves static files from a folder,
and includes simple caching with Last-Modified headers.

### Features

- Handles GET and POST requests
- Serves static files from www/ directory
- Sends correct Content-Type (MIME) headers
- Supports HTTP 304 Not Modified (cache)
- Multithreaded (one thread per client)
- Simple and beginner-friendly code

```
project/
├── server.cpp          # main server code
└── www/                # static web files
    ├── index.html
    ├── about.html
    ├── style.css
    └── form.html
```
### Build and run

Make sure you have g++ and pthread support.
```
g++ -std=c++17 server.cpp -o server -pthread
./server
```
By default, the server runs on port 8080.

Then open your browser and go to:

- http://localhost:8080/
- http://localhost:8080/about.html
### Example request

**GET request:**
```
GET /index.html HTTP/1.1
Host: localhost
```
**POST request:**
```
POST / HTTP/1.1
Host: localhost
Content-Length: 13

name=12345
```
**Response example:**
```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 120
Cache-Control: max-age=60
Last-Modified: Mon, 06 Oct 2025 10:30:00 GMT
Connection: close
```
### How it works

1. The server opens a TCP socket on port 8080.

2. When a client connects, it starts a new thread to handle the request.

3. It reads the HTTP request and checks if the file exists in www/.

4. If found, it sends the file with correct headers and caching info.

5. If not, it returns 404 Not Found.

### License

MIT License.
