#pragma once

#include "SourceFile.h"

namespace CommentParser {

shared_ptr<Expression> parser(const char *text);
void collectParameters(const std::string& fulltext, SourceFile *root_file);

}
