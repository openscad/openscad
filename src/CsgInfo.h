#pragma once

#include "csgnode.h"
#include "Tree.h"
#include "GeometryEvaluator.h"
#include "CSGTreeEvaluator.h"
#include "CSGTreeNormalizer.h"
#include "rendersettings.h"
#include "printutils.h"

/*
   Small helper class for compiling and normalizing node trees into CSG products
 */
class CsgInfo
{
public:
	CsgInfo() {}
	shared_ptr<class CSGProducts> root_products;
	shared_ptr<CSGProducts> highlights_products;
	shared_ptr<CSGProducts> background_products;

	bool compile_products(const Tree &tree) {
		const AbstractNode *root_node = tree.root();
		GeometryEvaluator geomevaluator(tree);
		CSGTreeEvaluator evaluator(tree, &geomevaluator);
		shared_ptr<CSGNode> csgRoot = evaluator.buildCSGTree(*root_node);
		std::vector<shared_ptr<CSGNode>> highlightNodes = evaluator.getHighlightNodes();
		std::vector<shared_ptr<CSGNode>> backgroundNodes = evaluator.getBackgroundNodes();

		PRINT("Compiling design (CSG Products normalization)...");
		CSGTreeNormalizer normalizer(RenderSettings::inst()->openCSGTermLimit);
		if (csgRoot) {
			shared_ptr<CSGNode> normalizedRoot = normalizer.normalize(csgRoot);
			if (normalizedRoot) {
				this->root_products.reset(new CSGProducts());
				this->root_products->import(normalizedRoot);
				PRINTB("Normalized CSG tree has %d elements", int(this->root_products->size()));
			}
			else {
				this->root_products.reset();
				PRINT("WARNING: CSG normalization resulted in an empty tree");
			}
		}

		if (highlightNodes.size() > 0) {
			PRINTB("Compiling highlights (%i CSG Trees)...", highlightNodes.size());
			this->highlights_products.reset(new CSGProducts());
			for (unsigned int i = 0; i < highlightNodes.size(); i++) {
				highlightNodes[i] = normalizer.normalize(highlightNodes[i]);
				this->highlights_products->import(highlightNodes[i]);
			}
		}

		if (backgroundNodes.size() > 0) {
			PRINTB("Compiling background (%i CSG Trees)...", backgroundNodes.size());
			this->background_products.reset(new CSGProducts());
			for (unsigned int i = 0; i < backgroundNodes.size(); i++) {
				backgroundNodes[i] = normalizer.normalize(backgroundNodes[i]);
				this->background_products->import(backgroundNodes[i]);
			}
		}
		return true;
	}
};
