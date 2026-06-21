#ifndef OPENSCAD_HTTPCLIENT_H
#define OPENSCAD_HTTPCLIENT_H

#include <string>
#include <map>
#include <functional>
#include <memory>

class HTTPClient
{
public:
  using Headers = std::map<std::string, std::string>;
  using ResponseCallback = std::function<void(int status_code, const std::string& body)>;
  using ChunkCallback = std::function<void(int status_code, const std::string& chunk)>;
  using ErrorCallback = std::function<void(const std::string& error_msg)>;
  using CompleteCallback = std::function<void()>;

  HTTPClient();
  ~HTTPClient();

  // Prevent copy, allow move
  HTTPClient(const HTTPClient&) = delete;
  HTTPClient& operator=(const HTTPClient&) = delete;
  HTTPClient(HTTPClient&&) noexcept;
  HTTPClient& operator=(HTTPClient&&) noexcept;

  // Asynchronous HTTP/HTTPS GET request
  void asyncGet(const std::string& url, const Headers& headers, ResponseCallback on_response,
                ErrorCallback on_error);

  // Asynchronous HTTP/HTTPS POST request
  void asyncPost(const std::string& url, const Headers& headers, const std::string& body,
                 ResponseCallback on_response, ErrorCallback on_error);

  // Asynchronous HTTP/HTTPS POST request with streaming response (SSE / Chunked transfer)
  void asyncPostStream(const std::string& url, const Headers& headers, const std::string& body,
                       ChunkCallback on_chunk, ErrorCallback on_error, CompleteCallback on_complete);

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

#endif  // OPENSCAD_HTTPCLIENT_H
