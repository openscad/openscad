#pragma once

#include "lsp/lsp.h"
#include <deque>

struct openFile {
    DocumentUri uri;
    std::string modified_content;
};

struct project {
    WorkspaceFolder workspace;

    std::deque<openFile> open_files;
    // store project status information
};

