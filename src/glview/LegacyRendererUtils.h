#pragma once

#include "linalg.h"
#include "ColorMap.h"
#include "enums.h"
#include "PolySet.h"
#include "Renderer.h"

void render_surface(const PolySet& geom, const Transform3d& m, const Renderer::shaderinfo_t *shaderinfo = nullptr);
