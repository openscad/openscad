#pragma once

#include "project.h"
#include "lsp.h"

#include <QObject>

#include <chrono>
#include <unordered_map>
#include <functional>

class QTcpSocket;

class ConnectionHandler;
class ResponseMessage;
class ResponseResult;
class ResponseError;
class RequestMessage;
class Connection;

using request_callback_t = std::function<void(const ResponseMessage &, Connection *conn, project *proj)>;

class Connection : public QObject {
    Q_OBJECT
public:

    Connection(ConnectionHandler *handler, QTcpSocket *client);

    virtual ~Connection() {};
public:
    static void default_reporting_message_handler(const ResponseMessage &, Connection *, project *);

    // Indicating that "result" should be ignored, but "error" should be printed
    static void no_reponse_expected(const ResponseMessage &, Connection *, project *);

public:
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

    void close();
    bool is_done();

    project active_project;

private slots:
    void onReadyRead();

private:
    enum class PACKET_EXPECT {
        HEADER,
        BODY,
    } packet_state = PACKET_EXPECT::HEADER;
    QByteArray pending_data;

    struct connection_header {
        size_t content_length = 0;
    } header;

    void read_header();
    void read_body();

protected:
    ConnectionHandler *handler;
    QTcpSocket *socket;

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