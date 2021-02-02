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

class openFile {
public:
    openFile(const TextDocumentItem &doc) :
        document(doc), rootInst("group"), top_ctx(Context::create<BuiltinContext>())
    {}

    ~openFile();

    TextDocumentItem document;

    // TODO link the AST tree?
    AbstractNode *rootNode = nullptr;
    FileModule *rootModule = nullptr;
    Tree tree;

    // parse the connection and update rootNode. Do not send to a renderer!
    void update(Connection *conn);

private:
    ModuleInstantiation rootInst;
	ContextHandle<BuiltinContext> top_ctx;

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

