#include "connection_handler.h"
#include "connection.h"
#include "messages.h"

#include <QTcpSocket>

ConnectionHandler::ConnectionHandler(QObject *parent, const ConnectionHandler::project_initializer &project_init,  uint16_t port) :
        QObject(parent),
        project_init_callback(project_init)
{
    register_messages();

    if (!this->server.listen(QHostAddress::LocalHost, port)) {
        std::cout << "Problem to listen on port " << port << ":"
                << this->server.errorString().toStdString() << "\n";
        // TODO: Terminate horribly!
    }
    connect(&this->server, SIGNAL(newConnection()),
            this, SLOT(onNewConnection()));
    std::cout << "Listening on port " << port << "\n";


    // TODO: Timer to remove pending connections and unresolved requests
}

ConnectionHandler::~ConnectionHandler() {

}

void ConnectionHandler::send(RequestMessage &message,
    const std::string &method,
    request_callback_t callback) {
    this->connections.remove_if([](const std::unique_ptr<Connection> &c) {
        return !c || c->is_done();
    });

    if (this->connections.empty()) {
        return;
    }

    RequestId id;
    id.type = RequestId::AUTO_INCREMENT;

    assert(this->connections.front());
    for (auto &conn : this->connections) {
        if(conn) {
            conn->send(message, method, id, callback);
        } else {
            std::cout << "There is buggy information in the connections queue\n";
        }
    }
}


void ConnectionHandler::onNewConnection() {
    QTcpSocket *clientSocket = this->server.nextPendingConnection();

    this->connections.emplace_back(std::make_unique<TCPLSPConnection>(this, clientSocket, this->project_init_callback()));

    connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));


    std::cout << "Received connection from " << this->connections.back()->peerName()  << "\n";
}

void ConnectionHandler::onSocketStateChanged(QAbstractSocket::SocketState socketState) {
    if (socketState == QAbstractSocket::UnconnectedState) {
        this->connections.remove_if([](const std::unique_ptr<Connection> &c) {
            if (c && !c->is_done())
                std::cout << "closing connection to " << c->peerName() << "\n";
            return !c || c->is_done();
        });
    }
}


template <>
bool decode_env::declare_field(JSONObject &object, RequestId &target, const FieldNameType &field);

void ConnectionHandler::handle_message(const QString &buffer, Connection *conn) {
    // Convert to json and construct the message from it
    RequestId id;
    std::string method;

    try {
        decode_env env(buffer, storage_direction::READ);
        auto root = env.document.object();
        {
            EncapsulatedObjectRef wrapper(root, storage_direction::READ);
            env.declare_field_default(wrapper, id, "id", {});

            env.declare_field_default(wrapper, method, "method", {});
            if (method.empty()) {
                // when the method is empty, this might be a hint for a response mesasge?
                if (wrapper.ref().find("result") != wrapper.ref().end() ||
                        wrapper.ref().find("error") != wrapper.ref().end()) {


                    ResponseMessage msg(wrapper.ref());
                    msg.id = id;
                    {
                        EncapsulatedChildObjectRef wrapper(root, "params", storage_direction::READ);
                        env.declare_field(wrapper, msg, "");
                    }
                    conn->handle_pending_response(msg);
                    return;
                } else {
                    std::cout << "ERROR: No Method!\n";
                    conn->send(ResponseError(ErrorCode::InvalidRequest, "No Method given"), id);
                    return;
                }
            }
        }

        std::cout << "Handling Message [id " << id.value()  << "] with method " << method << "\n";
        auto it = this->typemap.find(method);
        if (it == this->typemap.end()) {
            std::cerr << "Not defined method requested " << method << "\n";
            conn->send(ResponseError(ErrorCode::MethodNotFound, std::string("Method [") + method + "] not implemented"), id);
            return;
        } else {
            auto decoded_msg = it->second(env);
            decoded_msg->process(conn, conn->active_project.get(), id);
        }
    }
    catch (std::unique_ptr<ResponseMessage> &msg) {
        std::cout << "Caught response message\n";
        conn->send(*msg, id);
    }
    catch (std::unique_ptr<ResponseError> &msg) {
        std::cout << "Caught error ptr message: " << msg->message << "\n";
        conn->send(*msg, id);
    }
    catch (ResponseError &msg) {
        std::cout << "Caught error message: " << msg.message << "\n";
        conn->send(msg, id);
    }
    catch(std::exception &err) {
        std::cerr << "Caught std::exception during message handling: " << err.what() << "\n";
        conn->send(ResponseError(ErrorCode::InternalError, err.what()), {});
    }
    /*catch(...) {
        conn->send(ResponseError(ErrorCode::InternalError, "Unspecified internal error"));
    }*/
}
