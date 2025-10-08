<<<<<<< HEAD
# Mini HTTP Server (C++17 Windows/Linux)

A lightweight multithreaded HTTP server written in pure **C++17** using basic sockets and a simple thread pool.  
It serves static files from a specified folder and logs all requests to console and file.

### Features

- Handles basic **GET** requests  
- Serves static files from configurable web root (default: `www/`)  
- **Thread pool** for concurrent request handling 
- **Config file support** (`config.txt`)  
- Uses modern C++17 features (`std::filesystem`, `std::thread`, `std::mutex`)
- Thread-safe colored console and file **logging**
- Works on **Windows** and **Linux**

```
project/
├── main.cpp            # main server code
└── www/                # static web files
    ├── app.js
    ├── img.png
    ├── index.html
    └── style.css
```

### Configuration (`config.txt`)

Created automatically on first run.
```
Simple HTTP server configuration
PORT = 8080
THREADS = 4
WEB_ROOT = www
LOG_FILE = server.log
```
### Build and run

#### On Windows (Visual Studio or MSVC)
1. Make sure to compile with **C++17**:
/std:c++17
2. Link with Winsock library:
Ws2_32.lib
3. Build and run `server.exe`.

#### On Linux (g++)

```bash
g++ -std=c++17 server.cpp -o server -pthread
./server
```

### Usage
Once started, the server listens on the configured port (default 8080).

Open in your browser:

```
http://localhost:8080/
http://localhost:8080/anyfile.htm
```
Or test via terminal:
```
curl -v http://localhost:8080/
curl -v http://localhost:8080/doesnotexist
```
### Example console output
```
[2025-10-08 12:10:02] [INFO] Loaded config:
[2025-10-08 12:10:02] [OK] Server started on port 8080
[2025-10-08 12:10:10] [HTTP] [127.0.0.1] GET /index.html -> 200
[2025-10-08 12:10:15] [HTTP] [127.0.0.1] GET /notfound -> 404
```

Logs are also saved to server.log.

### HTTP support
Currently supports:
* `GET` - returns static files
* `404 Not Found` for missing files
* `405 Method Not Allowed` for unsupported methods

Automatic MIME detection for:
`.html`, `.css`, `.js`, `.png`, `.jpg`, `.jpeg`, and others (default `text/plain`).

### Example request/response

**Request:**
```
GET /index.html HTTP/1.1
Host: localhost
```
**Response:**
```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1234
Connection: close
```
### How it works

1. Server loads configuration from config.txt.

2. Creates the web root folder if it doesn’t exist.

3. Starts a listening socket on the given port.

4. Accepts incoming connections and pushes them to a thread pool.

5. Each worker thread handles one client request and responds accordingly.

6. All activity is logged to console and file.

### Notes
* Only GET requests are supported in this version.

* No directory traversal protection (avoid serving sensitive paths).

* Designed for learning and local use — not for production deployment.

### License

MIT License.
=======
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
>>>>>>> 05fbcb20e0dbc5af75e13d6e6188e8abcf479787
