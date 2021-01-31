#include "lsp/messages.h"
#include "lsp/connection.h"
#include "lsp/project.h"

#include <iostream>

#define UNUSED(x) (void)(x)

void InitializeRequest::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(proj);
    InitializeResult msg;
    std::cout << "Processing InitializeRequest\n";
    // TODO fill in the initialize Result (Capabilities are automatically encoded)

    // TODO initialize the project from the initalization parameters

    conn->send(msg, id);
}

void ShutdownRequest::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(proj);
    UNUSED(conn);
    UNUSED(id);
    // TODO : Shutdown response
}

void ExitRequest::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(proj);
    UNUSED(id);
    // TODO do i need a shutdown response?
    conn->close();
}


void DidOpenTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    // Called when a document is opened in the editor
    proj->open_files.emplace_back(textDocument);

    if (id.is_set()) {
        conn->send(SuccessResponse(true), id);
    }
}

void DidChangeTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    // Called when a document is opened
    std::cout << "Changed Text document with new contents: " << this->textDocument.text << "\n\n";

    auto file = proj->getFile(textDocument.uri);
    if (file) {
        file->document = this->textDocument; // move assign and overwrite
    }
    if (id.is_set()) {
        conn->send(SuccessResponse(true), id);
    }
}

void DidCloseTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(conn);

    std::cout << "Closed Text document " << this->textDocument.uri.getPath() << "\n\n";

    proj->open_files.remove_if([this](const auto &item) { \
        return item.document.uri == this->textDocument.uri;
    });
    if (id.is_set()) {
        conn->send(SuccessResponse(true), id);
    }
}
