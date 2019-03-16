#include "parameterextractor.h"
#include "modcontext.h"

ParameterExtractor::ParameterExtractor()
{

}

ParameterExtractor::~ParameterExtractor()
{
}

void ParameterExtractor::applyParameters(FileModule *fileModule, entry_map_t& entries)
{
  if (!fileModule) return;

  for (auto &assignment : fileModule->scope.assignments) {
    auto entry = entries.find(assignment->getName());
    if (entry != entries.end()) {
      if (entry->second->groupName != "Hidden") {
        entry->second->applyParameter(assignment);
        entry->second->set = false;
      }
    }
  }
}

void ParameterExtractor::setParameters(const FileModule* module,entry_map_t& entries,std::vector<std::string>& ParameterPos, bool &rebuildParameterWidget)
{
  if (!module) return;

  ContextHandle<Context> ctx{Context::create<Context>()};

  ParameterPos.clear();
  for (auto &assignment : module->scope.assignments) {
    const Annotation *param = assignment->annotation("Parameter");
    if (!param) continue;

    Value defaultValue = assignment->getExpr()->evaluate(ctx.ctx);
    if (defaultValue.type() == Value::Type::UNDEFINED) continue;

    ParameterObject *entryObject = new ParameterObject(ctx.ctx, assignment, std::move(defaultValue));

    //check whether object exist or not previously
    if (entries.find(assignment->getName()) == entries.end()) {
      //if object doesn't exist, add new entry
      entries[assignment->getName()] = entryObject;
      rebuildParameterWidget = true;
    } else {
      //if entry object already exists, we check if its modified
      //or not
      if (*entryObject == *entries[assignment->getName()]) {
        delete entryObject;
        //if entry is not modified, then we don't add new entry
        entryObject = entries[assignment->getName()];
      } else {
        delete entries[assignment->getName()];
        //if entry is modified, then we add new entry
        entries[assignment->getName()] = entryObject;
      }
    }
    entryObject->set = true;
    ParameterPos.push_back(assignment->getName());
  }
}
