#include "parameterextractor.h"

#include "modcontext.h"

ParameterExtractor::ParameterExtractor()
{
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

        //need to improve structure
        if(entries.find(assignment.name) == entries.end()){
            entries[assignment.name] = entryObject;
        }else{
            if(*entryObject==*entries[assignment.name]){
                entryObject=entries[assignment.name];
           }else{
                entries[assignment.name] = entryObject;
            }
        }

        entryObject->set=true;
    }
    connectWidget();
}
