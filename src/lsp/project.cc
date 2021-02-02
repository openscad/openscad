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

    Connection *const conn;

};

OptionalType<openFile &> project::getFile(const DocumentUri &uri) {
    for (auto &file : this->open_files) {
        if (file.document.uri == uri)
            return file;
    }

    return {};
}


openFile::~openFile() {
    delete rootNode;
    delete rootModule;
}

static void consoleOutput(const Message &msg,void *userdata) {
    auto ctx = (LogContext*)userdata;

    std::cout << msg.msg << "\n";

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
    ctx->conn->send(logmsg, "window/logMessage", {}, Connection::no_response_expected);
}

static void errorlogOutput(const Message &msg, void *userdata) {
    // We have to keep track of all possible occurring errors, which will be sent
    // all at once after we are done parsing and rendering
    auto ctx = (LogContext*)userdata;
    // send as diagnostic message
    std::cout << "ERROR: " << msg.msg << "\n";

    Diagnostic diag;
    diag.range.start.line = msg.loc.firstLine() > 0 ? msg.loc.firstLine() - 1 : 0;
    diag.range.start.character = msg.loc.firstColumn() > 0 ? msg.loc.firstColumn() - 1 : 0;

    if (msg.loc.lastLine() == 0 || msg.loc.lastLine() < msg.loc.firstLine()) {
        // full line == till beginning of next line
        diag.range.end.line = msg.loc.firstLine() - 1;
        diag.range.end.character = std::numeric_limits<int>::max();
    } else {
        diag.range.end.line = msg.loc.lastLine() > 0 ? msg.loc.lastLine() -1 : 0;
        diag.range.end.line = msg.loc.lastColumn() > 0 ? msg.loc.lastColumn() - 1 : 0;
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
        return msg.loc.isNone() || DocumentUri::fromPath(msg.loc.fileName()) == diag.uri;
    });
    if (it == ctx->diagnostics.end()) {
        it = ctx->diagnostics.insert(ctx->diagnostics.end(), PublishDiagnosticsParams());
        it->uri = DocumentUri::fromPath(msg.loc.isNone()?"" : msg.loc.fileName());
    }
    it->diagnostics.push_back(diag);
}



void openFile::update(Connection *conn) {
    LogContext ctx = { { {} }, conn };

    std::cout << "Updating document...\n";
    // grab all the logging data
    set_output_handler(consoleOutput, errorlogOutput, &ctx);
	bool parse_result = parse(this->rootModule, this->document.text.c_str(), this->document.uri.getPath().c_str(), this->document.uri.getPath().c_str(), false);

    if (parse_result && this->rootModule) {
        // parse successful - create the modules
        this->rootInst = ModuleInstantiation("group");

        //this->top_ctx = Context::create<BuiltinContext>();
		ContextHandle<FileContext> filectx{Context::create<FileContext>(this->top_ctx.ctx)};
		top_ctx->setDocumentPath(this->document.uri.getPath());
        this->rootNode = this->rootModule->instantiateWithFileContext(filectx.ctx, &this->rootInst, nullptr);
        //LOG(message_group::Echo, Location::NONE, "", "Updated");
        this->tree.setRoot(this->root_node);
    } else {
        // parse failed - try to get some error log?
        // we do have parser_error_pos as the character offset (qscintillaeditor might help convert it?)
    }
    std::cout << "Sending diagnostics...\n";

    for (auto &diag : ctx.diagnostics) {
        if (diag.uri.raw_uri.empty()) {
            diag.uri = this->document.uri;
        }
        conn->send(diag, "textDocument/publishDiagnostics", {}, Connection::no_response_expected);
    }
}