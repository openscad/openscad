#include "lsp/messages.h"
#include "lsp/connection.h"
#include "lsp/project.h"
#include "lsp/language_server_interface.h"

#include "MainWindow.h"
#include "node.h"
#include "ModuleInstantiation.h"

/* Redirect the call to the LSP interface, which knows the mainWindow */
void OpenSCADRender::process(Connection *conn, project *proj, const RequestId &id) {
    auto file = proj->getFile(this->uri);

    if (!file) {
        conn->send(ResponseError(ErrorCode::InvalidParams, "File was not opened!"), id);
        return;
    }

    proj->lspinterface->viewModePreview(file->document.text);
    conn->send(SuccessResponse(true), id);
}

void ImplementationRequest::process(Connection *conn, project *proj, const RequestId &id) {
    auto file = proj->getFile(this->textDocument.uri);
    if (!file) {
        conn->send(ResponseError(ErrorCode::InvalidParams, "No associated document found"), id);
        return;
    }

    std::deque<const AbstractNode *> nodes;
    Location loc(
            this->position.line + 1, this->position.character, // start
            this->position.line + 1, this->position.character, // end
            std::make_shared<fs::path>(this->textDocument.uri.getPath()));

    file->rootNode->getNodesByLocation(loc, nodes);

    if (nodes.empty()) {
        conn->send(ResponseError(ErrorCode::InternalError, "Could not find module at position"), id);
        return;
    }

    for (const auto *anode: nodes) {
        // the instantiation location
        auto modinst = anode->modinst;
        if (modinst) {
            // find the module. but where!!!
            // this is the user modules name
            // TODO find the declaration of the module

            // TODO: look at Builtins::keywordList
        }
    }
    conn->send(ResponseError(ErrorCode::ParseError, "Referenced element does not have a module"), id);
}
