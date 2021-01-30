/*
 * OpenSCAD (https://www.openscad.org)
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#pragma once

#include "GeometryCache.h"
#include "CGALCache.h"
#include "polyset.h"
#include "CGAL_Nef_polyhedron.h"

/*! Cache helper that respects the caller's type wishes. */
class MixedCache
{
public:
	template <typename T>
	static shared_ptr<const T> getOrSet(const std::string &key,
																			const std::function<shared_ptr<const T>()> &f)
	{
		if (key != "") {
			if (std::is_assignable<T, CGAL_Nef_polyhedron>::value &&
					CGALCache::instance()->contains(key)) {
				auto value = CGALCache::instance()->get(key);
				auto cast = dynamic_pointer_cast<const T>(value);
				if ((value == nullptr) == (cast == nullptr)) return cast;
			}
			else if (GeometryCache::instance()->contains(key)) {
				auto value = GeometryCache::instance()->get(key);
				auto cast = dynamic_pointer_cast<const T>(value);
				if ((value == nullptr) == (cast == nullptr)) return cast;
			}
		}

		auto result = f();
		if (key != "") {
			if (auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(result)) {
				CGALCache::instance()->insert(key, nef);
			}
			else if (auto polyset = dynamic_pointer_cast<const PolySet>(result)) {
				GeometryCache::instance()->insert(key, polyset);
			}
		}
		return result;
	}
};
