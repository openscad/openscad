#pragma once

#include "AST.h"

AbstractNode *transform_tree(AbstractNode *root);

void printTree(const AbstractNode& node, const std::string& indent = "");
