#include "comment.h"

/*
    Insert Parameters in AST of given scad file
    in form of annotations
     */
void addParameter(const char *fulltext, class FileModule *root_module){
    
    //Getting list of all group names in the file
    GroupList groupList = collectGroups(std::string(fulltext));

    for (AssignmentList::iterator it = root_module->scope.assignments.begin();it != root_module->scope.assignments.end();it++) {

        //get loaction of assignment node 
        const Location locate=(*it).location();
        int loc =locate.firstLine();
        if(!it->expr.get()->isLiteral()){
            continue;
        }
        // makeing list to add annotations
        AnnotationList *annotationList = new AnnotationList();
        AssignmentList *assignmentList;
         
        //extracting the parameter 
        string name = getParameter(std::string(fulltext),loc);        
            //getting the node for parameter annnotataion
            assignmentList=parser(name.c_str());
            if(assignmentList==NULL){
                assignmentList=new AssignmentList();
                Expression *expr;
                expr=new Literal(ValuePtr(std::string("")));
                Assignment *assignment;
                assignment=new Assignment("", shared_ptr<Expression>(expr));
                assignmentList->push_back(*assignment);   
            }
            const Annotation *Parameter;
            Parameter=Annotation::create("Parameter",*assignmentList);

            // adding parameter to the list
            annotationList->push_back(*Parameter);
        
        
        //extracting the description 
        name = getDescription(std::string(fulltext),loc-1);
        if(name!= ""){ 
        
            //creating node for description
            assignmentList=new AssignmentList();
            Expression *expr;
            
            expr=new Literal(ValuePtr(std::string(name.c_str())));
            
            Assignment *assignment;
            
            assignment=new Assignment("", shared_ptr<Expression>(expr));
            assignmentList->push_back(*assignment);
                
            const Annotation * Description;    
            Description=Annotation::create("Description", *assignmentList);
            annotationList->push_back(*Description);
        }
        
        // Look for the group to which the given assignment belong
        int i=0;        
        for( ;i<groupList.size() && groupList[i].lineNo<loc;i++){            
         }
         i--; 
         
         if(i>=0){
            //creating node for description
            assignmentList=new AssignmentList();
            Expression *expr;
            
            expr=new Literal(ValuePtr(groupList[i].commentString));
            
            Assignment *assignment;
            
            assignment=new Assignment("", shared_ptr<Expression>(expr));
            assignmentList->push_back(*assignment);
                
            const Annotation * Description;    
            Description=Annotation::create("Group", *assignmentList);
            annotationList->push_back(*Description);
        }

        (*it).add_annotations(annotationList);
        
    }
}
    
/* 
    gives the string parameter for given 
    Assignment
    */
string getParameter(string fulltext, int loc){
    
    unsigned int start = 0;
    if( loc<1){
        return "";
    }
    for(; start<fulltext.length() ; start++){
       
        if(fulltext[start]=='\n')
            loc--;
       
        if(loc<=1)
            break;    
    }
    
    int end=start+1;
    while(fulltext[end]!='\n'){
        end++;
        
    }
    
    string comment = fulltext.substr(start,end-start);

    unsigned int startText=0;
    int noOfSemicolon=0;
    bool isComment=true;
    for(;startText<comment.length()-1;startText++){
        
        if(comment[startText]=='\"'){
            isComment=!isComment;
        }
        if( comment[startText]== '/' && comment[startText+1]=='/'  && isComment){
            break;
        }
        if(comment[startText]== ';' && isComment && noOfSemicolon>0){
            
            return "";
        }
        if(comment[startText]== ';' && isComment){
            
            noOfSemicolon++;
        }
       
    }
    
    
    if(startText+2>comment.length()){
        return "";
    }
    return comment.substr(startText+2);
    
}

/* 
    Gives the string of Description for given 
    Assignment
    */
    
string getDescription(string fulltext, int loc){
    
    unsigned int start = 0;
    if( loc<1){
        return "";
    }
    
    for(; start<fulltext.length() ; start++){
       
       if(loc<=1)
            break;  
       
       if(fulltext[start]=='\n')
            loc--;  
            
    }

    //not a valid description
    if(fulltext[start] != '/' || fulltext[start+1] != '/'){ 
        return "";
    }
    
    //Jump over the two forward slashes
    start=start+2;  
    
    //Jump over all the spaces
    while(fulltext[start]==' ' || fulltext[start]=='\t'){  
        start++;
    }
    string retString = "";
    
    //go till the end of the line
    while(fulltext[start]!='\n'){  
        
        //replace // with space
        if(fulltext[start] == '/' && fulltext[start+1] == '/'){ 
            
                retString += " ";
                start++;
        }else{
        
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
GroupList collectGroups(string fulltext){
    
    GroupList groupList; //container of all group names
    int lineNo=1; // tracks line number
    bool isComment=true; //check if its string or comment
    
    //iterate through whole scad file
    for(unsigned int i=0; i<fulltext.length(); i++){
        
        //increase line number
        if(fulltext[i]=='\n'){
            lineNo++;
            continue;
        }
        
        //start or end of string negate the checkpoint
        if(fulltext[i]=='\"'){
            isComment=!isComment;
            continue;
        }
        
         if(fulltext[i]=='/' && fulltext[i+1]=='/'){
            i++;
            while(fulltext[i]!='\n'){
                i++;
            }
            lineNo++;
            continue;
        }
        
        //start of multi line comment if check is true
        if(fulltext[i]=='/' && fulltext[i+1]=='*' && isComment){
            
            //store comment
            string comment;
            i=i+2;
            
            // till */ every character is comment
            while(fulltext[i]!='*' && fulltext[i+1]!='/'){
                comment+=fulltext[i];
                i++;
            }
            
            //store info related to group
            GroupInfo groupInfo;
            string finalGroupName; //Final group name
            string groupName; //group name
            bool isGroupName=false;
            for(unsigned int it=0; it<comment.length();it++){
            
                //Start of Group Name
                if(comment[it]=='[' ){
                    isGroupName=true;
                    continue;
                }
                
                //End of Group Name
                 if(comment[it]==']' ){
                    isGroupName=false;
                    
                    //Setting of group name 
                    if(!finalGroupName.empty()){
                        finalGroupName=finalGroupName+"-"+groupName;
                    }else{
                        finalGroupName=finalGroupName+groupName;
                    }
                    groupName.clear();
                    continue;
                }
                
                //collect characters if it belong to group name
                if(isGroupName){
                    groupName+=comment[it];
                }                
            }
            
            groupInfo.commentString=finalGroupName;
            groupInfo.lineNo=lineNo;
            groupList.push_back(groupInfo);
        }
    }
    return groupList;
}
 

