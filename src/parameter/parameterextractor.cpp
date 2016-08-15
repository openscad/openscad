#include "parameterextractor.h"

#include "modcontext.h"

ParameterExtractor::ParameterExtractor()
{
    resetPara=false;
}

ParameterExtractor::~ParameterExtractor()
{
}

void ParameterExtractor::applyParameters(FileModule *fileModule)
{
    if (fileModule == NULL) {
        return;
    }

    for (AssignmentList::iterator it = fileModule->scope.assignments.begin();it != fileModule->scope.assignments.end();it++) {
        entry_map_t::iterator entry = entries.find((*it).name);
        if (entry == entries.end()) {
            continue;
        }
            (*entry).second->applyParameter(&(*it));
            (*entry).second->set=false;

    }
}

void ParameterExtractor::setParameters(const FileModule* module)
{
    if (module == NULL) {
        return;
    }

    ModuleContext ctx;

    foreach(Assignment assignment, module->scope.assignments)
    {
        const Annotation *param = assignment.annotation("Parameter");
        if (!param) {
            continue;
        }

        const ValuePtr defaultValue = assignment.expr.get()->evaluate(&ctx);
        if (defaultValue->type() == Value::UNDEFINED) {
            continue;
        }

        ParameterObject *entryObject = new ParameterObject();
        entryObject->setAssignment(&ctx, &assignment, defaultValue);

        //check whether object exist or not previously
        if(entries.find(assignment.name) == entries.end() || resetPara){

            //if object doen't exist
            //or we have reset Parameters then add new entry
            entries[assignment.name] = entryObject;
        }else{

            //if entry object is already exist we check if its modified
            //or not
            if(*entryObject==*entries[assignment.name]){

                //if entry is not modified then we don't add new entry
                entryObject=entries[assignment.name];
           }else{

                //if entry is modified then we add new entry
                entries[assignment.name] = entryObject;
            }
        }
        entryObject->set=true;
    }
    connectWidget();
    this->resetPara=false;
}
