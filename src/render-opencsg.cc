#include "render-opencsg.h"
#include "polyset.h"
#include "csgterm.h"
#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity) :
			OpenCSG::Primitive(operation, convexity) { }
	PolySet *p;
	double *m;
	int csgmode;
	virtual void render() {
		glPushMatrix();
		glMultMatrixd(m);
		p->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m);
		glPopMatrix();
	}
};

void renderCSGChainviaOpenCSG(CSGChain *chain, GLint *shaderinfo, bool highlight, bool background)
{
	std::vector<OpenCSG::Primitive*> primitives;
	int j = 0;
	for (int i = 0;; i++)
	{
		bool last = i == chain->polysets.size();

		if (last || chain->types[i] == CSGTerm::TYPE_UNION)
		{
			if (j+1 != i) {
				OpenCSG::render(primitives);
				glDepthFunc(GL_EQUAL);
			}
			if (shaderinfo)
				glUseProgram(shaderinfo[0]);
			for (; j < i; j++) {
				double *m = chain->matrices[j];
				glPushMatrix();
				glMultMatrixd(m);
				int csgmode = chain->types[j] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
				if (highlight) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20), m, shaderinfo);
				} else if (background) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10), m, shaderinfo);
				} else if (m[16] >= 0 || m[17] >= 0 || m[18] >= 0 || m[19] >= 0) {
					// User-defined color from source
					glColor4d(m[16], m[17], m[18], m[19]);
					if (shaderinfo) {
						glUniform4f(shaderinfo[1], m[16], m[17], m[18], m[19]);
						glUniform4f(shaderinfo[2], (m[16]+1)/2, (m[17]+1)/2, (m[18]+1)/2, 1.0);
					}
					chain->polysets[j]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m, shaderinfo);
				} else if (chain->types[j] == CSGTerm::TYPE_DIFFERENCE) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode), m, shaderinfo);
				} else {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode), m, shaderinfo);
				}
				glPopMatrix();
			}
			if (shaderinfo)
				glUseProgram(0);
			for (unsigned int k = 0; k < primitives.size(); k++) {
				delete primitives[k];
			}
			glDepthFunc(GL_LEQUAL);
			primitives.clear();
		}

		if (last)
			break;

		OpenCSGPrim *prim = new OpenCSGPrim(chain->types[i] == CSGTerm::TYPE_DIFFERENCE ?
				OpenCSG::Subtraction : OpenCSG::Intersection, chain->polysets[i]->convexity);
		prim->p = chain->polysets[i];
		prim->m = chain->matrices[i];
		prim->csgmode = chain->types[i] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
		if (highlight)
			prim->csgmode += 20;
		else if (background)
			prim->csgmode += 10;
		primitives.push_back(prim);
	}
}
