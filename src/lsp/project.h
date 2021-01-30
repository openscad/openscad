#pragma once

#include "lsp/lsp.h"
#include <list>

class LanguageServerInterface;
class MainWindow;

struct openFile {
    openFile(const TextDocumentItem &doc) :
        document(doc)
    {}

    TextDocumentItem document;

    // TODO link the AST tree?
};

struct project {
    // Interface towards OpenSCAD
    LanguageServerInterface *interface = nullptr;
    MainWindow *mainWindow;

    // Interface towards the editor
    WorkspaceFolder workspace;
    std::list<openFile> open_files;
    // store project status information

    OptionalType<openFile &> getFile(const DocumentUri &uri);
};

