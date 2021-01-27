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
    UNUSED(proj);
    UNUSED(conn);
    // Called when a document is opened
    std::cout << "\nOpened Text document with contents: " << this->textDocument.text << "\n\n";
}

void DidChangeTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    // Called when a document is opened
    std::cout << "Changed Text document with new contents: " << this->textDocument.text << "\n\n";
}

void DidCloseTextDocument::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(proj);
    UNUSED(conn);
    // Called when a document is opened
    std::cout << "Closed Text document " << this->textDocument.uri.getPath() << "\n\n";
}

void TextDocumentHover::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(proj);
    // Called when a document is opened
    std::cout << "Hover over : " << this->textDocument.uri.getPath() << " at " << this->position.line << ":"<< this->position.character << "\n";

    HoverResponse hover;
    hover.contents = "Hello VSCode! I Am Alive, you are at line " + std::to_string(this->position.line);
    hover.range.start = this->position;
    hover.range.end = this->position;

    conn->send(hover, id);
}

///////////////////////////////////////////////////////////
// OpenSCAD Extensions
///////////////////////////////////////////////////////////

void OpenSCADRender::process(Connection *conn, project *proj, const RequestId &id) {
    UNUSED(proj);
    std::cout << "Starting rendering\n";
}

