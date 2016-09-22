#ifndef COMMENT_H
#define COMMENT_H

#include "FileModule.h"
#include "Assignment.h"

namespace CommentParser {

	AssignmentList *parser(const char *text);
	void addParameter(const char *fulltext, FileModule *root_module);

}

#endif // COMMENT_H
