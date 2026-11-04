#include "gaf/http.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstring>
#include <sstream>

namespace gaf {

namespace {

// Read until a full line (CRLF) is available or timeout. Returns false on
// timeout/error.
bool read_line(int fd, std::string& line, long timeout_ms) {
    line.clear();
    while (true) {
        struct pollfd pfd{fd, POLLIN, 0};
        int pr = poll(&pfd, 1, static_cast<int>(timeout_ms));
        if (pr < 0) { if (errno == EINTR) continue; return false; }
        if (pr == 0) return false; // timeout
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0) return false;
        line.push_back(c);
        if (c == '\n') {
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }
    }
}

// Read exactly `len` bytes (blocking with poll timeout). Returns false on
// error/timeout/short read.
bool read_exact(int fd, std::string& buf, size_t len, long timeout_ms) {
    size_t got = 0;
    while (got < len) {
        struct pollfd pfd{fd, POLLIN, 0};
        int pr = poll(&pfd, 1, static_cast<int>(timeout_ms));
        if (pr < 0) { if (errno == EINTR) continue; return false; }
        if (pr == 0) return false;
        char tmp[8192];
        size_t want = len - got;
        if (want > sizeof(tmp)) want = sizeof(tmp);
        ssize_t n = recv(fd, tmp, want, 0);
        if (n <= 0) return false;
        buf.append(tmp, static_cast<size_t>(n));
        got += static_cast<size_t>(n);
    }
    return true;
}

std::string to_lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

} // namespace

bool http_request(const std::string& host, int port, const std::string& method,
                  const std::string& path, const std::string& body,
                  HttpResponse& out, long timeout_ms) {
    // Resolve + connect.
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = nullptr;
    std::string port_s = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_s.c_str(), &hints, &res) != 0) return false;

    int fd = -1;
    for (auto* ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) break;
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) return false;

    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    // Build request.
    std::ostringstream req;
    req << method << " " << path << " HTTP/1.1\r\n"
        << "Host: " << host << ":" << port << "\r\n"
        << "Connection: close\r\n";
    if (!body.empty()) {
        req << "Content-Type: application/json\r\n"
            << "Content-Length: " << body.size() << "\r\n";
    }
    req << "\r\n" << body;

    std::string req_str = req.str();
    size_t sent = 0;
    while (sent < req_str.size()) {
        ssize_t n = send(fd, req_str.data() + sent, req_str.size() - sent, 0);
        if (n <= 0) { ::close(fd); return false; }
        sent += static_cast<size_t>(n);
    }

    // Read status line.
    std::string status_line;
    if (!read_line(fd, status_line, timeout_ms)) { ::close(fd); return false; }
    // "HTTP/1.1 200 OK"
    std::istringstream ss(status_line);
    std::string httpver;
    int status = 0;
    ss >> httpver >> status;
    out.status = status;

    // Read headers.
    bool chunked = false;
    long content_length = -1;
    std::string line;
    while (read_line(fd, line, timeout_ms)) {
        if (line.empty()) break;
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = to_lower(line.substr(0, colon));
        std::string val = line.substr(colon + 1);
        while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(0, 1);
        if (key == "content-length") content_length = std::stol(val);
        else if (key == "transfer-encoding" && to_lower(val).find("chunked") != std::string::npos)
            chunked = true;
    }

    // Read body.
    if (chunked) {
        std::string size_line;
        while (read_line(fd, size_line, timeout_ms)) {
            if (size_line.empty()) break;
            size_t chunk_size = std::stoul(size_line, nullptr, 16);
            if (chunk_size == 0) break;
            std::string chunk;
            if (!read_exact(fd, chunk, chunk_size, timeout_ms)) { ::close(fd); return false; }
            out.body += chunk;
            std::string crlf;
            read_exact(fd, crlf, 2, timeout_ms); // trailing CRLF
        }
    } else if (content_length >= 0) {
        if (!read_exact(fd, out.body, static_cast<size_t>(content_length), timeout_ms)) {
            ::close(fd);
            return false;
        }
    } else {
        // Read until close.
        char tmp[8192];
        while (true) {
            struct pollfd pfd{fd, POLLIN, 0};
            int pr = poll(&pfd, 1, static_cast<int>(timeout_ms));
            if (pr <= 0) break;
            ssize_t n = recv(fd, tmp, sizeof(tmp), 0);
            if (n <= 0) break;
            out.body.append(tmp, static_cast<size_t>(n));
        }
    }

    ::close(fd);
    return true;
}

bool http_get_json(const std::string& host, int port, const std::string& path,
                   nlohmann::json& out, long timeout_ms) {
    HttpResponse r;
    if (!http_request(host, port, "GET", path, "", r, timeout_ms)) return false;
    try {
        out = nlohmann::json::parse(r.body);
    } catch (...) {
        return false;
    }
    return r.ok();
}

bool http_post_json(const std::string& host, int port, const std::string& path,
                    const nlohmann::json& req, nlohmann::json& out, long timeout_ms) {
    HttpResponse r;
    if (!http_request(host, port, "POST", path, req.dump(), r, timeout_ms)) return false;
    try {
        out = nlohmann::json::parse(r.body);
    } catch (...) {
        return false;
    }
    return r.ok();
}

} // namespace gaf
