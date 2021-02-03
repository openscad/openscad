#include "lsp/messages.h"
#include "lsp/connection.h"
#include "lsp/project.h"

#include <iostream>

#define UNUSED(x) (void)(x)

void InitializeRequest::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(proj);
    InitializeResult msg;
    // TODO fill in the initialize Result (Capabilities are automatically encoded)

    // TODO initialize the project from the initalization parameters

    conn->send(msg, id);
}

void ShutdownRequest::process(Connection *conn, project *proj, const RequestId &id) {
    proj->open_files.clear();
    conn->send(NullResponse(), id);
}

void ExitNotification::process(Connection *conn, project *, const RequestId &) {
    conn->close();
}

void DidOpenTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    // Called when a document is opened in the editor
    proj->open_files.emplace_back(textDocument);
    auto file = proj->getFile(textDocument.uri);

    file->update(conn);
}

void DidChangeTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    if (this->contentChanges.size() == 0) {
        return;
    }

    auto file = proj->getFile(textDocument.uri);
    if (!file) {
        proj->open_files.emplace_back(textDocument);
        file = proj->getFile(textDocument.uri);
    }

    file->document.text = std::move(this->contentChanges[0].text);
    file->update(conn);
}

void DidCloseTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(conn);

    proj->open_files.remove_if([this](const auto &item) { \
        return item.document.uri == this->textDocument.uri;
    });
}
