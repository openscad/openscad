#ifndef __CSGINFO_H__
#define __CSGINFO_H__

#include "OffscreenView.h"

class CsgInfo
{
public:
	CsgInfo() { qglview = NULL; }
	OffscreenView *qglview;
};


#ifdef ENABLE_OPENCSG

#include <opencsg.h>
#include "OpenCSGRenderer.h"
#include "csgterm.h"
#include "csgtermnormalizer.h"

class CsgInfo_OpenCSG : public CsgInfo
{
public:
	CsgInfo_OpenCSG()
	{
		root_chain = NULL;
		highlights_chain = NULL;
		background_chain = NULL;
		qglview = NULL;
	}
	shared_ptr<CSGTerm> root_norm_term;    // Normalized CSG products
	class CSGChain *root_chain;
	std::vector<shared_ptr<CSGTerm> > highlight_terms;
	CSGChain *highlights_chain;
	std::vector<shared_ptr<CSGTerm> > background_terms;
	CSGChain *background_chain;
};

#endif // ENABLE_OPENCSG

#endif

