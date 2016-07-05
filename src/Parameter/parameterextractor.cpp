#include "parameterextractor.h"

#include "FileModule.h"
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

        if(entries.find(assignment.name) == entries.end() || !(*entryObject==*entries[assignment.name]) ){
            entries[assignment.name] = entryObject;
        }
        else{
               entryObject=entries[assignment.name];
        }

        entryObject->set=true;
    }
    connectWidget();
}
