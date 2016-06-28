#include "comment.h"

void addparameter(const char *fulltext, class FileModule *root_module){

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
        name = getParameter(std::string(fulltext),loc-1);
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
    bool check=true;
    for(;startText<comment.length()-1;startText++){
        
        if(comment[startText]=='\"' || comment[startText]=='\''){
            check=!check;
        }
        if( comment[startText]== '/' && comment[startText+1]=='/'  && check){
            break;
        }
       
    }
    
    
    if(startText+2>comment.length()){
        return "";
    }
    return comment.substr(startText+2);
    
}

