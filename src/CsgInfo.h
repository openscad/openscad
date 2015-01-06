#pragma once

#include "OffscreenView.h"
#include "csgterm.h"
#include "Tree.h"
#include "GeometryEvaluator.h"
#include "CSGTermEvaluator.h"
#include "csgtermnormalizer.h"
#include "rendersettings.h"
#include "printutils.h"

class CsgInfo
{
public:
    CsgInfo() : glview(NULL), root_chain(NULL), highlights_chain(NULL), background_chain(NULL), progress_function(NULL)
	{
		normalizelimit = RenderSettings::inst()->openCSGTermLimit;
	}
	OffscreenView *glview;
	shared_ptr<CSGTerm> root_norm_term;    // Normalized CSG products
	class CSGChain *root_chain;
	std::vector<shared_ptr<CSGTerm> > highlight_terms;
	CSGChain *highlights_chain;
	std::vector<shared_ptr<CSGTerm> > background_terms;
	CSGChain *background_chain;
	int normalizelimit;

	void (*progress_function)();
	void call_progress_function()
	{
		if (progress_function) progress_function();
	}

	bool compile_chains( const Tree &tree )
	{
		const AbstractNode *root_node = tree.root();
		GeometryEvaluator geomevaluator(tree);
		CSGTermEvaluator evaluator(tree, &geomevaluator);
		boost::shared_ptr<CSGTerm> root_raw_term = evaluator.evaluateCSGTerm( *root_node, this->highlight_terms, this->background_terms );

		PRINT("Compiling design (CSG Products normalization)...");
		call_progress_function();
		CSGTermNormalizer normalizer( normalizelimit );
		if (root_raw_term) {
			this->root_norm_term = normalizer.normalize(root_raw_term);
			if (this->root_norm_term) {
				this->root_chain = new CSGChain();
				this->root_chain->import(this->root_norm_term);
				PRINTB("Normalized CSG tree has %d elements", int(this->root_chain->objects.size()));
			}
			else {
				this->root_chain = NULL;
				PRINT("WARNING: CSG normalization resulted in an empty tree");
				call_progress_function();
			}
		}

		if (this->highlight_terms.size() > 0) {
			PRINTB("Compiling highlights (%i CSG Trees)...", this->highlight_terms.size() );
			call_progress_function();
			this->highlights_chain = new CSGChain();
			for (unsigned int i = 0; i < this->highlight_terms.size(); i++) {
				this->highlight_terms[i] = normalizer.normalize(this->highlight_terms[i]);
				this->highlights_chain->import(this->highlight_terms[i]);
			}
		}

		if (this->background_terms.size() > 0) {
			PRINTB("Compiling background (%i CSG Trees)...", this->background_terms.size());
			call_progress_function();
			this->background_chain = new CSGChain();
			for (unsigned int i = 0; i < this->background_terms.size(); i++) {
				this->background_terms[i] = normalizer.normalize(this->background_terms[i]);
				this->background_chain->import(this->background_terms[i]);
			}
		}
		return true;
	}
};
