#pragma once

#include "lsp/lsp.h"
#include "node.h"
#include "context.h"
#include "builtincontext.h"
#include "ModuleInstantiation.h"

#include <list>
#include <QJsonObject>

class FileModule;
class AbstractNode;


class Connection;
class LanguageServerInterface;
class MainWindow;
class ExecuteCommandRequest;
struct LogContext;

class openFile {
public:
    openFile(const TextDocumentItem &doc);

    openFile &operator=(const openFile &rhs) {
        document = rhs.document;
        rootNode = nullptr;
        rootModule = nullptr;
        return *this;
    }
    openFile &operator=(openFile &&rhs) {
        document = std::move(rhs.document);
        rootNode = nullptr;
        rootModule = nullptr;
        return *this;
    }
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
    // Initialisation options
    QJsonObject clientCapabilities;

    // Interface towards OpenSCAD
    LanguageServerInterface *lspinterface = nullptr;
    MainWindow *mainWindow;

    // Interface towards the editor
    WorkspaceFolder workspace;

    // File Management
    std::list<openFile> open_files;

    openFile *getFile(const DocumentUri &uri);

    openFile *openFileFromDisk(const DocumentUri &uri);
};

