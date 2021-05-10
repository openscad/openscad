#include "parameterextractor.h"

ParameterExtractor::ParameterExtractor()
{

}

ParameterExtractor::~ParameterExtractor()
{
}

void ParameterExtractor::applyParameters(SourceFile *sourceFile, entry_map_t& entries)
{
  if (!sourceFile) return;

  for (auto &assignment : sourceFile->scope.assignments) {
    auto entry = entries.find(assignment->getName());
    if (entry != entries.end()) {
      if (entry->second->groupName != "Hidden") {
        entry->second->applyParameter(assignment);
        entry->second->set = false;
      }
    }
  }
}

void ParameterExtractor::setParameters(const SourceFile* sourceFile,entry_map_t& entries,std::vector<std::string>& ParameterPos, bool &rebuildParameterWidget)
{
  if (!sourceFile) return;

  ParameterPos.clear();
  for (auto &assignment : sourceFile->scope.assignments) {
    const Annotation *param = assignment->annotation("Parameter");
    if (!param) continue;

    Value defaultValue = assignment->getExpr()->evaluateLiteral();
    if (defaultValue.type() == Value::Type::UNDEFINED) continue;

    ParameterObject *entryObject = new ParameterObject(assignment, std::move(defaultValue));

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
