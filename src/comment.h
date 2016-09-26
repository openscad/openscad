#ifndef COMMENT_H
#define COMMENT_H

#include "FileModule.h"
#include "Assignment.h"

namespace CommentParser {

	shared_ptr<Expression> parser(const char *text);
	void addParameter(const char *fulltext, FileModule *root_module);

}

#endif // COMMENT_H
