#include "parameterextractor.h"

#include "module.h"
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
        entry_map_t::iterator entry = entries.find((*it).first);
        if (entry == entries.end()) {
            continue;
        }
            (*entry).second->applyParameter(&(*it));
            (*entry).second->set=false;

    }
}

void ParameterExtractor::setParameters(const Module *module)
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

        const ValuePtr defaultValue = assignment.second.get()->evaluate(&ctx);
        if (defaultValue->type() == Value::UNDEFINED) {
            continue;
        }

        ParameterObject *entryObject = new ParameterObject();
        entryObject->setAssignment(&ctx, &assignment, defaultValue);

        //need to improve structure
        if(entries.find(assignment.first) == entries.end()){
            entries[assignment.first] = entryObject;
        }else{
            if(*entryObject==*entries[assignment.first]){
                entryObject=entries[assignment.first];
           }else{
                entries[assignment.first] = entryObject;
            }
        }

        entryObject->set=true;
    }
    connectWidget();
}
