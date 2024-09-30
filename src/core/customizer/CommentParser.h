#pragma once

#include <memory>
#include <string>

#include "core/SourceFile.h"

namespace CommentParser {

std::shared_ptr<Expression> parser(const char *text);
void collectParameters(const std::string& fulltext, SourceFile *root_file);

}
