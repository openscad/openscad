#ifndef TEXTNODE_H_
#define TEXTNODE_H_

#include "node.h"
#include "visitor.h"
#include "value.h"

#include "FreetypeRenderer.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
        TextNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}

        virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
        
	virtual std::string toString() const;
	virtual std::string name() const { return "text"; }

	virtual PolySet *evaluate_polyset(class PolySetEvaluator *) const;
        
        virtual FreetypeRenderer::Params get_params() const;
private:
        FreetypeRenderer::Params params;
        friend TextModule;
};

#endif
