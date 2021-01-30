#include "lsp/language_server_interface.h"
#include "lsp/connection_handler.h"
#include "lsp/connection.h"
#include "lsp/messages.h"
#include "lsp/project.h"
#include "MainWindow.h"

LanguageServerInterface::LanguageServerInterface(MainWindow *mainWindow, int port) :
        port(port),
        mainWindow(mainWindow)
{

    // Connect signals
    connect(mainWindow, SIGNAL(externallySetCursor(QString, int, int)),
            this, SLOT(sendCursor(QString, int, int)));

    // Connect slots
    connect(this, SIGNAL(viewModePreview()),
            mainWindow, SLOT(viewModePreview()));

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
    std::cout << "Starting handler \n";
    this->handler = std::make_unique<ConnectionHandler>(this,
        [this]() {return init_project();},
        port);
}

std::unique_ptr<project> LanguageServerInterface::init_project() {
    auto new_project = std::make_unique<project>();

    new_project->interface = this;
    // Add more initialization magic here - if needed

    // Guaranteed NRVO is comes with C++20! - then we wont need the move any more
    return std::move(new_project);
}


void LanguageServerInterface::sendCursor(QString file, int line, int column) {
    assert(handler.get());
    std::cout << "Sending window/showDocument to " << file.toStdString() << "\n";
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

