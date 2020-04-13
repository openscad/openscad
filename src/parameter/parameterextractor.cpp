#include "parameterextractor.h"
#include "modcontext.h"
#include <memory>
#include "UserModule.h"

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
    auto entry = entries.find(assignment->name);
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
  // std::cout<<"-------------------testing area begins------------------------\n";
	// auto mp = module->scope.astModules;
  // // for(auto i=mp.begin();i!=mp.end();i++)
  // // {
  // //   std::cout<<i->first<<"***\n";
  // //   std::cout<<*(i->second).get().name<<"####\n";
  // // }
  // // for(int i=0;i<mp.size();i++)
  // // {
  // //   std::cout<<mp[i].first<<"***\n";
  // //   std::cout<<**(mp[i].second).location().firstLine()<<"####\n";
  // // }

  // for(auto &module : module->scope.astModules)
  // {
	// 	  std::cout<<module.first<<"***\n";
  //     std::cout<<typeid(module.second).name()<<"###\n";
  //     UserModule temp = *module.second; 
  //     std::cout<<temp.location().firstLine()<<"###\n";
	// }


  // std::cout<<"--------------------testing area ends--------------------------\n";




  for (auto &assignment : module->scope.assignments) {
    const Annotation *param = assignment->annotation("Parameter");
    // std::cout<<param->getName()<<std::endl;
    if (!param) continue;

    const ValuePtr defaultValue = assignment->expr->evaluate(ctx.ctx);
    if (defaultValue->type() == Value::ValueType::UNDEFINED) continue;

    ParameterObject *entryObject = new ParameterObject(ctx.ctx, assignment, defaultValue);

    //check whether object exist or not previously
    if (entries.find(assignment->name) == entries.end()) {
      //if object doesn't exist, add new entry
      entries[assignment->name] = entryObject;
      rebuildParameterWidget = true;
    } else {
      //if entry object already exists, we check if its modified
      //or not
      if (*entryObject == *entries[assignment->name]) {
        delete entryObject;
        //if entry is not modified, then we don't add new entry
        entryObject = entries[assignment->name];
      } else {
        delete entries[assignment->name];
        //if entry is modified, then we add new entry
        entries[assignment->name] = entryObject;
      }
    }
    entryObject->set = true;
    ParameterPos.push_back(assignment->name);
  }
}
