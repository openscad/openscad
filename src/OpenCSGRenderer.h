#ifndef OPENCSG_RENDERER_H_
#define OPENCSG_RENDERER_H_

#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

#include "renderer.h"
#include "system-gl.h"


class GeometryPrimitive : public OpenCSG::Primitive
{
	public:
		GeometryPrimitive(shared_ptr<const Geometry> geom, Transform3d m, Renderer::csgmode_e csgmode, OpenCSG::Operation operation, unsigned int convexity)
				  : OpenCSG::Primitive(operation, convexity), id_(0), built_(false), geom_(geom), m_(m), csgmode_(csgmode) {}
		virtual ~GeometryPrimitive();
		virtual void render();
	private:
		unsigned int id_;
		bool built_;
		shared_ptr<const Geometry> geom_;
		Transform3d m_;
		Renderer::csgmode_e csgmode_;
};


class OpenCSGRenderer : public Renderer
{
	public:
		OpenCSGRenderer(class CSGChain *root_chain, CSGChain *highlights_chain,
				CSGChain *background_chain, GLint *shaderinfo);
		virtual ~OpenCSGRenderer();
		virtual void draw(bool showfaces, bool showedges) const;
		virtual BoundingBox getBoundingBox() const;
	private:
		void buildCSGChain(class CSGChain *chain, GLint *shaderinfo,
				   bool Highlight, bool background);
		void renderCSGChain(class CSGChain *chain, GLint *shaderinfo,
				    bool highlight, bool background) const;

		CSGChain *root_chain_;
		CSGChain *highlights_chain_;
		CSGChain *background_chain_;
		GLint *shaderinfo_;

		bool root_chain_built_;
		bool highlights_chain_built_;
		bool background_chain_built_;
		std::vector<unsigned int> root_chain_list_ids_;
		std::vector<std::vector<OpenCSG::Primitive *> > root_chain_primitives_;
		std::vector<unsigned int> highlights_chain_list_ids_;
		std::vector<std::vector<OpenCSG::Primitive *> > highlights_chain_primitives_;
		std::vector<unsigned int> background_chain_list_ids_;
		std::vector<std::vector<OpenCSG::Primitive *> > background_chain_primitives_;
};

#endif // OPENCSG_RENDERER_H_
