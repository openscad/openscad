#include "HTTPClient.h"

#ifndef __EMSCRIPTEN__

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <thread>
#include <mutex>
#include <set>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#endif

// URL parsing helper structure
struct ParsedURL {
  std::string scheme;
  std::string host;
  std::string port;
  std::string target;
  std::string error;
};

static ParsedURL parseURL(const std::string& url)
{
  ParsedURL parsed;
  size_t scheme_end = url.find("://");
  std::string rest;

  if (scheme_end != std::string::npos) {
    parsed.scheme = url.substr(0, scheme_end);
    std::transform(parsed.scheme.begin(), parsed.scheme.end(), parsed.scheme.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (parsed.scheme != "http" && parsed.scheme != "https") {
      parsed.error = "Unsupported URL scheme: " + parsed.scheme;
      return parsed;
    }
    rest = url.substr(scheme_end + 3);
  } else {
    parsed.scheme = "https";
    rest = url;
  }

  size_t path_start = rest.find('/');
  std::string host_port;
  if (path_start == std::string::npos) {
    host_port = rest;
    parsed.target = "/";
  } else {
    host_port = rest.substr(0, path_start);
    parsed.target = rest.substr(path_start);
  }

  size_t port_start = host_port.find(':');
  if (port_start == std::string::npos) {
    parsed.host = host_port;
    parsed.port = (parsed.scheme == "https") ? "443" : "80";
  } else {
    parsed.host = host_port.substr(0, port_start);
    parsed.port = host_port.substr(port_start + 1);
  }

  if (parsed.host.empty()) {
    parsed.error = "Invalid URL: host is empty";
  }

  return parsed;
}

#ifdef _WIN32
static void load_system_certificates(boost::asio::ssl::context& ctx)
{
  HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
  if (!hStore) {
    return;
  }

  X509_STORE *store = SSL_CTX_get_cert_store(ctx.native_handle());
  if (!store) {
    CertCloseStore(hStore, 0);
    return;
  }

  PCCERT_CONTEXT pContext = nullptr;
  while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != nullptr) {
    const unsigned char *pbCertEncoded = pContext->pbCertEncoded;
    X509 *x509 = d2i_X509(nullptr, &pbCertEncoded, pContext->cbCertEncoded);
    if (x509) {
      X509_STORE_add_cert(store, x509);
      X509_free(x509);
    }
  }
  CertCloseStore(hStore, 0);
}
#else
static void load_system_certificates(boost::asio::ssl::context& ctx)
{
  ctx.set_default_verify_paths();
}
#endif

// A session representing a single HTTP/HTTPS request/response lifetime
class CancelableSession
{
public:
  virtual ~CancelableSession() = default;
  virtual void cancel() = 0;
};

class SessionRegistry
{
public:
  virtual ~SessionRegistry() = default;
  virtual void register_session(std::shared_ptr<CancelableSession> session) = 0;
  virtual void unregister_session(std::shared_ptr<CancelableSession> session) = 0;
};

template <bool IsSSL>
class Session : public CancelableSession, public std::enable_shared_from_this<Session<IsSSL>>
{
public:
  using StreamType = typename std::conditional<IsSSL, boost::asio::ssl::stream<boost::beast::tcp_stream>,
                                               boost::beast::tcp_stream>::type;

  Session(boost::asio::io_context& io_ctx, SessionRegistry *registry,
          boost::asio::ssl::context *ssl_ctx,  // null if !IsSSL
          const std::string& host, const std::string& port, const std::string& target,
          boost::beast::http::request<boost::beast::http::string_body> req,
          HTTPClient::ResponseCallback on_response, HTTPClient::ChunkCallback on_chunk,
          HTTPClient::ErrorCallback on_error, HTTPClient::CompleteCallback on_complete)
    : resolver_(boost::asio::make_strand(io_ctx)),
      stream_(create_stream(io_ctx, ssl_ctx)),
      registry_(registry),
      host_(host),
      port_(port),
      target_(target),
      req_(std::move(req)),
      on_response_(std::move(on_response)),
      on_chunk_(std::move(on_chunk)),
      on_error_(std::move(on_error)),
      on_complete_(std::move(on_complete)),
      body_offset_(0)
  {
  }

  void run()
  {
    if (registry_) {
      registry_->register_session(this->shared_from_this());
    }

    if constexpr (IsSSL) {
      if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
        boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                    boost::asio::error::get_ssl_category()};
        fail(ec, "SNI set_host_name");
        return;
      }
    }

    resolver_.async_resolve(
      host_, port_, boost::beast::bind_front_handler(&Session::on_resolve, this->shared_from_this()));
  }

  void cancel() override
  {
    auto self = this->shared_from_this();
    boost::asio::post(resolver_.get_executor(),
                      [this, self]() { boost::beast::get_lowest_layer(stream_).close(); });
  }

private:
  SessionRegistry *registry_;
  boost::asio::ip::tcp::resolver resolver_;
  StreamType stream_;
  std::string host_;
  std::string port_;
  std::string target_;
  boost::beast::http::request<boost::beast::http::string_body> req_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::response_parser<boost::beast::http::string_body> parser_;

  HTTPClient::ResponseCallback on_response_;
  HTTPClient::ChunkCallback on_chunk_;
  HTTPClient::ErrorCallback on_error_;
  HTTPClient::CompleteCallback on_complete_;
  std::size_t body_offset_;

  static StreamType create_stream(boost::asio::io_context& io_ctx, boost::asio::ssl::context *ssl_ctx)
  {
    if constexpr (IsSSL) {
      return StreamType(boost::asio::make_strand(io_ctx), *ssl_ctx);
    } else {
      return StreamType(boost::asio::make_strand(io_ctx));
    }
  }

  void remove_self()
  {
    if (registry_) {
      registry_->unregister_session(this->shared_from_this());
    }
  }

  void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results)
  {
    if (ec) {
      return fail(ec, "resolve");
    }

    boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    boost::beast::get_lowest_layer(stream_).async_connect(
      results, boost::beast::bind_front_handler(&Session::on_connect, this->shared_from_this()));
  }

  void on_connect(boost::beast::error_code ec,
                  boost::asio::ip::tcp::resolver::results_type::endpoint_type)
  {
    if (ec) {
      return fail(ec, "connect");
    }

    if constexpr (IsSSL) {
      boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
      stream_.async_handshake(
        boost::asio::ssl::stream_base::client,
        boost::beast::bind_front_handler(&Session::on_handshake, this->shared_from_this()));
    } else {
      write_request();
    }
  }

  void on_handshake(boost::beast::error_code ec)
  {
    if (ec) {
      return fail(ec, "handshake");
    }

    write_request();
  }

  void write_request()
  {
    boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    boost::beast::http::async_write(
      stream_, req_, boost::beast::bind_front_handler(&Session::on_write, this->shared_from_this()));
  }

  void on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
  {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      return fail(ec, "write");
    }

    if (!on_chunk_) {
      boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
      boost::beast::http::async_read(
        stream_, buffer_, parser_,
        boost::beast::bind_front_handler(&Session::on_read, this->shared_from_this()));
    } else {
      boost::beast::get_lowest_layer(stream_).expires_never();
      read_chunk();
    }
  }

  void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
  {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      return fail(ec, "read");
    }

    if (on_response_) {
      on_response_(parser_.get().result_int(), parser_.get().body());
    }

    close_connection();
  }

  void read_chunk()
  {
    boost::beast::http::async_read_some(
      stream_, buffer_, parser_,
      boost::beast::bind_front_handler(&Session::on_read_chunk, this->shared_from_this()));
  }

  void on_read_chunk(boost::beast::error_code ec, std::size_t bytes_transferred)
  {
    boost::ignore_unused(bytes_transferred);

    if (!ec) {
      auto& body = parser_.get().body();
      if (body.size() > body_offset_) {
        std::string new_data = body.substr(body_offset_);
        body_offset_ = body.size();
        if (on_chunk_) {
          on_chunk_(parser_.get().result_int(), new_data);
        }
      }

      if (parser_.is_done()) {
        if (on_complete_) {
          on_complete_();
        }
        close_connection();
      } else {
        read_chunk();
      }
    } else if (ec == boost::beast::http::error::end_of_stream) {
      auto& body = parser_.get().body();
      if (body.size() > body_offset_) {
        std::string new_data = body.substr(body_offset_);
        body_offset_ = body.size();
        if (on_chunk_) {
          on_chunk_(parser_.get().result_int(), new_data);
        }
      }
      if (on_complete_) {
        on_complete_();
      }
      close_connection();
    } else {
      return fail(ec, "read_chunk");
    }
  }

  void close_connection()
  {
    remove_self();
    if constexpr (IsSSL) {
      boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(5));
      stream_.async_shutdown(
        boost::beast::bind_front_handler(&Session::on_shutdown, this->shared_from_this()));
    } else {
      boost::beast::error_code ec;
      boost::beast::get_lowest_layer(stream_).socket().shutdown(
        boost::asio::ip::tcp::socket::shutdown_both, ec);
      boost::beast::get_lowest_layer(stream_).close();
    }
  }

  void on_shutdown(boost::beast::error_code ec)
  {
    boost::ignore_unused(ec);
    boost::beast::get_lowest_layer(stream_).close();
  }

  void fail(boost::beast::error_code ec, const char *what)
  {
    remove_self();
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }

    std::string msg = std::string(what) + ": " + ec.message();
    if (on_error_) {
      on_error_(msg);
    }
  }
};

// Private implementation structure
class HTTPClient::Impl : public SessionRegistry
{
public:
  boost::asio::io_context io_ctx;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
  boost::asio::ssl::context ssl_ctx;
  std::thread background_thread;

  std::mutex sessions_mutex;
  std::set<std::shared_ptr<CancelableSession>> active_sessions;

  void register_session(std::shared_ptr<CancelableSession> session) override
  {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    active_sessions.insert(session);
  }

  void unregister_session(std::shared_ptr<CancelableSession> session) override
  {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    active_sessions.erase(session);
  }

  Impl()
    : work_guard(boost::asio::make_work_guard(io_ctx)), ssl_ctx(boost::asio::ssl::context::tlsv12_client)
  {
    ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    load_system_certificates(ssl_ctx);

    background_thread = std::thread([this]() { io_ctx.run(); });
  }

  ~Impl() override
  {
    cancelPendingRequests();
    work_guard.reset();
    io_ctx.stop();
    if (background_thread.joinable()) {
      background_thread.join();
    }
  }

  void cancelPendingRequests()
  {
    std::vector<std::shared_ptr<CancelableSession>> to_cancel;
    {
      std::lock_guard<std::mutex> lock(sessions_mutex);
      for (const auto& s : active_sessions) {
        to_cancel.push_back(s);
      }
    }
    for (const auto& s : to_cancel) {
      s->cancel();
    }
  }

  // Rule of 5
  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;
};

HTTPClient::HTTPClient() : impl(std::make_unique<Impl>())
{
}

HTTPClient::~HTTPClient() = default;

HTTPClient::HTTPClient(HTTPClient&&) noexcept = default;
HTTPClient& HTTPClient::operator=(HTTPClient&&) noexcept = default;

void HTTPClient::cancelPendingRequests()
{
  impl->cancelPendingRequests();
}

void HTTPClient::asyncGet(const std::string& url, const Headers& headers, ResponseCallback on_response,
                          ErrorCallback on_error)
{
  ParsedURL parsed = parseURL(url);
  if (!parsed.error.empty()) {
    if (on_error) {
      on_error(parsed.error);
    }
    return;
  }
  if (parsed.host.empty()) {
    if (on_error) {
      on_error("Invalid URL: host is empty");
    }
    return;
  }

  boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get,
                                                                   parsed.target, 11};
  req.set(boost::beast::http::field::host, parsed.host);
  req.set(boost::beast::http::field::user_agent, "OpenSCAD/AI-Service");
  for (const auto& header : headers) {
    req.set(header.first, header.second);
  }

  if (parsed.scheme == "https") {
    auto session = std::make_shared<Session<true>>(
      impl->io_ctx, impl.get(), &impl->ssl_ctx, parsed.host, parsed.port, parsed.target, std::move(req),
      std::move(on_response), nullptr, std::move(on_error), nullptr);
    session->run();
  } else if (parsed.scheme == "http") {
    auto session = std::make_shared<Session<false>>(
      impl->io_ctx, impl.get(), nullptr, parsed.host, parsed.port, parsed.target, std::move(req),
      std::move(on_response), nullptr, std::move(on_error), nullptr);
    session->run();
  } else {
    if (on_error) {
      on_error("Unsupported URL scheme: " + parsed.scheme);
    }
  }
}

void HTTPClient::asyncPost(const std::string& url, const Headers& headers, const std::string& body,
                           ResponseCallback on_response, ErrorCallback on_error)
{
  ParsedURL parsed = parseURL(url);
  if (!parsed.error.empty()) {
    if (on_error) {
      on_error(parsed.error);
    }
    return;
  }
  if (parsed.host.empty()) {
    if (on_error) {
      on_error("Invalid URL: host is empty");
    }
    return;
  }

  boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::post,
                                                                   parsed.target, 11};
  req.set(boost::beast::http::field::host, parsed.host);
  req.set(boost::beast::http::field::user_agent, "OpenSCAD/AI-Service");
  req.set(boost::beast::http::field::content_type, "application/json");
  for (const auto& header : headers) {
    req.set(header.first, header.second);
  }
  req.body() = body;
  req.prepare_payload();

  if (parsed.scheme == "https") {
    auto session = std::make_shared<Session<true>>(
      impl->io_ctx, impl.get(), &impl->ssl_ctx, parsed.host, parsed.port, parsed.target, std::move(req),
      std::move(on_response), nullptr, std::move(on_error), nullptr);
    session->run();
  } else if (parsed.scheme == "http") {
    auto session = std::make_shared<Session<false>>(
      impl->io_ctx, impl.get(), nullptr, parsed.host, parsed.port, parsed.target, std::move(req),
      std::move(on_response), nullptr, std::move(on_error), nullptr);
    session->run();
  } else {
    if (on_error) {
      on_error("Unsupported URL scheme: " + parsed.scheme);
    }
  }
}

void HTTPClient::asyncPostStream(const std::string& url, const Headers& headers, const std::string& body,
                                 ChunkCallback on_chunk, ErrorCallback on_error,
                                 CompleteCallback on_complete)
{
  ParsedURL parsed = parseURL(url);
  if (!parsed.error.empty()) {
    if (on_error) {
      on_error(parsed.error);
    }
    return;
  }
  if (parsed.host.empty()) {
    if (on_error) {
      on_error("Invalid URL: host is empty");
    }
    return;
  }

  boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::post,
                                                                   parsed.target, 11};
  req.set(boost::beast::http::field::host, parsed.host);
  req.set(boost::beast::http::field::user_agent, "OpenSCAD/AI-Service");
  req.set(boost::beast::http::field::content_type, "application/json");
  for (const auto& header : headers) {
    req.set(header.first, header.second);
  }
  req.body() = body;
  req.prepare_payload();

  if (parsed.scheme == "https") {
    auto session = std::make_shared<Session<true>>(
      impl->io_ctx, impl.get(), &impl->ssl_ctx, parsed.host, parsed.port, parsed.target, std::move(req),
      nullptr, std::move(on_chunk), std::move(on_error), std::move(on_complete));
    session->run();
  } else if (parsed.scheme == "http") {
    auto session = std::make_shared<Session<false>>(
      impl->io_ctx, impl.get(), nullptr, parsed.host, parsed.port, parsed.target, std::move(req),
      nullptr, std::move(on_chunk), std::move(on_error), std::move(on_complete));
    session->run();
  } else {
    if (on_error) {
      on_error("Unsupported URL scheme: " + parsed.scheme);
    }
  }
}

#else  // __EMSCRIPTEN__

class HTTPClient::Impl
{
};

HTTPClient::HTTPClient() : impl(std::make_unique<Impl>())
{
}
HTTPClient::~HTTPClient() = default;
HTTPClient::HTTPClient(HTTPClient&&) noexcept = default;
HTTPClient& HTTPClient::operator=(HTTPClient&&) noexcept = default;

void HTTPClient::asyncGet(const std::string&, const Headers&, ResponseCallback, ErrorCallback on_error)
{
  if (on_error) {
    on_error("Network client is not supported on WebAssembly.");
  }
}

void HTTPClient::asyncPost(const std::string&, const Headers&, const std::string&, ResponseCallback,
                           ErrorCallback on_error)
{
  if (on_error) {
    on_error("Network client is not supported on WebAssembly.");
  }
}

void HTTPClient::asyncPostStream(const std::string&, const Headers&, const std::string&, ChunkCallback,
                                 ErrorCallback on_error, CompleteCallback)
{
  if (on_error) {
    on_error("Network client is not supported on WebAssembly.");
  }
}

#endif  // __EMSCRIPTEN__
