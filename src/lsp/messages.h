#pragma once

#include "lsp.h"
#include "project.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>

#include <istream>
#include <memory>
#include <string>
#include <iostream>

class Connection;
struct decode_env;
class ResponseMessage;
class RequestMessage;

enum class storage_direction {
    READ, WRITE
};

using FieldNameType = QString;

/**
 * This class helps to avoid the inconvienience that we cannot construct Qt JSON Object trees from the bottom up.
 * They have to be constructed from leaf to root, so we create an interface how we insert a child into the tree
 * after it has been fully constructed
 */
class EncapsulatedObjectRef {
protected:
    QJsonObject &object;
    const storage_direction direction;

public:
    EncapsulatedObjectRef(QJsonObject &object, const storage_direction direction) :
        object(object), direction(direction) {}

    EncapsulatedObjectRef(EncapsulatedObjectRef &&rhs) :
        object(rhs.object), direction(rhs.direction) {}

    virtual ~EncapsulatedObjectRef() {}

    virtual QJsonObject *operator->() { return &ref(); }
    virtual QJsonObject *operator*() { return &ref(); }
    virtual QJsonValueRef operator[](const FieldNameType &t) { return this->ref()[t]; }

    virtual operator QJsonObject &() { return this->ref(); }

    virtual QJsonObject &ref() { return this->object; }

    virtual std::string toStdString() {
        return QJsonDocument(ref()).toJson(QJsonDocument::Compact).toStdString();
    }
};


/**
 * Just like EncapsulatedObjectRef but add the child upon destruction.
 * Use the Object from EncapsulatedObjectRef as parent object
 */
class EncapsulatedChildObjectRef : public EncapsulatedObjectRef {
    const FieldNameType field;
    QJsonObject child;
public:
    EncapsulatedChildObjectRef(QJsonObject &parent, const FieldNameType &field, const storage_direction dir) :
        EncapsulatedObjectRef(parent, dir),
        field(field),
        child((dir==storage_direction::READ)? parent[field].toObject() : QJsonObject{})
    {}

    EncapsulatedChildObjectRef(EncapsulatedChildObjectRef &&rhs) :
        EncapsulatedObjectRef(std::move(rhs)),
        field(std::move(rhs.field)),
        child(std::move(rhs.child))
    {}

    QJsonObject &ref() { return this->child; }

    virtual ~EncapsulatedChildObjectRef() {
        if (this->direction == storage_direction::WRITE && !this->child.empty()) { //isNull
            this->object[this->field] = this->child;
        }
    }
};


using JSONObject = EncapsulatedObjectRef;

struct decode_env {
    const storage_direction dir;
    QJsonDocument document;

    decode_env(const QByteArray &, storage_direction dir=storage_direction::READ);
    decode_env(storage_direction dir);

    void store(QByteArray *, ResponseMessage &);
    void store(QByteArray *, RequestMessage &);

    template<typename value_type>
    bool declare_field(JSONObject &parent, value_type &dst, const FieldNameType &field) {
        //assert(parent.isObject());
        if (this->dir == storage_direction::READ)  {
            auto object = parent.ref();
            auto it = object.find(field);
            if (it != object.end()) {
                if constexpr (std::is_integral<value_type>::value) {
                    dst = it->toInt();
                } else if constexpr(std::is_floating_point<value_type>::value) {
                    dst = it->toDouble();
                } else if constexpr(std::is_same<value_type, bool>::value) {
                    dst = it->toBool();
                } else if constexpr(std::is_convertible<value_type, std::string>::value) {
                    dst = it->toString().toStdString();
                } else if constexpr(std::is_convertible<value_type, QString>::value) {
                    dst = it->toString();
                } else {
                    std::cerr << "Trying to decode unknown type\n";
                    printf("%i<- If this is part of your error message, you have messed up something in the decoding if your message - maybe you have forgotton to declare it as MAKE_DECODEABLE?", dst);
                    //static_assert(std::is_same<value_type, bool>::value);
                }
                return true;
            } else {
                return false;
            }
        } else {
            if constexpr(std::is_convertible<value_type, std::string>::value) {
                parent[field] = QString::fromStdString(dst);
            } else {
                parent[field] = dst;
            }
            return true;
        }
    }

    template <typename value_type>
    bool declare_field_optional(JSONObject &object, OptionalType<value_type> &target, const FieldNameType &field) {
        bool retval = false;
        if (this->dir == storage_direction::READ) {
            if (object.ref().find(field) != object.ref().end()) {
                value_type t;
                retval = declare_field(object, t, field);
                target = t;
            }
        } else if (target) {
            // Only store the field into the json if the optional is set
            value_type t = target.value();
            retval = declare_field(object, t, field);
        } else {
            return false;
        }
        return retval;
    }

    template <typename value_type>
    bool declare_field_array(JSONObject &parent, std::vector<value_type> &target, const FieldNameType &field) {
        if (this->dir == storage_direction::READ) {
            target.clear();
            auto field_it = parent->find(field);
            if (field_it == parent->end()) {
                return false;
            }
            if (!field_it->isArray()) {
                // TODO How can I throw a Error Result here?
                assert(field_it->isArray());
                return false;
            }
            QJsonArray array = field_it->toArray();
            target.reserve(array.size());
            for (const auto array_it : array) {
                value_type t;
                {
                    QJsonObject obj = array_it.toObject();
                    JSONObject wrapper(obj, this->dir);
                    declare_field(wrapper, t, "");
                }
                target.emplace_back(std::move(t));
            }
            if (array.empty()) return false;
        } else {
            QJsonArray array;
            for (auto &it : target) {
                QJsonObject dst;
                {
                    JSONObject wrapper(dst, this->dir);
                    declare_field(wrapper, it, "");
                }
                array.append(dst);
            }
            parent[field] = array;
        }
        return true;
    }

    template<typename value_type>
    bool declare_field_default(JSONObject &object, value_type &dst, const FieldNameType &field, const value_type &dflt) {
        if (!this->declare_field(object, dst, field)) {
            dst = dflt;
            return false;
        }
        return true;
    }

    EncapsulatedChildObjectRef start_object(JSONObject &parent, const FieldNameType &field) {
        return EncapsulatedChildObjectRef(parent, field, this->dir);
    }
};

#define MAKE_DECODEABLE \
virtual void decode(decode_env &env, JSONObject &object, const FieldNameType &field) { env.declare_field(object, *this, field); } \

#define MESSAGE_CLASS(MESSAGETYPE) \
struct MESSAGETYPE; \
template<> \
bool decode_env::declare_field(JSONObject &object, MESSAGETYPE &target, const FieldNameType &field); \
struct MESSAGETYPE





struct RequestMessage {
    RequestMessage() {
        this->id.type = RequestId::AUTO_INCREMENT;
    }
    RequestId id;
    std::string method;

    virtual void process(Connection *conn, project *project, const RequestId &id) = 0;
    virtual void decode(decode_env &env, JSONObject &object, const FieldNameType &field) = 0;

    virtual ~RequestMessage() {}
};
template<>
bool decode_env::declare_field(JSONObject &object, RequestMessage &target, const FieldNameType &field);

// Base Class for Results
MESSAGE_CLASS(ResponseResult) {
    //MAKE_DECODEABLE;
    virtual void decode(decode_env &env, JSONObject &object, const FieldNameType &field) = 0;

    virtual ~ResponseResult() {}
};


MESSAGE_CLASS(ResponseError) {
    MAKE_DECODEABLE;
    ResponseError() {}
    ResponseError(ErrorCode err, const std::string &msg) :
        code(err), message(msg) {}

    ErrorCode code;
    std::string message;

    virtual ~ResponseError() {}
};

class ResponseMessage;
template<>
bool decode_env::declare_field(JSONObject &object, ResponseMessage &target, const FieldNameType &field);

MESSAGE_CLASS(ResponseMessage) {
    MAKE_DECODEABLE;

    // Ctor which takes ownership of the result
    ResponseMessage(ResponseResult *_r) : result(_r), use_result(_r == nullptr), delete_result(_r != nullptr) {}
    // Ctor which does NOT take take ownership of result
    ResponseMessage(ResponseResult &_r) : result(&_r), use_result(true), delete_result(false) {}
    // An incoming message can not directly be mapped to a type - so we store it as JSON
    ResponseMessage(const QJsonObject &obj) : result(nullptr), raw_result(obj), use_result(false), delete_result(false) {}

    RequestId id;
    ResponseResult *result;
    QJsonObject raw_result;
    OptionalType<ResponseError> error;

    const bool use_result = true;
    const bool delete_result;
    virtual ~ResponseMessage() {
        if (delete_result) delete(result);
    }

};


///////////////////////////////////////////////////////////
// Begin Interaction Messages
///////////////////////////////////////////////////////////
MESSAGE_CLASS(InitializeRequest) : public RequestMessage {
    MAKE_DECODEABLE;
    virtual void process(Connection *, project *, const RequestId &id);

    std::string rootUri;
    std::string rootPath;

    // Not used, here for completion
    // Config config;
    // ClientCap capabilities;

    std::vector<WorkspaceFolder> workspaceFolders;
};

MESSAGE_CLASS(ServerCapabilities) {
    MAKE_DECODEABLE;
};

MESSAGE_CLASS(InitializeResult) : public ResponseResult {
    MAKE_DECODEABLE;

    ServerCapabilities capabilities;
};

MESSAGE_CLASS(InitializedNotifiy) : public RequestMessage {
    MAKE_DECODEABLE;
    virtual void process(Connection *, project *, const RequestId &){};
};

MESSAGE_CLASS(ShutdownRequest) : public RequestMessage {
    MAKE_DECODEABLE;

    virtual void process(Connection *, project *, const RequestId &id);
};

MESSAGE_CLASS(ExitRequest) : public RequestMessage {
    MAKE_DECODEABLE;

    virtual void process(Connection *, project *, const RequestId &id);
};


///////////////////////////////////////////////////////////
// LSP Messages based on capabilities
/// capability: hoverProvider
MESSAGE_CLASS(TextDocumentPositionParams) : public RequestMessage {
    MAKE_DECODEABLE;

    Position position;
    TextDocumentIdentifier textDocument;
};

/// capability: textDocumentSync
MESSAGE_CLASS(DidOpenTextDocument) : public RequestMessage {
    MAKE_DECODEABLE;

    TextDocumentItem textDocument;
    virtual void process(Connection *, project *, const RequestId &id);
};

MESSAGE_CLASS(DidChangeTextDocument) : public RequestMessage {
    MAKE_DECODEABLE;

    TextDocumentItem textDocument;

    // Todo handle lists?
    TextDocumentContentChangeEvent contentChanges;

    virtual void process(Connection *, project *, const RequestId &id);
};

MESSAGE_CLASS(DidCloseTextDocument) : public RequestMessage {
    MAKE_DECODEABLE;

    TextDocumentIdentifier textDocument;
    virtual void process(Connection *, project *, const RequestId &id);
};

/// capability: hoverProvider
MESSAGE_CLASS(TextDocumentHover) : public TextDocumentPositionParams {
    MAKE_DECODEABLE;

    virtual void process(Connection *, project *, const RequestId &id);
};

MESSAGE_CLASS(HoverResponse) : public ResponseResult {
    MAKE_DECODEABLE;

    std::string contents;
    lsRange range;
};

MESSAGE_CLASS(Diagnostic) {
    MAKE_DECODEABLE;

    lsRange range;
    int severity;
    std::string message;
    // And many more...
};

MESSAGE_CLASS(PublishDiagnosticsParams) : RequestMessage {
    MAKE_DECODEABLE;

    DocumentUri uri;
    OptionalType<int> version;
    std::vector<Diagnostic> diagnostics;
};

// client capability: window.showDocument
MESSAGE_CLASS(ShowDocumentParams) : public RequestMessage {
    MAKE_DECODEABLE;

    DocumentUri uri;
    OptionalType<bool> external;
    OptionalType<bool> takeFocus;
    OptionalType<lsRange> selection;

    virtual void process(Connection *, project *, const RequestId &){ assert(false); };
};

///////////////////////////////////////////////////////////
// OpenSCAD extensions
///////////////////////////////////////////////////////////
MESSAGE_CLASS(OpenSCADRender) : public RequestMessage {
    MAKE_DECODEABLE;
    DocumentUri uri;

    // load (if needed) and start the rendering of the given document
    virtual void process(Connection *, project *, const RequestId &id);
};


#undef MESSAGE_CLASS
#undef MAKE_DECODEABLE
