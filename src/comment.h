#ifndef COMMENT_H
#define COMMENT_H

#include "expression.h"
#include"Assignment.h"
#include "FileModule.h"
#include <string>

using std::string;
extern AssignmentList * parser(const char *text);

string getParameter(string fulltext, int loc);
void addparameter(const char *fulltext, class FileModule *root_module);

#endif // COMMENT_H
