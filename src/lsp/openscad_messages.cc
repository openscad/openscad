#include "lsp/messages.h"
#include "lsp/connection.h"
#include "lsp/project.h"
#include "lsp/language_server_interface.h"

#include "MainWindow.h"
#include "node.h"

void OpenSCADRender::process(Connection *conn, project *proj, const RequestId &id) {
    std::cout << "Starting rendering\n";

    proj->interface->requestPreview();
}
