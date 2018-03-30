#include "comment.h"
#include "expression.h"
#include "annotation.h"
#include <string>
#include <vector>
#include <boost/range/adaptor/reversed.hpp>
// gcc 4.8 and earlier have issues with std::regex see
// #2291 and https://stackoverflow.com/questions/12530406/is-gcc-4-8-or-earlier-buggy-about-regular-expressions
// therefor, we use boost::regex
#include <boost/regex.hpp>

struct GroupInfo {
	std::string commentString;
	std::string expr;
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
	std::string finalGroupName;

	boost::regex regexName("\\[(.*?)\\]");
	boost::match_results<std::string::const_iterator>  match;
	while(boost::regex_search(comment, match, regexName)) {
		std::string groupName = match[1].str();
		if (finalGroupName.empty()) {
			finalGroupName = groupName;
		} else {
			finalGroupName = finalGroupName + "-" + groupName;
		}
		groupName.clear();
		comment = match.suffix();
	}

	boost::regex regexShowIf("show_if\\((.*?)\\)");
	boost::regex_search(comment, match, regexShowIf);
	std::string expr = match[1].str();

	groupInfo.commentString = finalGroupName;
	groupInfo.lineNo = lineNo;
	groupInfo.expr = expr;
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
	static auto EmptyStringLiteral(std::make_shared<Literal>(ValuePtr(std::string(""))));

	// Get all groups of parameters
	GroupList groupList = collectGroups(std::string(fulltext));
	int parseTill=getLineToStop(fulltext);
	// Extract parameters for all literal assignments
	for (auto &assignment : root_module->scope.assignments) {
		if (!assignment.expr.get()->isLiteral()) continue; // Only consider literals

		// get location of assignment node
		int firstLine = assignment.location().firstLine();
		if(firstLine>=parseTill || (
			assignment.location().fileName() != "" &&
			assignment.location().fileName() != root_module->getFilename() &&
			assignment.location().fileName() != root_module->getFullpath()
			)) {
			continue;
		}
		// making list to add annotations
		auto annotationList = new AnnotationList();
 
		// Extracting the parameter comment
		std::string comment = getComment(std::string(fulltext), firstLine);
		// getting the node for parameter annotation
		shared_ptr<Expression> params = CommentParser::parser(comment.c_str());
		if (!params) {
			params = EmptyStringLiteral;
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

		// Look for the group to which the given assignment belongs
		for (const auto &groupInfo :boost::adaptors::reverse(groupList)){
			if (groupInfo.lineNo < firstLine) {
				//creating node for description
				shared_ptr<Expression> commentStr(new Literal(ValuePtr(groupInfo.commentString)));
				annotationList->push_back(Annotation("Group", commentStr));
				break;
			}
		}
		assignment.addAnnotations(annotationList);
	}

	shared_ptr<Vector> Vector1(new Vector(Location::NONE));
	for (const auto &groupInfo :groupList){
		if(groupInfo.expr != ""){
			auto* Vec = new Vector(Location::NONE);
			auto* commentStr = new Literal(ValuePtr(groupInfo.commentString));
			auto* expr = new Literal(ValuePtr(groupInfo.expr));

			Vec->push_back(commentStr);
			Vec->push_back(expr);
			Vector1->push_back(Vec);
		}
	}

	auto annotationList = new AnnotationList();
	annotationList->push_back(Annotation("Groups", Vector1));
	root_module->addAnnotations(annotationList);
}
