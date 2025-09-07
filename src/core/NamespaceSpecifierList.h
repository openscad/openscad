#pragma once

#include <memory>
#include <vector>

class NamespaceSpecifier;

using NamespaceSpecifierList = std::vector<std::shared_ptr<NamespaceSpecifier>>;
