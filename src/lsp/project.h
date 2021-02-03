#pragma once

#include "lsp/lsp.h"
#include "node.h"
#include "context.h"
#include "builtincontext.h"
#include "ModuleInstantiation.h"

#include <list>

class FileModule;
class AbstractNode;


class Connection;
class LanguageServerInterface;
class MainWindow;
struct LogContext;

class openFile {
public:
    openFile(const TextDocumentItem &doc);
    ~openFile();

    TextDocumentItem document;

    // TODO Add Tree of Contextes to evaluate what is ok where
    AbstractNode *rootNode = nullptr;
    FileModule *rootModule = nullptr;

    // parse the connection and update rootNode. Do not send to a renderer!
    void update(Connection *conn);

private:
    ModuleInstantiation rootInst;
	ContextHandle<BuiltinContext> top_ctx;

    std::unique_ptr<LogContext> log_ctx;

};

struct project {
    // Interface towards OpenSCAD
    LanguageServerInterface *lspinterface = nullptr;
    MainWindow *mainWindow;

    // Interface towards the editor
    WorkspaceFolder workspace;
    std::list<openFile> open_files;
    // store project status information

    OptionalType<openFile &> getFile(const DocumentUri &uri);
};
