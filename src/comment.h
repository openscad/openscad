#ifndef COMMENT_H
#define COMMENT_H

#include "expression.h"
#include"Assignment.h"
#include "FileModule.h"
#include <string>
#include<vector>

using namespace std;

extern AssignmentList * parser(const char *text);

struct GroupInfo{
    string commentString;
    int lineNo;
};

typedef vector <GroupInfo> GroupList;

GroupList collectGroups(string fulltext);
string getParameter(string fulltext, int loc);
string getDescription(string fulltext, int loc);
void addParameter(const char *fulltext, class FileModule *root_module);

#endif // COMMENT_H
