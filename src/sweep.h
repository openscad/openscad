//
//  sweep.h
//
//
//  Created by Oskar Linde on 2014-05-17.
//
//

#pragma once

#include <vector>
#include <Eigen/Geometry>



class Polygon2d * sweep2d_2d(std::vector<Eigen::Projective3d> const& path,
							 Polygon2d const& poly);


