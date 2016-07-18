#include "parameterset.h"
#include "modcontext.h"

ParameterSet::ParameterSet()
{
}
ParameterSet::~ParameterSet()
{
    myfile.close();
}

void ParameterSet::getParameterSet(string filename){

    myfile.open (filename);
    // send your JSON above to the parser below, but populate ss first
    if(myfile.is_open()){
        try{
            pt::read_json(myfile, this->root);
        }
        catch (std::exception const& e){

            std::cerr << e.what() << std::endl;
        }
    }
}

void ParameterSet::applyParameterSet(FileModule *fileModule,string setName)
{
    try{
        if (fileModule == NULL ||root.empty()) {
            return;
        }
        ModuleContext ctx;
        string path="SET."+setName;
        for (AssignmentList::iterator it = fileModule->scope.assignments.begin();it != fileModule->scope.assignments.end();it++) {

            for(pt::ptree::value_type &v : root.get_child(path)){
                if(v.first== (*it).name){
                    Assignment *assignment;
                    assignment=&(*it);
                    const ValuePtr defaultValue = assignment->expr.get()->evaluate(&ctx);
                    if(defaultValue->type()== Value::NUMBER){
                        assignment->expr = shared_ptr<Expression>(new Literal(ValuePtr(v.second.get_value<double>())))  ;
                    }else if(defaultValue->type()== Value::BOOL){
                          assignment->expr = shared_ptr<Expression>(new Literal(ValuePtr(v.second.get_value<bool>())));
                    }else if(defaultValue->type()== Value::VECTOR){

                        AssignmentList *assignmentList;
                        assignmentList=parser(v.second.data().c_str());
                        for(int i=0; i<assignmentList->size(); i++) {
                            assignment=assignmentList[i].data();
                        }
                    }else if(defaultValue->type()== Value::STRING){
                            assignment->expr = shared_ptr<Expression>(new Literal(ValuePtr(v.second.data())));
                    }
                 }
             }
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

