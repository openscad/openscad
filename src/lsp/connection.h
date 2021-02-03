#pragma once

#include "lsp/project.h"
#include "lsp/lsp.h"

#include <QObject>
#include <QTextStream>

#include <chrono>
#include <unordered_map>
#include <functional>
#include <memory>

class QTcpSocket;
class QSocketNotifier;
class QTextStream;
class QFile;

class ConnectionHandler;
class ResponseMessage;
class ResponseResult;
class ResponseError;
class RequestMessage;
class Connection;

using request_callback_t = std::function<void(const ResponseMessage &, Connection *conn, project *proj)>;

class Connection : public QObject {
    Q_OBJECT

    friend class ConnectionHandler;
public:
    Connection(ConnectionHandler *handler, std::unique_ptr<project> &&project,
            std::unique_ptr<QTextStream> &&in_stream, std::unique_ptr<QTextStream> &&out_stream);

    virtual ~Connection() {};

    static void default_reporting_message_handler(const ResponseMessage &, Connection *, project *);

    // Indicating that "result" should be ignored, but "error" should be printed
    static void no_response_expected(const ResponseMessage &, Connection *, project *);

    void send(RequestMessage &message,
            const std::string &method,
            const RequestId &id,
            request_callback_t = &Connection::default_reporting_message_handler);

    void send(ResponseMessage &message, const RequestId &id);
    void send(ResponseResult &result, const RequestId &id);
    void send(ResponseError &error, const RequestId &id);

    // rvalue-constructed
    void send(ResponseResult &&result, const RequestId &id);
    void send(ResponseError &&error, const RequestId &id);

    // to be called on a regular basis to avoid overfilling the pending messages buffer
    void clean_pending_messages(const std::chrono::system_clock::duration &max_age);
    void handle_pending_response(const ResponseMessage &msg);

    virtual void close() = 0;
    virtual bool is_done() = 0;
    virtual std::string peerName() = 0;

    std::unique_ptr<project> active_project;

    void log(MessageType type, const std::string &message);
    void error(const std::string &message) { log(MessageType::Error, message); }
    void warn(const std::string &message) { log(MessageType::Warning, message); }
    void info(const std::string &message) { log(MessageType::Info, message); }
    void debug(const std::string &message) { log(MessageType::Log, message); }

private:
    enum class PACKET_EXPECT {
        HEADER,
        BODY,
    } packet_state = PACKET_EXPECT::HEADER;
    struct connection_header {
        size_t content_length = 0;
    } header;

    void read_header();
    void read_body();

protected slots:
    void onReadyRead();

protected:
    ConnectionHandler *handler;
    std::unique_ptr<QTextStream> in_stream;
    std::unique_ptr<QTextStream> out_stream;

    // Since QTextStream's readline returns even before the line has been read,
    // this method will actually read a line. (including the \r\n!)
    // Will return an empty string, if no line has been
    boost::optional<QString> real_read_line();
private:
    QString pending_data; // buffer for real_read_line;


protected:
    virtual void send(const QByteArray &buffer);

    struct pending_message {
        pending_message(const request_callback_t &callback) :
            callback(callback), pending_since(std::chrono::system_clock::now())
        {}
        request_callback_t callback;
        std::chrono::system_clock::time_point pending_since;
    };
    std::unordered_map<int, pending_message> pending_messages;

    // Used for outgoing requests
    int next_request_id = 0;
};

class TCPLSPConnection : public Connection {
public:
    TCPLSPConnection(ConnectionHandler *handler, QTcpSocket *client, std::unique_ptr<project> &&project);

    virtual void close();
    virtual bool is_done();
    virtual std::string peerName();

protected:
    QTcpSocket *socket;
};

class StdioLSPConnection : public Connection {
public:
    StdioLSPConnection(ConnectionHandler *handler, std::unique_ptr<project> &&project);
    virtual ~StdioLSPConnection();

    virtual void close() { active = false; }
    virtual bool is_done() { return !active; }
    virtual std::string peerName() { return "stdio"; }

protected:
    std::unique_ptr<QSocketNotifier> notifier;

    bool active = true;
};