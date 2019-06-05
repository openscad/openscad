#include "import.h"
#include "grid.h"
#include "printutils.h"
#include "calc.h"
#include "Polygon2d.h"
#include "degree_trig.h"

Polygon2d *import_dxf(const std::string &filename)
{
    // We now assume that brlcad can pass dxf data through this
    // My thought is we may create a class for each entity to store the data?

    // We should have somthing simiar to this.
    // brlcad::entities_list *entities = brlcad::read_dxf_file(filename.c_str());
    // Assume we have receive a data of square.
    
    // To make it simple, I will ignore other enitities. Basically just use necessary
    // component to form the geometry

    // These are other expected values that we should get from just the line entity
    // std::string curr_layer_name
    // std::string curr_color;
    // double scale;
    
    // Last extract data like coord from the entity
    
    //suqare
    auto poly = new Polygon2d();
	Grid2d<std::vector<int>> grid(GRID_COARSE);
    std::vector<double> xverts = {-20, -20, 20, 20};
	std::vector<double> yverts = {-20, 20, 20, -20};
    Outline2d outline;
    // int vertexNum = 4;
    // for(int i = 0; i < vertexNum; i++){
    //     grid.align(xverts.at(i), yverts.at(i));
    //     //what does the grid.align do? Just align the vertex on the grid?
    //     outline.vertices.push_back(Vector2d(xverts.at(i),yverts.at(i)));
    //     outline.positive = true;
    // }

    //Now assuming we got the data of a circle
    //We should get raidus, center of the circle and special values $fn, $fa, $fs
    
    double radius = 10;
    double fn = 0, fs = 2, fa = 12;
    xverts.at(0) = 0;
    yverts.at(0) = 0;
    int n = Calc::get_fragments_from_r(radius, fn, fs, fa);
    Vector2d center(xverts.at(0), yverts.at(0));
    for (int i = 0; i < n; i++) {
        double a1 = (360.0 * i) / n;
        double a2 = (360.0 *(i + 1)) / n;
        outline.vertices.push_back(Vector2d(cos_degrees(a1)*radius + center[0], 
                                            sin_degrees(a1)*radius + center[1]));
        outline.vertices.push_back(Vector2d(cos_degrees(a2)*radius + center[0], 
                                            sin_degrees(a2)*radius + center[1]));
    }
    
    poly->addOutline(outline);
    return poly;
}
