#pragma once

#include "node.h"
#include "visitor.h"
#include <string>

class MarkerNode : public AbstractNode
{
    public:
        MarkerNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
        virtual Response accept(class State &state, Visitor &visitor) const {
            return visitor.visit(state, *this);
        }
        virtual std::string name() const { return "marker"; }
	    virtual std::string toString() const;

        std::string value;
};
