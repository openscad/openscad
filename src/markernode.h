#pragma once

#include "node.h"
#include <string>

class MarkerNode : public AbstractNode
{
    public:
        VISITABLE();

        MarkerNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
        virtual std::string name() const { return "marker"; }
        virtual std::string toString() const;

        std::string value;
};
