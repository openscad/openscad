#pragma once

#include "geometry/linalg.h"
#include "glview/ColorMap.h"
#include "core/enums.h"
#include "geometry/PolySet.h"
#include "glview/Renderer.h"

void render_surface(const PolySet& geom, const Transform3d& m, const Renderer::shaderinfo_t *shaderinfo = nullptr);
