#include "lsp/project.h"


OptionalType<openFile &> project::getFile(const DocumentUri &uri) {
    for (auto &file : this->open_files) {
        if (file.document.uri == uri)
            return file;
    }

    return {};
}