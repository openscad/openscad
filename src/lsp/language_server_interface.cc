#include "lsp/language_server_interface.h"
#include "lsp/connection_handler.h"
#include "lsp/connection.h"
#include "lsp/messages.h"
#include "MainWindow.h"

LanguageServerInterface::LanguageServerInterface(int port, MainWindow *mainWindow) :
        port(port)
{

    std::cout << "Connecting Signals\n";
    // Connect signals
    connect(mainWindow, SIGNAL(externallySetCursor(QString, int, int)),
            this, SLOT(sendCursor(QString, int, int)));

    // Connect slots
    connect(this, SIGNAL(viewModePreview()),
            mainWindow, SLOT(viewModePreview()));
    std::cout << "Done with Signal\n";

    connect(&workerthread, SIGNAL(started()), this, SLOT(start()));

    this->moveToThread(&workerthread);
    workerthread.start();
}

LanguageServerInterface::~LanguageServerInterface() {
    this->stop();
}

void LanguageServerInterface::stop() {
    workerthread.exit(0);
}

void LanguageServerInterface::start() {
    std::cout << "Starting handler \n";
    this->handler = std::make_unique<ConnectionHandler>(this, port);
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
    handler->send(showdoc, "window/showDocument", &Connection::no_reponse_expected);
}

