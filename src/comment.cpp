#include "comment.h"

void addparameter(const char *fulltext, class FileModule *root_module){
    
    multicomment group=collectcomment(std::string(fulltext));
    for (AssignmentList::iterator it = root_module->scope.assignments.begin();it != root_module->scope.assignments.end();it++) {

        //get loaction of assignment node 
        const Location locate=(*it).location();
        int loc =locate.firstLine();
        
        // makeing list to add annotations
        AnnotationList *annotationList = new AnnotationList();
         AssignmentList *assignmentList;
         
        //extracting the parameter 
        string name = getParameter(std::string(fulltext),loc);
        if(name!= "" ){
        
            //getting the node for parameter annnotataion
            assignmentList=parser(name.c_str());
            if(assignmentList!=NULL){
                const Annotation *Parameter;
                Parameter=Annotation::create("Parameter",*assignmentList);

                // adding parameter to the list
                annotationList->push_back(*Parameter);
            }
        }
        
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
        
        int i=0;        
        for( ;i<group.size() && group[i].line<loc;i++){
            
         }
         i--; 
         
         if(i>=0){
            //creating node for description
            assignmentList=new AssignmentList();
            Expression *expr;
            
            expr=new Literal(ValuePtr(group[i].commentString));
            
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
    
string getParameter(string fulltext, int loc){
    
    int start = 0;
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

    int startText=0;
    int noOfSemicolon=0;
    bool check=true;
    for(;startText<comment.length()-1;startText++){
        
        if(comment[startText]=='\"' || comment[startText]=='\''){
            check=!check;
        }
        if( comment[startText]== '/' && comment[startText+1]=='/'  && check){
            break;
        }
        if(comment[startText]== ';' && check && noOfSemicolon>0){
            
            return "";
        }
        if(comment[startText]== ';' && check){
            
            noOfSemicolon++;
        }
       
    }
    
    
    if(startText+2>comment.length()){
        return "";
    }
    return comment.substr(startText+2);
    
}

string getDescription(string fulltext, int loc){
    int start = 0;
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
    
    //jump over the two forward slashes
    start=start+2;  
    
    //jump over all the spaces
    while(fulltext[start]==' ' || fulltext[start]=='\t'){  
        start++;
    }
    string retString = "";
    //go till the end of the world, I mean the end of the line
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

multicomment collectcomment(string fulltext){
    
    multicomment multi;
    int line=1;
    bool check=true;
    for(int i=0; i<fulltext.length(); i++){
    
        if(fulltext[i]=='\n'){
            line++;
        }
        if(fulltext[i]=='\"' || fulltext[i]=='\''){
            check=!check;
        }
        
        if(fulltext[i]=='/' && fulltext[i+1]=='*' && check){
            string comment;
            i=i+2;
            while(fulltext[i]!='*' && fulltext[i+1]!='/'){
                comment+=fulltext[i];
                i++;
            }
            multilineComment m;
            string group;
            string commentPart;
            bool bracketCheck=false;
            for(int it=0; it<comment.length();it++){
                if(comment[it]=='[' ){
                    bracketCheck=true;
                    continue;
                }
                 if(comment[it]==']' ){
                    bracketCheck=false;
                    
                    if(!group.empty()){
                        group=group+"-"+commentPart;
                    }else{
                        group=group+commentPart;
                    }
                    commentPart.clear();
                    continue;
                }
                if(bracketCheck){
                    commentPart+=comment[it];
                }                
            }
            
            m.commentString=group;
            m.line=line;
            multi.push_back(m);
        }
    }
    return multi;
}
 

