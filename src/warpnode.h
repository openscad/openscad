// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include <string>

#include "node.h"
#include "value.h"

class WarpNode : public AbstractPolyNode
{
public:
	VISITABLE();
	WarpNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractPolyNode(mi, ctx) {}
	std::string toString() const override;
	std::string name() const override { return "warp"; }

	double grid_x, grid_y, grid_z;
        double R, r;

	class warp_exception: public std::exception {
		public:
			warp_exception(const std::string message) : m (message) {};
			std::string message() {return m;}
		private:
			std::string m;
	};
};
