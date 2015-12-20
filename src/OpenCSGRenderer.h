#ifndef OPENCSG_RENDERER_H_
#define OPENCSG_RENDERER_H_

#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgterm.h"


class OpenCSGPrim : public OpenCSG::Primitive
{
	public:
		OpenCSGPrim(shared_ptr<const Geometry> geom, Transform3d m, Renderer::csgmode_e csgmode, OpenCSG::Operation operation, unsigned int convexity, GLint *shaderinfo)
			: OpenCSG::Primitive(operation, convexity), id_(0), built_(false), geom_(geom), m_(m), csgmode_(csgmode), shaderinfo_(shaderinfo) {}
		virtual ~OpenCSGPrim();
		virtual void render();
	private:
		unsigned int id_;
		bool built_;
		shared_ptr<const Geometry> geom_;
		Transform3d m_;
		Renderer::csgmode_e csgmode_;
		GLint *shaderinfo_;
};


class OpenCSGRenderer : public Renderer
{
	public:
		OpenCSGRenderer(class CSGProducts *root_products, CSGProducts *highlights_products,
				CSGProducts *background_products, GLint *shaderinfo);
		virtual ~OpenCSGRenderer();
		virtual void draw(bool showfaces, bool showedges) const;
		virtual BoundingBox getBoundingBox() const;
	private:
#ifdef ENABLE_OPENCSG
		class OpenCSGPrim *createCSGPrimitive(const class CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type, GLint *shaderinfo) const;
#endif
		void renderCSGProducts(const class CSGProducts &products, GLint *shaderinfo,
					bool highlight_mode, bool background_mode) const;

		CSGProducts *root_products_;
		CSGProducts *highlights_products_;
		CSGProducts *background_products_;
		GLint *shaderinfo_;

		bool root_products_built_;
		bool highlights_products_built_;
		bool background_products_built_;
		std::vector<unsigned int> root_products_ids_;
		std::vector<std::vector<OpenCSG::Primitive *> > root_products_primitives_;
		std::vector<unsigned int> highlights_products_ids_;
		std::vector<std::vector<OpenCSG::Primitive *> > highlights_products_primitives_;
		std::vector<unsigned int> background_products_ids_;
		std::vector<std::vector<OpenCSG::Primitive *> > background_products_primitives_;
};

#endif // OPENCSG_RENDERER_H_
