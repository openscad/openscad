#include "connection_handler.h"
#include "connection.h"
#include "lsp.h"
#include "messages.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <assert.h>

#include <functional>
#include <iostream>
#include <utility>                                                  // for move

#define UNUSED(x) (void)(x)

/////////////////////////////////////////////////////////////////////
// LSP Basic Structures
/////////////////////////////////////////////////////////////////////
template <>
bool decode_env::declare_field(JSONObject &object, RequestId &target, const FieldNameType &field) {
    auto data = object.ref()[field];
    if (dir == storage_direction::READ) {
        if (data.type() == QJsonValue::String) {
            declare_field(object, target.value_str, field);
            target.type = RequestId::STRING;
        } else if (data.type() == QJsonValue::Double) {
            declare_field(object, target.value_int, field);
            target.type = RequestId::INT;
        }
    } else {
        switch(target.type) {
        case RequestId::STRING:
            declare_field(object, target.value_str, field);
            break;
        case RequestId::INT:
            declare_field(object, target.value_int, field);
            break;
        case RequestId::AUTO_INCREMENT:
            assert("Auto Increment fields should never be encoded - set them before encoding.");
            break;
        case RequestId::UNSET:
            break;
        }
    }
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &object, ErrorCode &errorcode, const FieldNameType &field) {
    int interror = static_cast<int>(errorcode);
    declare_field(object, interror, field);
    if (this->dir == storage_direction::WRITE) {
        errorcode = static_cast<ErrorCode>(interror);
    }
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &object, DocumentUri &target, const FieldNameType &field) {
    return declare_field(object, target.raw_uri, field);
}

template <>
bool decode_env::declare_field(JSONObject &parent, Position &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.line, "line");
    declare_field(object, target.character, "character");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, lsRange &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.start, "start");
    declare_field(object, target.end, "end");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, lsLocation &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.uri, "uri");
    declare_field(object, target.range, "range");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, LocationLink &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.targetUri, "targetUri");
    declare_field(object, target.targetRange, "targetRange");
    declare_field(object, target.targetSelectionRange, "targetSelectionRange");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &object, SymbolKind &target, const FieldNameType &field) {
    uint8_t symbol = static_cast<uint8_t>(target);
    declare_field(object, symbol, field);
    if (this->dir == storage_direction::WRITE) {
        target = static_cast<SymbolKind>(symbol);
    }
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, SymbolInformation &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.name, "name");
    declare_field(object, target.kind, "kind");
    declare_field(object, target.location, "location");
    declare_field_optional(object, target.containerName, "containerName");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, TextDocumentIdentifier &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.uri, "uri");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, VersionedTextDocumentIdentifier &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.uri, "uri");
    declare_field_optional(object, target.version, "version");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, TextEdit &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.range, "range");
    declare_field(object, target.newText, "newText");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, TextDocumentItem &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.uri, "uri");
    declare_field(object, target.languageId, "languageId");
    declare_field(object, target.version, "version");
    declare_field(object, target.text, "text");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, TextDocumentContentChangeEvent &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    //declare_field_optional(object, target.range, "range");
    //declare_field_optional(object, target.rangeLength, "rangeLength");
    declare_field(object, target.text, "text");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, WorkDoneProgress &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.kind, "kind");
    declare_field_optional(object, target.title, "title");
    declare_field_optional(object, target.message, "message");
    declare_field_optional(object, target.percentage, "percentage");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, WorkDoneProgressParam &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.token, "token");
    declare_field(object, target.value, "value");
    return true;
}

template <>
bool decode_env::declare_field(JSONObject &parent, WorkspaceFolder &target, const FieldNameType &field) {
    auto object = start_object(parent, field);
    declare_field(object, target.uri, "uri");
    declare_field(object, target.name, "name");
    return true;
}

//////////////////////////////////////////////////////////////////////
// LSP Base Protocol
//////////////////////////////////////////////////////////////////////

template<>
bool decode_env::declare_field(JSONObject &parent, ResponseError &target, const FieldNameType &) {
    declare_field(parent, target.code, "code");
    declare_field(parent, target.message, "message");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, ResponseResult &target, const FieldNameType &field) {
    target.decode(*this, object, field); // For Polymorphism
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, RequestMessage &target, const FieldNameType &) {
    std::string jsonprocversion = "2.0";
    declare_field(object, jsonprocversion, "jsonrpc");
    if (this->dir == storage_direction::READ || target.id.is_set()) {
        declare_field(object, target.id, "id");
    }
    declare_field(object, target.method, "method");

    auto child = start_object(object, "params");
    target.decode(*this, child, "");  // For Polymorphism
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, ResponseMessage &target, const FieldNameType &) {
    std::string jsonprocversion = "2.0";
    declare_field(object, jsonprocversion, "jsonrpc");
    declare_field(object, target.id, "id");

    if (this->dir == storage_direction::READ) {
        //target.use_result = false;
        //target.raw_result = object.ref(); // (already done during construction)

        if (object.ref().find("error") != object.ref().end()) {
            auto errorchild = start_object(object, "error");
            declare_field_optional(errorchild, target.error, "error");
        }

    } else {
        if (target.use_result && target.result) {
//            assert(target.result);
            auto resultchild = start_object(object, "result");
            target.result->decode(*this, resultchild, "result");
        }
        auto errorchild = start_object(object, "error");
        declare_field_optional(errorchild, target.error, "error");
    }

    if (!target.error && !(target.result || !target.use_result)) {
        std::cerr << "Having a message with neither error nor result\n";
    }

    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, SuccessResponse &target, const FieldNameType &) {
    declare_field(object, target.success, "success");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, InitializeRequest &target, const FieldNameType &) {
    // TODO

    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, InitializeResult &target, const FieldNameType &) {
    declare_field(object, target.capabilities, "capabilities");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, ServerCapabilities &, const FieldNameType &field) {
    assert(this->dir != storage_direction::READ); // The following assignment code does not allow reading!


    object[field] = QJsonObject {
        //{"hoverProvider", true}, // Future
        {"textDocumentSync", QJsonObject {
                {"openClose", true },
                {"change", 1 }, // None = 0, Full = 1, Incremental = 2
            },
        },
        // {"implementationProvider", true }, // "Go To implementation"
        {"window", QJsonObject {
                {"showDocument", QJsonObject {
                        { "support", true } // This allows click to code
                    },
                }
            },
        },
    };
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, InitializedNotifiy &target, const FieldNameType &) {
    // Does not have fields
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &, ShutdownRequest &, const FieldNameType &) {
    // Does not have fields
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &, ExitRequest &, const FieldNameType &) {
    // Does not have fields
    return true;
}

///////////////////////////////////////////////////////////
// LSP General Messages
///////////////////////////////////////////////////////////
template<>
bool decode_env::declare_field(JSONObject &object, MessageType &target, const FieldNameType &field) {
    return declare_field(object, (int&)target, field);
}

template<>
bool decode_env::declare_field(JSONObject &object, DidOpenTextDocument &target, const FieldNameType &) {
    declare_field(object, target.textDocument, "textDocument");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, DidChangeTextDocument &target, const FieldNameType &) {
    declare_field(object, target.textDocument, "textDocument");
    declare_field(object, target.contentChanges, "contentChanges");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, DidCloseTextDocument &target, const FieldNameType &) {
    declare_field(object, target.textDocument, "textDocument");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, TextDocumentPositionParams &target, const FieldNameType &) {
    declare_field(object, target.textDocument, "textDocument");
    declare_field(object, target.position, "position");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, LogMessageNotification &target, const FieldNameType &) {
    declare_field(object, target.type, "type");
    declare_field(object, target.message, "message");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, ShowDocumentParams &target, const FieldNameType &) {
    declare_field(object, target.uri, "uri");
    declare_field_optional(object, target.external, "external");
    declare_field_optional(object, target.takeFocus, "takeFocus");
    declare_field_optional(object, target.selection, "selection");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, Diagnostic &target, const FieldNameType &field) {
    //auto object = start_object(parent, field);
    declare_field(object, target.range, "range");
    declare_field(object, target.severity, "severity");
    declare_field_optional(object, target.code, "code");
    declare_field_optional(object, target.source, "source");
    declare_field(object, target.message, "message");

    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, PublishDiagnosticsParams &target, const FieldNameType &) {
    declare_field(object, target.uri, "uri");
    declare_field_optional(object, target.version, "version");
    declare_field_array(object, target.diagnostics, "diagnostics");
    return true;
}

template<>
bool decode_env::declare_field(JSONObject &object, ShowMessageParams &target, const FieldNameType &) {
    declare_field(object, target.type, "type");
    declare_field(object, target.message, "message");
    return true;
}


template<>
bool decode_env::declare_field(JSONObject &object, ImplementationRequest &target, const FieldNameType &field) {
    return declare_field(object, (TextDocumentPositionParams &)target, field);
}



///////////////////////////////////////////////////////////
// OpenSCAD extensions
///////////////////////////////////////////////////////////
template<>
bool decode_env::declare_field(JSONObject &object, OpenSCADRender &target, const FieldNameType &uri) {
    declare_field(object, target.uri, "uri");
    return true;
}


///////////////////////////////////////////////////////////
// Management logic
///////////////////////////////////////////////////////////

/**
 * message register has to be defined here, in order for the env.declare_field<> template overloads
 * for the given message type to be defined.
 * This is implemented as a macro cascade to allow the output of "textDocument/hover --> TextDocumentHover"
 * upon startup.
 */
void ConnectionHandler::register_messages() {
    std::cerr << "Method mapping:\n";

    // This has to be a macro for the "symbol to string conversion" lovelyness
    #define MAP(method, messagetype) do {\
        this->typemap.emplace(method, [](decode_env &env)->std::unique_ptr<RequestMessage> { \
            static_assert(std::is_base_of<RequestMessage, messagetype>::value, "Can only <MAP> RequestMessage types to requests"); \
            auto resp = std::make_unique<messagetype>(); \
            auto root = env.document.object(); \
            { \
            EncapsulatedChildObjectRef wrapper(root, "params", storage_direction::READ); \
            env.declare_field(wrapper, *resp, ""); \
            } \
            return resp; \
        }); \
        std::cerr << "\t" << method << " \t --> " << #messagetype "\n"; \
    } while (0)

    // Define Messages here
    MAP("initialize", InitializeRequest);
    MAP("initialized", InitializedNotifiy);
    MAP("shutdown", ShutdownRequest);
    MAP("textDocument/didOpen", DidOpenTextDocument);
    MAP("textDocument/didChange", DidChangeTextDocument);
    MAP("textDocument/didClose", DidCloseTextDocument);
    MAP("textDocument/implementation", ImplementationRequest);
    //MAP("textDocument/hover", TextDocumentHover);

    MAP("$openscad/render", OpenSCADRender);



    #undef MAP
}


decode_env::decode_env(const QString &buffer, storage_direction dir) :
        dir(dir)
{
    QJsonParseError err;
    this->document = QJsonDocument::fromJson(buffer.toUtf8(), &err);

    if (this->document.isNull()) {
        ResponseError msg(ErrorCode::InvalidRequest,
            std::string("JSON Parse Error at offset: ") + std::to_string(err.offset) + ": " + err.errorString().toStdString());
        throw msg;
    }
}


decode_env::decode_env(storage_direction dir) :
        dir(dir)
{
    assert(dir == storage_direction::WRITE);
}

void decode_env::store(QByteArray *buffer, ResponseMessage &msg) {
    QJsonObject root;
    {
        EncapsulatedObjectRef wrapper(root, storage_direction::WRITE);
        this->declare_field(wrapper, msg, "");
    }
    this->document.setObject(root);
    buffer->append(this->document.toJson(QJsonDocument::JsonFormat::Compact));
}

void decode_env::store(QByteArray *buffer, RequestMessage &msg) {
    QJsonObject root;
    {
        EncapsulatedObjectRef wrapper(root, storage_direction::WRITE);
        this->declare_field(wrapper, msg, "");
    }
    this->document.setObject(root);
    buffer->append(this->document.toJson(QJsonDocument::JsonFormat::Compact));
}
