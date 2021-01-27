#pragma once

#include <QThread>
#include <QObject>

#include <string>
#include <memory>

class MainWindow;
class ConnectionHandler;
/**
 * To interact with the openscad language server you should use this interface.
 *
 * Upon creation it will start a language server on the given port listening on localhost for incoming language clients.
 * These clients will send certain commands described by the slots, and have to be connected.
 */
class LanguageServerInterface : public QObject {
    Q_OBJECT

public:
    // This constructor will connect all the neccessary slots and signals with the main window
    LanguageServerInterface(int port, MainWindow *mainWindow);

    virtual ~LanguageServerInterface();

    void stop();

signals:
    // Request a preview
    void viewModePreview();

public slots:
    void sendCursor(QString file, int line, int column);

private slots:
    void start();

private:
    int port;
    QThread workerthread;
    std::unique_ptr<ConnectionHandler> handler;
};

