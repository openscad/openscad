#include "lsp/project.h"
#include "lsp/connection.h"
#include "lsp/messages.h"

#include "openscad.h"           // for parse()
#include "parsersettings.h"     // for parser_error_pos
#include "printutils.h"         // for set_output_handler()
#include "FileModule.h"
#include "modcontext.h"         // for FileContext

struct LogContext {
    std::list<PublishDiagnosticsParams> diagnostics;

    Connection *conn;

};

OptionalType<openFile &> project::getFile(const DocumentUri &uri) {
    for (auto &file : this->open_files) {
        if (file.document.uri == uri)
            return file;
    }

    return {};
}

openFile::openFile(const TextDocumentItem &doc) :
    document(doc),
    rootInst("group"),
    top_ctx(Context::create<BuiltinContext>()),
    log_ctx(std::make_unique<LogContext>())
{}

openFile::~openFile() {
    delete rootNode;
    delete rootModule;
}

static void consoleOutput(const Message &msg,void *userdata) {
    auto ctx = (LogContext*)userdata;

    std::cerr << "MSG" << msg.msg << "\n";

    LogMessageNotification logmsg;
    switch(msg.group) {
        case message_group::Error:
        case message_group::Export_Error:
        case message_group::Parser_Error:
            logmsg.type = MessageType::Error;
            break;
        case message_group::Warning:
        case message_group::Font_Warning:
        case message_group::Export_Warning:
            logmsg.type = MessageType::Warning;
            break;
        case message_group::None:
        case message_group::Echo:
            logmsg.type = MessageType::Info;
            break;
        case message_group::Trace:
        case message_group::Deprecated:
            logmsg.type = MessageType::Log;
            break;
        case message_group::UI_Warning:
        case message_group::UI_Error:
            // We ignore UI errors!
            return;
    }

    // we ignore msg.loc for now?

    logmsg.message = msg.msg;

    // send as log message
    // TODO: Currently these are sent twice, so the LSP gets a log and diagnostics...
    //ctx->conn->send(logmsg, "window/logMessage", {}, Connection::no_response_expected);
}

static void errorlogOutput(const Message &msg, void *userdata) {
    // We have to keep track of all possible occurring errors, which will be sent
    // all at once after we are done parsing and rendering
    auto ctx = (LogContext*)userdata;
    // send as diagnostic message
    //std::cerr << "ERROR: " << msg.msg << "\n";

    Diagnostic diag;
    diag.range.start.line = msg.loc.firstLine() > 0 ? msg.loc.firstLine() - 1 : 0;
    diag.range.start.character = msg.loc.firstColumn() > 0 ? msg.loc.firstColumn() - 1 : 0;

    if (msg.loc.lastLine() == 0 || msg.loc.lastLine() < msg.loc.firstLine()) {
        // full line == till beginning of next line
        diag.range.end.line = msg.loc.firstLine() - 1 + 1;
        diag.range.end.character = 0;
    } else {
        diag.range.end.line = msg.loc.lastLine() > 0 ? msg.loc.lastLine() -1 : 0;
        diag.range.end.character = msg.loc.lastColumn() > 0 ? msg.loc.lastColumn() - 1 : 0;
    }
    diag.source = "openscad";

    switch(msg.group) {
        case message_group::Error:
        case message_group::Export_Error:
        case message_group::Parser_Error:
            diag.severity = MessageType::Error;
            break;
        case message_group::Warning:
        case message_group::Font_Warning:
        case message_group::Export_Warning:
            diag.severity = MessageType::Warning;
            break;
        case message_group::None:
        case message_group::Echo:
            diag.severity = MessageType::Info;
            break;
        case message_group::Trace:
        case message_group::Deprecated:
            diag.severity = MessageType::Log;
            break;
        case message_group::UI_Warning:
        case message_group::UI_Error:
            // We ignore UI errors!
            return;
    }

    diag.message = msg.msg;

    // Find if there is already a diagnostic for this file
    auto it = std::find_if(ctx->diagnostics.begin(), ctx->diagnostics.end(), [&](const PublishDiagnosticsParams &diag) {
        return !msg.loc.isNone() && DocumentUri::fromPath(msg.loc.fileName()) == diag.uri;
    });
    if (it == ctx->diagnostics.end()) {
        it = ctx->diagnostics.insert(ctx->diagnostics.end(), PublishDiagnosticsParams());
        it->uri = DocumentUri::fromPath(msg.loc.isNone()?"" : msg.loc.fileName());
    }
    it->diagnostics.push_back(diag);
}



void openFile::update(Connection *conn) {
    // grab all the logging data
    log_ctx->conn = conn;
    set_output_handler(consoleOutput, errorlogOutput, log_ctx.get());

    for (auto &diagfile : log_ctx->diagnostics) {
        diagfile.diagnostics.clear();
    }

    // ensure that the root document is contained in here - otherwise it might not be sent if there are no errors
    {
        auto it = std::find_if(log_ctx->diagnostics.begin(), log_ctx->diagnostics.end(), [&](const PublishDiagnosticsParams &diag) {
            return this->document.uri == diag.uri;
        });
        if (it == log_ctx->diagnostics.end()) {
            PublishDiagnosticsParams rootDiag;
            rootDiag.uri = this->document.uri;
            log_ctx->diagnostics.push_back(rootDiag);
        }
    }

    delete this->rootModule;
    this->rootModule = nullptr;
    bool parse_result = parse(this->rootModule, this->document.text.c_str(), this->document.uri.getPath().c_str(), this->document.uri.getPath().c_str(), false);
    if (parse_result && this->rootModule) {
        // parse successful - create the modules
        this->rootModule->handleDependencies();
        this->rootInst = ModuleInstantiation("group");

        ContextHandle<FileContext> filectx{Context::create<FileContext>(this->top_ctx.ctx)};
        top_ctx->setDocumentPath(this->document.uri.getPath());
        this->rootNode = this->rootModule->instantiateWithFileContext(filectx.ctx, &this->rootInst, nullptr);
        //LOG(message_group::Echo, Location::NONE, "", "Updated");
    } else {
        // parse failed - try to get some error log?
        // we do have parser_error_pos as the character offset (qscintillaeditor might help convert it?)
    }
    for (auto &diag : log_ctx->diagnostics) {
        // We have the ""-uri - find the "document.uri" element
        if (diag.uri.raw_uri.empty()) {
            auto it = std::find_if(log_ctx->diagnostics.begin(), log_ctx->diagnostics.end(), [&](const PublishDiagnosticsParams &diag) {
                return diag.uri == this->document.uri;
            });
            if (it != log_ctx->diagnostics.end()) {
                it->diagnostics.insert(it->diagnostics.end(), diag.diagnostics.begin(), diag.diagnostics.end());
            }
            break;
        }
    }

    for (auto &diag : log_ctx->diagnostics) {
        if (diag.uri.raw_uri.empty()) {
            // Skip the empty entry
            continue;
        }
        conn->send(diag, "textDocument/publishDiagnostics", {}, Connection::no_response_expected);
    }
}