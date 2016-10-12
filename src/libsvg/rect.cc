#include <stdlib.h>
#include <iostream>
#include <cmath>

#include <boost/format.hpp>

#include "rect.h"

namespace libsvg {

const std::string rect::name("rect"); 

rect::rect()
{
}

rect::rect(const rect& orig) : path(orig)
{
	rx = orig.rx;
	ry = orig.ry;
	width = orig.width;
	height = orig.height;
}

rect::~rect()
{
}

/**
 * Let rx and ry be length values.
 * 
 * If neither ‘rx’ nor ‘ry’ are properly specified, then set both rx and
 * ry to 0. (This will result in square corners.)
 * Otherwise, if a properly specified value is provided for ‘rx’, but not
 * for ‘ry’, then set both rx and ry to the value of ‘rx’.
 * Otherwise, if a properly specified value is provided for ‘ry’, but not
 * for ‘rx’, then set both rx and ry to the value of ‘ry’.
 * Otherwise, both ‘rx’ and ‘ry’ were specified properly. Set rx to the
 * value of ‘rx’ and ry to the value of ‘ry’.
 * If rx is greater than half of ‘width’, then set rx to half of ‘width’.
 * If ry is greater than half of ‘height’, then set ry to half of ‘height’.
 * The effective values of ‘rx’ and ‘ry’ are rx and ry, respectively.
 * 
 * 
 * Mathematically, a ‘rect’ element can be mapped to an equivalent
 * ‘path’ element as follows: (Note: all coordinate and length values
 * are first converted into user space coordinates according to Units.)
 *
 * 1) perform an absolute moveto operation to location (x+rx,y), where x is
 * the value of the ‘rect’ element's ‘x’ attribute converted to user space,
 * rx is the effective value of the ‘rx’ attribute converted to user space
 * and y is the value of the ‘y’ attribute converted to user space
 * 
 * 2) perform an absolute horizontal lineto operation to location (x+width-rx,y),
 * where width is the ‘rect’ element's ‘width’ attribute converted to user
 * space
 * 
 * 3) perform an absolute elliptical arc operation to coordinate (x+width,y+ry),
 * where the effective values for the ‘rx’ and ‘ry’ attributes on the ‘rect’
 * element converted to user space are used as the rx and ry attributes on
 * the elliptical arc command, respectively, the x-axis-rotation is set to
 * zero, the large-arc-flag is set to zero, and the sweep-flag is set to
 * one
 * 
 * 4) perform a absolute vertical lineto to location (x+width,y+height-ry),
 * where height is the ‘rect’ element's ‘height’ attribute converted to user
 * space
 * 
 * 5) perform an absolute elliptical arc operation to coordinate (x+width-rx,y+height)
 * 
 * 6) perform an absolute horizontal lineto to location (x+rx,y+height)
 * 
 * 7) perform an absolute elliptical arc operation to coordinate (x,y+height-ry)
 * 
 * 8) perform an absolute absolute vertical lineto to location (x,y+ry)
 * 
 * 9) perform an absolute elliptical arc operation to coordinate (x+rx,y)
 */
void
rect::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->x = parse_double(attrs["x"]);
	this->y = parse_double(attrs["y"]);
	this->width = parse_double(attrs["width"]);
	this->height = parse_double(attrs["height"]);
	this->rx = parse_double(attrs["rx"]);
	this->ry = parse_double(attrs["ry"]);

	bool has_rx = !(std::fabs(rx) < 1e-8);
	bool has_ry = !(std::fabs(ry) < 1e-8);
	
	if (has_rx || has_ry) {
		if (!has_rx) {
			this->rx = this->ry;
		} else if (!has_ry) {
			this->ry = this->rx;
		}
		if (this->rx > (this->width / 2)) {
			this->rx = this->width / 2;
		}
		if (this->ry > (this->height / 2)) {
			this->ry = this->height / 2;
		}
		
		std::string path = boost::str(boost::format(""
			"M %1%,%2% "
			"H %3% "
			"A %4%,%5% 0 0,1 %6%,%7% "
			"V %8% "
			"A %9%,%10% 0 0,1 %11%,%12% "
			"H %13% "
			"A %14%,%15% 0 0,1 %16%,%17% "
			"V %18% "
			"A %19%,%20% 0 0,1 %21%,%22% "
			"z")
		% (x + rx) % y
		% (x + width - rx)
		% rx % ry % (x + width) % (y + ry)
		% (y + height - ry)
		% rx % ry % (x + width - rx) % (y + height)
		% (x + rx)
		% rx % ry % x % (y + height - ry)
		% (y + ry)
		% rx % ry % (x + rx) % y
			);
		attrs["d"] = path;
		path::set_attrs(attrs);
	} else {
		path_t path;
		path.push_back(Eigen::Vector3d(get_x(), get_y(), 0));
		path.push_back(Eigen::Vector3d(get_x() + get_width(), get_y(), 0));
		path.push_back(Eigen::Vector3d(get_x() + get_width(), get_y() + get_height(), 0));
		path.push_back(Eigen::Vector3d(get_x(), get_y() + get_height(), 0));
		path.push_back(Eigen::Vector3d(get_x(), get_y(), 0));
		path_list.push_back(path);
	}
}

void
rect::dump()
{
	std::cout << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< ": width = " << this->width
		<< ": height = " << this->height
		<< std::endl;
}

}
