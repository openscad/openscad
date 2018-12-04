#pragma once

#include "FileModule.h"
#include "Assignment.h"

namespace CommentParser {

	shared_ptr<Expression> parser(const char *text);
	void collectParameters(const std::string& fulltext, FileModule *root_module);

}
