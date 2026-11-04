#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace gaf {

// Minimal HTTP/1.1 client over POSIX sockets.
//
// Supports the audio.cpp server's response style: Content-Length bodies and
// Connection: close. No TLS (the server is local, http://127.0.0.1). No
// external dependencies.
struct HttpResponse {
    int status = 0;
    std::string body;
    bool ok() const { return status >= 200 && status < 300; }
};

// Perform a request. `method` is GET/POST. `path` starts with '/'.
// `body` is sent for POST (Content-Type: application/json).
// Returns false on transport error (status stays 0).
bool http_request(const std::string& host, int port, const std::string& method,
                  const std::string& path, const std::string& body,
                  HttpResponse& out, long timeout_ms = 600000);

// Convenience: GET a JSON endpoint and parse the body.
bool http_get_json(const std::string& host, int port, const std::string& path,
                   nlohmann::json& out, long timeout_ms = 600000);

// Convenience: POST a JSON body and parse the JSON response.
bool http_post_json(const std::string& host, int port, const std::string& path,
                    const nlohmann::json& req, nlohmann::json& out,
                    long timeout_ms = 600000);

} // namespace gaf
