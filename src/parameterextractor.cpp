#include "parameterextractor.h"

#include "module.h"
#include "modcontext.h"
#include "expression.h"

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
    }
}

void ParameterExtractor::setParameters(const Module *module)
{
    if (module == NULL) {
        return;
    }

    ModuleContext ctx;

    entries.clear();
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
        entries[assignment.first] = entryObject;
    }
    connectWidget();
}
