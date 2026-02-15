#include "genlang.h"

std::vector<std::string> mapping_name;
std::vector<std::string> mapping_code;
std::vector<int> mapping_level;

std::vector<std::shared_ptr<AbstractNode>> shows;
std::shared_ptr<AbstractNode> genlang_result_node = nullptr;
void show_final(void)
{
  mapping_name.clear();
  mapping_code.clear();
  mapping_level.clear();
  if (shows.size() == 1) genlang_result_node = shows[0];
  else {
    DECLARE_INSTANCE();
    genlang_result_node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);
    genlang_result_node->children = shows;
  }
  shows.clear();
}
