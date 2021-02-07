#include "lsp/language_server_interface.h"
#include "lsp/connection_handler.h"
#include "lsp/connection.h"
#include "lsp/messages.h"
#include "lsp/project.h"
#include "MainWindow.h"
#include "AutoUpdater.h"

#include <QTcpSocket>

LanguageServerInterface::LanguageServerInterface(MainWindow *mainWindow, const LanguageServerInterface::Settings &settings) :
        settings(settings),
        mainWindow(mainWindow)
{
    // Connect signals
    connect(mainWindow, SIGNAL(externallySetCursor(QString, int, int)),
            this, SLOT(sendCursor(QString, int, int)));

    // Connect slots
    connect(this, SIGNAL(viewModePreview(const std::string &)),
            mainWindow, SLOT(compileDocument(const std::string &)));

    mainWindow->consoleOutput(Message(
        std::string("The language server is enabled in this window on TCP port ") + std::to_string(settings.port) + ".",
        Location::NONE, "", message_group::None));

#ifndef LSP_ON_MAINTHREAD
    connect(&workerthread, SIGNAL(started()), this, SLOT(start()));
    this->moveToThread(&workerthread);
    workerthread.start();
#else
    this->start();
#endif
}

LanguageServerInterface::~LanguageServerInterface() {
    this->stop();
}

void LanguageServerInterface::stop() {
#ifndef LSP_ON_MAINTHREAD
    workerthread.exit(0);
#endif
}

void LanguageServerInterface::start() {

    int port = (settings.mode == OperationMode::LISTEN)?settings.port : 0;

    this->handler = std::make_unique<ConnectionHandler>(this,
        [this]() {return init_project();},
        port);

    switch(settings.mode) {
    case OperationMode::CONNECT:
        {
            // TODO this leaks memory!
            QTcpSocket *socket = new QTcpSocket(this);
            socket->connectToHost("localhost", settings.port);
            this->handler->add_connection(
                std::make_unique<TCPLSPConnection>(this->handler.get(), socket, this->handler->make_project()));

        }
        break;
    case OperationMode::STDIO:
        this->handler->add_connection(
            std::make_unique<StdioLSPConnection>(this->handler.get(), this->handler->make_project()));
        break;
    case OperationMode::LISTEN:
    case OperationMode::NONE:
        break;
    }

    // Things interfering with LSP
    mainWindow->designActionAutoReload->setChecked(false);
    mainWindow->autoReloadSet(false);
    mainWindow->windowActionHideEditor->setChecked(true);
    mainWindow->hideEditor(true);
}

std::unique_ptr<project> LanguageServerInterface::init_project() {
    auto new_project = std::make_unique<project>();

    new_project->lspinterface = this;
    new_project->mainWindow = mainWindow;
    // Add more initialization magic here - if needed

    return new_project;
}


void LanguageServerInterface::sendCursor(QString file, int line, int column) {
    assert(handler.get());
    std::cerr << "Sending window/showDocument to " << file.toStdString() << "\n";
    ShowDocumentParams showdoc;
    showdoc.uri = DocumentUri::fromPath(file.toStdString());
    showdoc.external = false;
    showdoc.takeFocus = true;
    showdoc.selection = lsRange();
    showdoc.selection->start.line = line - 1;
    showdoc.selection->start.character = column - 1;
    showdoc.selection->end = showdoc.selection->start;
    handler->send(showdoc, "window/showDocument", &Connection::no_response_expected);
}
