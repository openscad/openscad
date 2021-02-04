#pragma once

#include <QThread>
#include <QObject>

#include <string>
#include <memory>

#define LSP_ON_MAINTHREAD

class MainWindow;
class ConnectionHandler;
class project;
/**
 * To interact with the openscad language server you should use this interface.
 *
 * Upon creation it will start a language server on the given port listening on localhost for incoming language clients.
 * These clients will send certain commands described by the slots, and have to be connected.
 */
class LanguageServerInterface : public QObject {
    Q_OBJECT

public:
    enum class OperationMode {
        NONE, STDIO, LISTEN, CONNECT
    };
    struct Settings {
        OperationMode mode = OperationMode::NONE;
        int port = 0;
    };

    // This constructor will connect all the neccessary slots and signals with the main window
    LanguageServerInterface(MainWindow *mainWindow, const Settings &settings);
    virtual ~LanguageServerInterface();

    void stop();

    std::unique_ptr<project> init_project();

    void requestPreview() {
        emit viewModePreview();
    }

signals:
    // Request a preview
    void viewModePreview();

    void starting();

public slots:
    void sendCursor(QString file, int line, int column);

private slots:
    void start();

private:
    Settings settings;
    std::unique_ptr<ConnectionHandler> handler;
    MainWindow *mainWindow;

#ifndef LSP_ON_MAINTHREAD
    QThread workerthread;
#endif
};

