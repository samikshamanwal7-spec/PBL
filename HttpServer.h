#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef _WIN32
  // winsock2.h pulls in windows.h; keep it from defining min/max macros
  // (which break the C++ standard headers) and from adding extras.
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
#endif

#include <string>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstring>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define CLOSE_SOCKET closesocket
  #define BAD_SOCKET INVALID_SOCKET
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define CLOSE_SOCKET close
  #define BAD_SOCKET (-1)
#endif

// ============================================================
// HttpServer.h -- Web front end (transport layer)
//
// A tiny HTTP/1.1 server built only on the operating system's
// socket API (no external libraries, no internet). It listens on
// 127.0.0.1 only, so the page can be opened from this computer
// and from nowhere else. It knows nothing about the stock logic:
// it parses a request, calls the handler, and sends the reply.
// ============================================================

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> params; // query string + form body
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
};

inline std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') out += ' ';
        else if (s[i] == '%' && i + 2 < s.size() &&
                 std::isxdigit(static_cast<unsigned char>(s[i + 1])) &&
                 std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
            out += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else out += s[i];
    }
    return out;
}

inline void parseParams(const std::string& s, std::map<std::string, std::string>& out) {
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t amp = s.find('&', pos);
        if (amp == std::string::npos) amp = s.size();
        std::string pair = s.substr(pos, amp - pos);
        if (!pair.empty()) {
            size_t eq = pair.find('=');
            if (eq == std::string::npos) out[urlDecode(pair)] = "";
            else out[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
        }
        if (amp == s.size()) break;
        pos = amp + 1;
    }
}

class HttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    explicit HttpServer(Handler h) : handler(std::move(h)) {}

    // Tries startPort, startPort+1, ... Returns the port used, or -1.
    int listenOn(int startPort, int tries = 10) {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
#endif
        for (int p = startPort; p < startPort + tries; ++p) {
            sock_t s = socket(AF_INET, SOCK_STREAM, 0);
            if (s == BAD_SOCKET) continue;
#ifndef _WIN32
            int yes = 1;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
#endif
            sockaddr_in addr;
            std::memset(&addr, 0, sizeof addr);
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<unsigned short>(p));
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // this computer only
            if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof addr) == 0 &&
                listen(s, 16) == 0) {
                listener = s;
                return p;
            }
            CLOSE_SOCKET(s);
        }
        return -1;
    }

    // Serves forever. Each connection gets its own thread, but the
    // handler itself runs under one lock, so backend data is never
    // touched by two requests at once.
    void run() {
        while (true) {
            sock_t c = accept(listener, nullptr, nullptr);
            if (c == BAD_SOCKET) continue;
            std::thread([this, c]() { handleConnection(c); }).detach();
        }
    }

private:
    Handler handler;
    sock_t listener = BAD_SOCKET;
    std::mutex handlerLock;

    static const char* statusText(int code) {
        switch (code) {
            case 200: return "OK";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 404: return "Not Found";
            default:  return "Internal Server Error";
        }
    }

    static void sendAll(sock_t c, const std::string& data) {
        size_t sent = 0;
        while (sent < data.size()) {
#ifdef MSG_NOSIGNAL
            int n = static_cast<int>(send(c, data.data() + sent, data.size() - sent, MSG_NOSIGNAL));
#else
            int n = static_cast<int>(send(c, data.data() + sent, static_cast<int>(data.size() - sent), 0));
#endif
            if (n <= 0) break;
            sent += static_cast<size_t>(n);
        }
    }

    void handleConnection(sock_t c) {
        // Browsers sometimes open connections they never use; don't wait forever.
#ifdef _WIN32
        DWORD tv = 5000;
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#else
        timeval tv; tv.tv_sec = 5; tv.tv_usec = 0;
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#endif
        const size_t LIMIT = 1 << 20; // 1 MB cap on a request
        std::string data;
        char buf[4096];
        size_t headerEnd = std::string::npos;
        while (data.size() < LIMIT) {
            int n = static_cast<int>(recv(c, buf, sizeof buf, 0));
            if (n <= 0) break;
            data.append(buf, static_cast<size_t>(n));
            headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) break;
        }
        if (headerEnd == std::string::npos) { CLOSE_SOCKET(c); return; }

        std::string header = data.substr(0, headerEnd);
        std::string body = data.substr(headerEnd + 4);

        std::string lower = header;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char ch) { return std::tolower(ch); });
        size_t contentLength = 0;
        size_t cl = lower.find("content-length:");
        if (cl != std::string::npos) {
            try { contentLength = static_cast<size_t>(std::stoul(lower.substr(cl + 15))); }
            catch (...) { contentLength = 0; }
        }
        while (body.size() < contentLength && body.size() < LIMIT) {
            int n = static_cast<int>(recv(c, buf, sizeof buf, 0));
            if (n <= 0) break;
            body.append(buf, static_cast<size_t>(n));
        }

        // Request line: METHOD target HTTP/1.1
        std::istringstream first(header.substr(0, header.find("\r\n")));
        HttpRequest req;
        std::string target, version;
        first >> req.method >> target >> version;
        size_t q = target.find('?');
        req.path = urlDecode(q == std::string::npos ? target : target.substr(0, q));
        if (q != std::string::npos) parseParams(target.substr(q + 1), req.params);
        req.body = body;
        if (req.method == "POST") parseParams(body, req.params);

        HttpResponse res;
        try {
            std::lock_guard<std::mutex> lock(handlerLock);
            res = handler(req);
        } catch (...) {
            res.status = 500;
            res.body = "{\"ok\":false,\"error\":\"Internal server error\"}";
        }

        std::ostringstream out;
        out << "HTTP/1.1 " << res.status << " " << statusText(res.status) << "\r\n"
            << "Content-Type: " << res.contentType << "\r\n"
            << "Content-Length: " << res.body.size() << "\r\n"
            << "Cache-Control: no-store\r\n"
            << "Connection: close\r\n\r\n";
        sendAll(c, out.str() + res.body);
        CLOSE_SOCKET(c);
    }
};

#endif // HTTP_SERVER_H
