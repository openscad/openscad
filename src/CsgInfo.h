#ifndef __CSGINFO_H__
#define __CSGINFO_H__

#include "OffscreenView.h"
#include "csgterm.h"
#include "Tree.h"
#include "CGALEvaluator.h"
#include "CSGTermEvaluator.h"
#include "csgtermnormalizer.h"
#include "rendersettings.h"

class CsgInfo
{
public:
	CsgInfo()
	{
		root_chain = NULL;
		highlights_chain = NULL;
		background_chain = NULL;
		glview = NULL;
	}
	OffscreenView *glview;
	shared_ptr<CSGTerm> root_norm_term;    // Normalized CSG products
	class CSGChain *root_chain;
	std::vector<shared_ptr<CSGTerm> > highlight_terms;
	CSGChain *highlights_chain;
	std::vector<shared_ptr<CSGTerm> > background_terms;
	CSGChain *background_chain;

	bool prep_chains( const Tree &tree )
	{
		const AbstractNode *root_node = tree.root();
		CGALEvaluator cgalevaluator(tree);
		CSGTermEvaluator evaluator(tree, &cgalevaluator.psevaluator);
		boost::shared_ptr<CSGTerm> root_raw_term = evaluator.evaluateCSGTerm( *root_node, this->highlight_terms, this->background_terms );

		if (!root_raw_term) {
			fprintf(stderr, "Error: CSG generation failed! (no top level object found)\n");
			return false;
		}

		// CSG normalization
		CSGTermNormalizer normalizer( RenderSettings::inst()->openCSGTermLimit );
		this->root_norm_term = normalizer.normalize(root_raw_term);
		if (this->root_norm_term) {
			this->root_chain = new CSGChain();
			this->root_chain->import(this->root_norm_term);
			fprintf(stderr, "Normalized CSG tree has %d elements\n", int(this->root_chain->polysets.size()));
		}
		else {
			this->root_chain = NULL;
			fprintf(stderr, "WARNING: CSG normalization resulted in an empty tree\n");
		}

		if (this->highlight_terms.size() > 0) {
			std::cerr << "Compiling highlights (" << this->highlight_terms.size() << " CSG Trees)...\n";

			this->highlights_chain = new CSGChain();
			for (unsigned int i = 0; i < this->highlight_terms.size(); i++) {
				this->highlight_terms[i] = normalizer.normalize(this->highlight_terms[i]);
				this->highlights_chain->import(this->highlight_terms[i]);
			}
		}

		if (this->background_terms.size() > 0) {
			std::cerr << "Compiling background (" << this->background_terms.size() << " CSG Trees)...\n";

			this->background_chain = new CSGChain();
			for (unsigned int i = 0; i < this->background_terms.size(); i++) {
				this->background_terms[i] = normalizer.normalize(this->background_terms[i]);
				this->background_chain->import(this->background_terms[i]);
			}
		}
		return true;
	}
};

#endif

