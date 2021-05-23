// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <stack>
#include <cmath>

#include "printutils.h"
#include "warp.h"
#include "roof_ss.h"


namespace warp_ns {

    // integer points (wrt given dimension) in interior of a line segment
    Polygon int_points_on_segment(const Vector3d &p0, const Vector3d &p1, int d) {
        // it is important that the points for (p0, p1) are exactly the same as for (p1, p0),
        // although in the opposite order
        if (p0[d] > p1[d]) {
            Polygon ppp = int_points_on_segment(p1, p0, d);
            std::reverse(ppp.begin(), ppp.end());
            return ppp;
        }

        Polygon ppp;
        double p01d = p1[d] - p0[d];
        double l = std::ceil(p0[d]);
        if (l == p0[d]) {
            l += 1.0;
        }
        for (; l < p1[d]; l+=1.0) {
            Vector3d p = ( (l - p0[d]) * p1 + (p1[d] - l) * p0 ) / p01d;
            p[d] = l; // make this exact
            ppp.push_back(p);
        }
        return ppp;
    }

    // cut a convex polygon along planes in dimension d (d=0 for X, d=1 for Y, d=2 for Z)
    Polygons cut_poly(const Polygon &poly, int d, double grid_d) {

        if (grid_d <= 0) {
            return Polygons(1,poly);
        }

        Polygon poly_i; // scaled poly with additional vertices at grid intersections
        Polygon poly_i_unscaled; // same unscaled, so we do not change existing vertices
        double min_d = poly[0][d] / grid_d,
               max_d = poly[0][d] / grid_d;
        // loop over edges, popupate poly_i
        for (size_t k=0; k<poly.size(); k++) {
            Vector3d p0 = ((k==0) ? poly.back() : poly[k-1]);
            Vector3d p0_unscaled = p0;
            Vector3d p1 = poly[k];
            p0[d] /= grid_d;
            p1[d] /= grid_d;
            
            max_d = std::max(max_d, p0[d]);
            min_d = std::min(min_d, p0[d]);
            
            Polygon int_points = int_points_on_segment(p0, p1, d);

            poly_i.push_back(p0);
            poly_i_unscaled.push_back(p0_unscaled);

            for (const auto &p : int_points) {
                Vector3d pp = p;
                pp[d] *= grid_d;
                poly_i.push_back(p);
                poly_i_unscaled.push_back(pp);
            }
        }
        
        if (min_d == max_d) {
            return Polygons(1,poly);
        }

        // strip index for an edge of poly_i given by endpoint
        std::function<int (int)> strip_i = [poly_i, d, min_d, &strip_i](int k) {
            int kp = (k==0) ? (poly_i.size() - 1) : (k - 1);
            Vector3d p0 = poly_i[kp];
            Vector3d p1 = poly_i[k];
            if (p0[d] == p1[d] && p0[d] == std::floor(p0[d])) {
                return strip_i(kp);
            } else {
                return int(std::min(std::floor(p0[d]), std::floor(p1[d])) - std::floor(min_d));
            }
        };
        
        Polygons ret;
        ret.resize(int(std::floor(max_d) - std::floor(min_d)) + 1);

        // loop over edges, put each edge into the respective strip
        for (size_t k=0; k<poly_i.size(); k++) {
            int kp = (k==0) ? (poly_i.size() - 1) : (k - 1);
            Vector3d p0 = poly_i_unscaled[kp];
            Vector3d p1 = poly_i_unscaled[k];
            int ei = strip_i(k);
            if (ei != strip_i(kp)) {
                ret[ei].push_back(p0);
            }
            ret[ei].push_back(p1);
        }

        // cleanup
        for (size_t k=0; k<ret.size();) {
            if (ret[k].size() == 0) {
                ret.erase(ret.begin() + k);
            } else {
                if (ret[k].size() < 3) {
                    /*
                    std::cout << "HUJ: " << ret[k].size() << ", PIZDA: " << poly_i.size() << "\n";
                    for (auto p: ret[k]) {
                        std::cout << "---\n" << p;
                    }
                    std::cout << "\n";
                    std::cout << "poly_i:\n";
                    for (size_t j = 0; j < poly_i.size(); j++) {
                        int jp = (j==0) ? (poly_i.size() - 1) : (j - 1);
                        std::cout << j << ": point: " << poly_i[j][d] << ", its floor: " << std::floor(poly_i[j][d])
                            << ", prev point: " << poly_i[jp][d] << ", its floor: " << std::floor(poly_i[jp][d])
                            << ", index: " << strip_i(j) << "\n";
                    }
                    std::cout << "\n\n";

                    if (poly_i[4] == poly_i[6]) {
                        std::cout << "4==6!!!\n";
                    }
                    
                    std::cout << "poly:\n";
                    for (size_t j = 0; j < poly.size(); j++) {
                        int jp = (j==0) ? (poly.size() - 1) : (j - 1);
                        std::cout << "point: " << poly[j][d] << ", its floor: " << std::floor(poly[j][d] / grid_d)
                            << ", prev point: " << poly[jp][d] << ", its floor: " << std::floor(poly[jp][d] / grid_d)
                            << "\n";
                        std::cout << "  points between: ";
                        for (auto & uu : int_points_on_segment(poly[jp] / grid_d, poly[j] / grid_d, d)) {
                            std::cout << "  " << uu[d];
                        }
                        std::cout << "\n";
                    }
                    std::cout << "\n\n";

                    exit(1);
                    */
                }
                k++;
            }
        }

        return ret;
    }

    shared_ptr<PolySet> warp(const PolySet &geom, double grid_x, double grid_y, double grid_z, double R, double r) {
        shared_ptr<PolySet> warped(new PolySet(3));

        for (const auto &face : geom.polygons) {
            for (const auto &poly0 : cut_poly(face, 0, grid_x)) {
                for (const auto &poly1 : cut_poly(poly0, 1, grid_y)) {
                    for (const auto &poly2 : cut_poly(poly1, 2, grid_z)) {
                        warped->append_poly(poly2);
                    }
                }
            }
        }

        for (auto &face : warped->polygons) {
            for (auto &v : face) {
                
                double rho = r + v[2];
                double theta = v[0] / (R + r);
                double phi = v[1] / r;

                v[0] = std::cos(theta) * (R + rho * std::cos(phi));
                v[1] = std::sin(theta) * (R + rho * std::cos(phi));
                v[2] = rho * std::sin(phi);
            }
        }

        return warped;
    }
} // warp_ns
