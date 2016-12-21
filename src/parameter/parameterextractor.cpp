#include "parameterextractor.h"
#include "modcontext.h"

ParameterExtractor::ParameterExtractor()
{
  resetPara = false;
}

ParameterExtractor::~ParameterExtractor()
{
}

void ParameterExtractor::applyParameters(FileModule *fileModule)
{
  if (!fileModule) return;

  for (auto &assignment : fileModule->scope.assignments) {
    auto entry = entries.find(assignment.name);
    if (entry != entries.end()) {
      entry->second->applyParameter(assignment);
      entry->second->set = false;
    }
  }
}

void ParameterExtractor::setParameters(const FileModule* module)
{
  if (!module) return;

  ModuleContext ctx;

  ParameterPos.clear();
  for (auto &assignment : module->scope.assignments) {
    const Annotation *param = assignment.annotation("Parameter");
    if (!param) continue;

    const ValuePtr defaultValue = assignment.expr->evaluate(&ctx);
    if (defaultValue->type() == Value::UNDEFINED) continue;

    ParameterObject *entryObject = new ParameterObject();
    entryObject->setAssignment(&ctx, &assignment, defaultValue);

    //check whether object exist or not previously
    if (entries.find(assignment.name) == entries.end() || resetPara) {

      //if object doen't exist
      //or we have reset Parameters then add new entry
      entries[assignment.name] = entryObject;
    } else {
      //if entry object is already exist we check if its modified
      //or not
      if (*entryObject == *entries[assignment.name]) {
        //if entry is not modified then we don't add new entry
        entryObject = entries[assignment.name];
      } else {
        //if entry is modified then we add new entry
        entries[assignment.name] = entryObject;
      }
    }
    entryObject->set = true;
    ParameterPos.push_back(assignment.name);
  }
  connectWidget();
  this->resetPara = false;
}
