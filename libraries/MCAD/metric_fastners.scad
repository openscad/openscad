/*
 *  OpenSCAD Metric Fastners Library (www.openscad.org)
 *  Copyright (C) 2010-2011  Giles Bathgate
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License,
 *  LGPL version 2.1, or (at your option) any later version of the GPL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
*/

$fn=50;
apply_chamfer=true;

module cap_bolt(dia,len)
{
	e=1.5*dia;
	h1=1.25*dia;
	cylinder(r=dia/2,h=len);
	translate([0,0,-h1]) cylinder(r=e/2,h=h1);
}

module csk_bolt(dia,len)
{
	h1=0.6*dia;
	h2=len-h1;
	cylinder(r=dia/2,h=h2);
	cylinder(r1=dia,r2=dia/2,h=h1);
}

module washer(dia)
{
	t=0.1*dia;
	difference()
	{
		cylinder(r=dia,h=t);
		translate([0,0,-t/2])cylinder(r=dia/2,h=t*2);
	}
}

module flat_nut(dia)
{
	m=0.8*dia;
	e=1.8*dia;
	c=0.2*dia;
	difference()
	{
		cylinder(r=e/2,h=m,$fn=6);
		translate([0,0,-m/2])cylinder(r=dia/2,h=m*2);
		if(apply_chamfer)
		    translate([0,0,c])cylinder_chamfer(e/2,c);
	}
}

module bolt(dia,len)
{
	e=1.8*dia;
	k=0.7*dia;
	c=0.2*dia;
	difference()
	{
		cylinder(r=e/2,h=k,$fn=6);
		if(apply_chamfer)
		    translate([0,0,c])cylinder_chamfer(e/2,c);
	}

	cylinder(r=dia/2,h=len);

}

module cylinder_chamfer(r1,r2)
{
	t=r1-r2;
	p=r2*2;
	rotate_extrude()
	difference()
	{
		translate([t,-p])square([p,p]);
		translate([t,0])circle(r2);
	}
}

module chamfer(len,r)
{
	p=r*2;
	linear_extrude(height=len)
	difference()
	{
		square([p,p]);
		circle(r);
	}
}

union()
{
//csk_bolt(3,14);
//washer(3);
//flat_nut(3);
//bolt(4,14);
//cylinder_chamfer(8,1);
//chamfer(10,2);
}
