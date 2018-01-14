#include "comment.h"
#include "expression.h"
#include "annotation.h"
#include <string>
#include <vector>
#include <boost/range/adaptor/reversed.hpp>

struct GroupInfo {
	std::string commentString;
	int lineNo;
};

typedef std::vector <GroupInfo> GroupList;

/*
  Finds line to break stop parsing parsing parameters

*/ 

static int getLineToStop( const std::string &fulltext){
	int lineNo=1;
	bool inString=false;
	for (unsigned int i=0; i<fulltext.length(); i++) {
	// increase line number
		if (fulltext[i] == '\n') {
			lineNo++;
			continue;
		}

		// skip escaped quotes inside strings
		if (inString && fulltext.compare(i, 2, "\\\"") == 0) {
			i++;
			continue;
		}

		//start or end of string negate the checkpoint
		if (fulltext[i] == '"') {
			inString = !inString;
			continue;
		}

		if (!inString && fulltext.compare(i, 2, "//") == 0) {
			i++;
			while (fulltext[i] != '\n' && i<fulltext.length()) i++;
			lineNo++;
			continue;
		}

		//start of multi line comment if check is true
		if (!inString && fulltext.compare(i, 2, "/*") == 0) {
			i ++;
			if(i<fulltext.length()) {
				i++;
			}
			else {
				continue;
			}
			// till */ every character is comment
			while (fulltext.compare(i, 2, "*/") != 0 && i<fulltext.length()) {
				if(fulltext[i]=='\n'){
					lineNo++;
				}
				i++;
			}
		}

		if (fulltext[i]== '{') {
			return lineNo;
		}
	}
	return lineNo;
}


/* 
  Finds the given line in the given source code text, and
  extracts the comment (excluding the "//" prefix)
*/
static std::string getComment(const std::string &fulltext, int line)
{
	if (line < 1) return "";

	// Locate line
	unsigned int start = 0;
	for (; start<fulltext.length() ; start++) {
		if (line <= 1) break;
		if (fulltext[start] == '\n') line--;
	}

	int end = start + 1;
	while (fulltext[end] != '\n') end++;

	std::string comment = fulltext.substr(start, end - start);

	// Locate comment
	unsigned int startText = 0;
	int noOfSemicolon = 0;
	bool inString = false;
	for (;startText < comment.length() - 1;startText++) {
		if (inString && comment.compare(startText, 2, "\\\"") == 0) {
			startText++;
			continue;
		}
		if (comment[startText] == '"') inString = !inString;
		if (!inString) {
			if (comment.compare(startText, 2, "//") == 0) break;
			if (comment[startText] == ';' && noOfSemicolon > 0) return "";
			if (comment[startText] == ';') noOfSemicolon++;
		}
	}

	if (startText + 2 > comment.length()) return "";

	std::string result = comment.substr(startText + 2);
	return result;
}

/* 
   Extracts a parameter description from comment on the given line.
   Returns description, without any "//"
*/
static std::string getDescription(const std::string &fulltext, int line)
{
	if (line < 1) return "";

	unsigned int start = 0;
	for (; start<fulltext.length() ; start++) {
		if (line <= 1) break;
		if (fulltext[start] == '\n') line--;
	}

	// not a valid description
	if (fulltext.compare(start, 2, "//") != 0) return "";

	// Jump over the two forward slashes
	start = start+2;

	//Jump over all the spaces
	while (fulltext[start] == ' ' || fulltext[start] == '\t') start++;
	std::string retString = "";

	// go till the end of the line
	while (fulltext[start] != '\n') {
		// replace // with space
		if (fulltext.compare(start, 2, "//") == 0) {
			retString += " ";
			start++;
		} else {
			retString += fulltext[start];
		}
		start++;
	}
	return retString;
}

/*
 Create groups by parsing the multi line comment provided
*/
static GroupInfo createGroup(std::string comment,int lineNo)
{
	//store info related to group
	GroupInfo groupInfo;
	std::string finalGroupName; //Final group name
	std::string groupName; //group name
	bool isGroupName = false;
	for (unsigned int it = 0; it < comment.length();it++) {

		//Start of Group Name
		if (comment[it] == '[') {
			isGroupName = true;
			continue;
		}

		//End of Group Name
		if (comment[it] == ']') {
			isGroupName = false;
			//Setting of group name 
			if (!finalGroupName.empty()) {
				finalGroupName = finalGroupName + "-" + groupName;
			} else {
				finalGroupName = finalGroupName + groupName;
			}
			groupName.clear();
			continue;
		}

		//collect characters if it belong to group name
		if (isGroupName) {
			groupName += comment[it];
		}
	}

	groupInfo.commentString = finalGroupName;
	groupInfo.lineNo = lineNo;
	return groupInfo;
}


/*
  This function collect all groups of parameters described in the
  scad file.
*/
static GroupList collectGroups(const std::string &fulltext)
{
	GroupList groupList; // container of all group names
	int lineNo = 1; // tracks line number
	bool inString = false; // check if its string or (line-) comment

	// iterate through whole scad file
	for (unsigned int i=0; i<fulltext.length(); i++) {
		// increase line number
		if (fulltext[i] == '\n') {
			lineNo++;
			continue;
		}

		// skip escaped quotes inside strings
		if (inString && fulltext.compare(i, 2, "\\\"") == 0) {
			i++;
			continue;
		}

		//start or end of string negate the checkpoint
		if (fulltext[i] == '"') {
			inString = !inString;
			continue;
		}

		if (!inString && fulltext.compare(i, 2, "//") == 0) {
			i++;
			while (fulltext[i] != '\n' && i<fulltext.length() ) i++;
			lineNo++;
			continue;
		}

		//start of multi line comment if check is true
		if (!inString && fulltext.compare(i, 2, "/*") == 0) {
			//store comment
			std::string comment;
			i++;
			if(i<fulltext.length()) {
				i++;
			}
			else {
				continue;
			}
			bool isGroup=true;
			// till */ every character is comment
			while (fulltext.compare(i, 2, "*/") != 0 && i<fulltext.length()) {
				if(fulltext[i]=='\n'){
					lineNo++;
					isGroup=false;
				}
				comment += fulltext[i];
				i++;
			}

			if(isGroup)
				groupList.push_back(createGroup(comment,lineNo));
		}
	}
	return groupList;
}



/*!
  Insert Parameters in AST of given scad file
  form of annotations
*/
void CommentParser::collectParameters(const char *fulltext, FileModule *root_module)
{
	// Get all groups of parameters
	GroupList groupList = collectGroups(std::string(fulltext));
	int parseTill=getLineToStop(fulltext);
	// Extract parameters for all literal assignments
	for (auto &assignment : root_module->scope.assignments) {
		if (!assignment.expr.get()->isLiteral()) continue; // Only consider literals

		// get location of assignment node
		int firstLine = assignment.location().firstLine();
		if(firstLine>=parseTill ) continue;

		// making list to add annotations
		AnnotationList *annotationList = new AnnotationList();
 
		// Extracting the parameter comment
		std::string comment = getComment(std::string(fulltext), firstLine);
		// getting the node for parameter annotation
		shared_ptr<Expression> params = CommentParser::parser(comment.c_str());
		if (!params) {
			params = shared_ptr<Expression>(new Literal(ValuePtr(std::string(""))));
		}

		// adding parameter to the list
		annotationList->push_back(Annotation("Parameter", params));

		//extracting the description
		std::string descr = getDescription(std::string(fulltext), firstLine - 1);
		if (descr != "") {
			//creating node for description
			shared_ptr<Expression> expr(new Literal(ValuePtr(std::string(descr.c_str()))));
			annotationList->push_back(Annotation("Description", expr));
		}

		// Look for the group to which the given assignment belong
		for (const auto &groupInfo :boost::adaptors::reverse(groupList)){
			if (groupInfo.lineNo < firstLine) {
				//creating node for description
				shared_ptr<Expression> expr(new Literal(ValuePtr(groupInfo.commentString)));
				annotationList->push_back(Annotation("Group", expr));
				break;
			}
		}
		assignment.addAnnotations(annotationList);
	}
}
