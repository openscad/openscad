#ifndef COMMENT_H
#define COMMENT_H

#include "expression.h"
#include"Assignment.h"
#include "FileModule.h"
#include <string>
#include<vector>

using namespace std;

extern AssignmentList * parser(const char *text);

struct multilineComment{
    string commentString;
    int line;
};

typedef vector <multilineComment> multicomment;

multicomment collectcomment(string fulltext);
string getParameter(string fulltext, int loc);
string getDescription(string fulltext, int loc);
void addparameter(const char *fulltext, class FileModule *root_module);

#endif // COMMENT_H
