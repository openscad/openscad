#include "comment.h"
#include "expression.h"

#include <string>
#include <vector>

struct GroupInfo {
  std::string commentString;
  int lineNo;
};

typedef std::vector <GroupInfo> GroupList;

    
/*! 
  gives the string parameter for given 
  Assignment

  Finds the given line in the given source code text, and
  extracts the comment (excluding the "//" prefix)
*/
static std::string getParameter(std::string fulltext, int line)
{
  if (line < 1) return "";

  // Locate line
  unsigned int start = 0;
  for (; start<fulltext.length() ; start++) {
    if (fulltext[start] == '\n') line--;
    if (line <= 1) break;
  }
    
  int end = start + 1;
  while (fulltext[end] != '\n') end++;
    
  std::string comment = fulltext.substr(start, end - start);

  // Locate comment
  unsigned int startText = 0;
  int noOfSemicolon = 0;
  bool isComment = true;
  for (;startText < comment.length() - 1 ; startText++) {
    if (comment[startText] == '"') isComment = !isComment;
    if (isComment) {
      if (comment.compare(startText, 2, "//") == 0) break;
      if (comment[startText] == ';' && noOfSemicolon > 0) return "";
      if (comment[startText] == ';') noOfSemicolon++;
    }
  }
    
  if (startText + 2 > comment.length()) return "";

  return comment.substr(startText + 2);
}

/* 
   Gives the string of Description for given 
   Assignment
*/
    
static std::string getDescription(std::string fulltext, int loc)
{
  if (loc < 1) return "";

  unsigned int start = 0;
  for (; start<fulltext.length() ; start++) {
    if (loc <= 1) break;  
    if (fulltext[start] == '\n') loc--;
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
  This function collect the list of groups of Parameter decsibred in 
  scad file
*/
static GroupList collectGroups(std::string fulltext)
{
  GroupList groupList; //container of all group names
  int lineNo = 1; // tracks line number
  bool isComment = true; //check if its string or comment
    
  //iterate through whole scad file
  for (unsigned int i=0; i<fulltext.length(); i++) {
    //increase line number
    if (fulltext[i] == '\n') {
      lineNo++;
      continue;
    }
        
    //start or end of string negate the checkpoint
    if (fulltext[i] == '"') {
      isComment = !isComment;
      continue;
    }
        
    if (fulltext.compare(i, 2, "//") == 0) {
      i++;
      while (fulltext[i] != '\n') i++;
      lineNo++;
      continue;
    }
        
    //start of multi line comment if check is true
    if (isComment && fulltext.compare(i, 2, "/*") == 0) {
      //store comment
      std::string comment;
      i += 2;
      // till */ every character is comment
      while (fulltext.compare(i, 2, "*/") != 0) {
        comment += fulltext[i];
        i++;
      }

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
      groupList.push_back(groupInfo);
    }
  }
  return groupList;
}

/*!
  Insert Parameters in AST of given scad file
  form of annotations
*/
void CommentParser::addParameter(const char *fulltext, FileModule *root_module)
{
  // Getting list of all group names in the file
  GroupList groupList = collectGroups(std::string(fulltext));

  for (auto &assignment : root_module->scope.assignments) {
    if (!assignment.expr.get()->isLiteral()) continue; // Only consider literals

    // get location of assignment node 
    int firstLine = assignment.location().firstLine();

    // making list to add annotations
    AnnotationList *annotationList = new AnnotationList();
         
    // extracting the parameter 
    std::string name = getParameter(std::string(fulltext), firstLine);        
    // getting the node for parameter annnotataion
    AssignmentList *assignmentList = CommentParser::parser(name.c_str());
    if (assignmentList == NULL) {
      assignmentList = new AssignmentList();
      Expression *expr = new Literal(ValuePtr(std::string("")));
      Assignment *assignment = new Assignment("", shared_ptr<Expression>(expr));
      assignmentList->push_back(*assignment);   
    }
    const Annotation *parameter = Annotation::create("Parameter",*assignmentList);

    // adding parameter to the list
    annotationList->push_back(*parameter);
        
        
    //extracting the description 
    name = getDescription(std::string(fulltext), firstLine-1);
    if (name != "") {
      //creating node for description
      assignmentList = new AssignmentList();
      Expression *expr = new Literal(ValuePtr(std::string(name.c_str())));
            
      Assignment *assignment = new Assignment("", shared_ptr<Expression>(expr));
      assignmentList->push_back(*assignment);
                
      const Annotation *description = Annotation::create("Description", *assignmentList);
      annotationList->push_back(*description);
    }
        
    // Look for the group to which the given assignment belong
    int i=0;        
    for (;i<groupList.size() && groupList[i].lineNo<firstLine;i++);
    i--; 
         
    if (i >= 0) {
      //creating node for description
      assignmentList = new AssignmentList();
      Expression *expr = new Literal(ValuePtr(groupList[i].commentString));
            
      Assignment *assignment = new Assignment("", shared_ptr<Expression>(expr));
      assignmentList->push_back(*assignment);
                
      const Annotation * description = Annotation::create("Group", *assignmentList);
      annotationList->push_back(*description);
    }
    assignment.add_annotations(annotationList);
  }
}
