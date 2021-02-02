#pragma once

#include "messages.h"
#include "lsp.h"
#include "connection.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include <cstdint>

#include <QObject>
#include <QTcpServer>
#include <QString>

// Forward declare Connection in order to speed up compile times
class Connection;
class project;

class ConnectionHandler : public QObject {
	Q_OBJECT
    /** This is the listener class which creates threads with Connections* running */

    friend class Connection;
public:
    using project_initializer = std::function<std::unique_ptr<project>()>;

    ConnectionHandler(QObject *parent, const project_initializer &initializer, uint16_t port=23725); // 0x5CAD = 23725
    virtual ~ConnectionHandler();

    void send(RequestMessage &message,
            const std::string &method,
            request_callback_t = &Connection::default_reporting_message_handler);

private slots:
	// Networking magic
    void onNewConnection();
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

protected:
    // Implemented in decoding.cc - needed for scoping of the decoding template magic.
    void register_messages();
    // Implemented in decoding.cc - needed for scoping of the decoding template magic.
    void handle_message(const QString &, Connection *);

private:
	// Since the message handling is single threaded
    RequestId active_id;
    std::unordered_map<std::string, std::function<std::unique_ptr<RequestMessage>(decode_env &)>> typemap;

    project_initializer project_init_callback;

	QTcpServer server;
    std::list<std::unique_ptr<Connection>> connections;
};
