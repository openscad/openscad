//    Enhancement of OpenSCAD Primitives Solid with Trinagles 
//    Copyright (C) 2011  Rene BAUMANN, Switzerland
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Lesser General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this library; If not, see <http://www.gnu.org/licenses/>
//    or write to the Free Software Foundation, Inc., 
//    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
// ================================================================
//
//	File providing functions and modules to draw 3D - triangles 
//	created in the X-Y plane with hight h, using various triangle 
//	specification methods. 
//	Standard traingle geometrical definition  is used. Vertices are named A,B,C, 
//	side a is opposite vertex A a.s.o. the angle at vertex A is named alpha, 
//	B(beta), C(gamma).
//
//	This SW is a contribution to the Free Software Community doing a marvelous
//	job of giving anyone access to knowledge and tools to educate himselfe.
//
//	Author:		Rene Baumann
//	Date:		11.09.2011
//	Edition:	0.3	11.09.2011 For review by Marius
//	Edition:	0.4	11.11.2011 Ref to GPL2.1 added
//
// --------------------------------------------------------------------------------------
//
// ===========================================
//
//	FUNCTION: 		3dtri_sides2coord
//	DESCRIPTION:
//		Enter triangle sides a,b,c and to get the A,B,C - corner
// 		co-ordinates. The trinagle's c-side lies on the x-axis
// 		and A-corner in the co-ordinates center [0,0,0]. Geometry rules
//		required that a + b is greater then c.  The traingle's vertices are
//		computed such that it is located in the X-Y plane,  side c is on the 
//		positive x-axis. 
//	PARAMETER:
//		a	: real		length of side a
//		b	: real		length of side b
//		c	: real		length of side c
//	RETURNS:
//		vertices : [Acord,Bcord,Ccord] Array of vertices coordinates
//
//	COMMENT:
//		vertices = 3dtri_sides2coord (3,4,5);
//		vertices[0] : Acord	vertex A cordinates the like [x,y,z] 
// -------------------------------------------------------------------------------------	
//
function 3dtri_sides2coord (a,b,c) = [
		[0,0,0],
		[c,0,0], 
		[(pow(c,2)+pow(a,2)-pow(b,2))/(2*c),sqrt ( pow(a,2) - 
		   pow((pow(c,2)+pow(a,2)-pow(b,2))/(2*c),2)),0]];
//
//
// ===========================================
//
//	FUNCTION: 		3dtri_centerOfGravityCoord
//	DESCRIPTION:
//		Enter triangle  A,B,C - corner coordinates to get the
//		triangles Center of Gravity coordinates. It is assumed
//		the triangle is parallel to the  X-Y plane. The function 
//		returns always zero for the z-coordinate
//	PARAMETER:
//		Acord	: [x,y,z] 	Coordinates of vertex A
//		Bcord	: [x,y,z] 	Coordinates of vertex B
//		Ccord	: [x,y,z] 	Coordinates of vertex C
//	RETURNS:
//		CG : [x,y,0] 	Center of gravity coordinate in X-Y-plane
//
//	COMMENT:
//		vertices = 3dtri_sides2coord (3,4,5);
//		cg = 3dtri_centerOfGravityCoord(vertices[0],vertices[1],vertices[2]);
// -------------------------------------------------------------------------------------	
//
function 3dtri_centerOfGravityCoord (Acord,Bcord,Ccord) = [
		(Acord[0]+Bcord[0]+Ccord[0])/3,(Acord[1]+Bcord[1]+Ccord[1])/3,0];
//
//
// ===========================================
//
//	FUNCTION: 		3dtri_centerOfcircumcircle
//	DESCRIPTION:
//		Enter triangle  A,B,C - corner coordinates to get the
//		circum circle coordinates. It is assumed
//		the triangle is parallel to the  X-Y plane. The function 
//		returns always zero for the z-coordinate
//	PARAMETER:
//		Acord	: [x,y,z] 	Coordinates of vertex A
//		Bcord	: [x,y,z] 	Coordinates of vertex B
//		Ccord	: [x,y,z] 	Coordinates of vertex C
//	RETURNS:
//		cc	: [x,y,0] 	Circumcircle center
//
//	COMMENT:
//		vertices = 3dtri_sides2coord (3,4,5);
//		cc = 3dtri_centerOfcircumcircle (vertices[0],vertices[1],vertices[2]);
// -------------------------------------------------------------------------------------	
//
function 3dtri_centerOfcircumcircle (Acord,Bcord,Ccord) = 
		[0.5*Bcord[0],
		 0.5*((pow(Ccord[1],2)+pow(Ccord[0],2)-Bcord[0]*Ccord[0])/Ccord[1]),
		 0];
//
// 
//
// ===========================================
//
//	FUNCTION: 		3dtri_radiusOfcircumcircle
//	DESCRIPTION:
//		 Provides the triangle's radius from circumcircle  to the vertices.
//		It is assumed the triangle is parallel to the  X-Y plane. The function 
//		returns always zero for the z-coordinate
//	PARAMETER:
//		Vcord	:   [x,y,z] 	Coordinates of a vertex A or B,C
//		CCcord : [x,y,z] 	Coordinates of circumcircle
//		r	: Radius at vertices if round corner triangle used,
//			  else enter "0"
//	RETURNS:
//		cr	: Circumcircle radius
//
//	COMMENT:  Calculate circumcircle radius of trinagle with round vertices having
//			radius R = 2
//		vertices = 3dtri_sides2coord (3,4,5);
//		cc = 3dtri_centerOfcircumcircle (vertices[0],vertices[1],vertices[2]);
//		cr = 3dtri_radiusOfcircumcircle (vertices[0],cc,2); 
// -------------------------------------------------------------------------------------	
//
function 3dtri_radiusOfcircumcircle (Vcord,CCcord,R) = 
		sqrt(pow(CCcord[0]-Vcord[0],2)+pow(CCcord[1]-Vcord[1],2))+ R;
//
//
//
// ===========================================
//
//	FUNCTION: 		3dtri_radiusOfIn_circle
//	DESCRIPTION:
//		Enter triangle  A,B,C - corner coordinates to get the
//		in-circle radius. It is assumed the triangle is parallel to the  
//		X-Y plane. The function always returns zero for the z-coordinate. 
//		Formula used for inner circle radius: r = 2A /(a+b+c)
//	PARAMETER:
//		Acord	: [x,y,z] 	Coordinates of vertex A
//		Bcord	: [x,y,z] 	Coordinates of vertex B
//		Ccord	: [x,y,z] 	Coordinates of vertex C
//		
//	RETURNS:
//		ir	: real		radius of in-circle
//
//	COMMENT:
//		vertices = 3dtri_sides2coord (3,4,5);
//		ir = 3dtri_radiusOfIn_circle (vertices[0],vertices[1],vertices[2]);
// -------------------------------------------------------------------------------------	
//
function 3dtri_radiusOfIn_circle (Acord,Bcord,Ccord) = 
	Bcord[0]*Ccord[1]/(Bcord[0]+sqrt(pow(Ccord[0]-Bcord[0],2)+pow(Ccord[1],2))+
	sqrt(pow(Ccord[0],2)+pow(Ccord[1],2)));
//
// 
//
// ===========================================
//
//	FUNCTION: 		3dtri_centerOfIn_circle
//	DESCRIPTION:
//		Enter triangle  A,B,C - corner coordinates to get the
//		in-circle coordinates. It is assumed
//		the triangle is parallel to the  X-Y plane. The function 
//		returns always zero for the z-coordinate
//	PARAMETER:
//		Acord	: [x,y,z] 	Coordinates of vertex A
//		Bcord	: [x,y,z] 	Coordinates of vertex B
//		Ccord	: [x,y,z] 	Coordinates of vertex C
//		r	: real		radius of in-circle
//	RETURNS:
//		ic	: [x,y,0] 	In-circle center co-ordinates
//
//	COMMENT:
//		vertices = 3dtri_sides2coord (3,4,5);
//		ir = 3dtri_radiusOfIn_circle (vertices[0],vertices[1],vertices[2]);
//		ic = 3dtri_centerOfIn_circle (vertices[0],vertices[1],vertices[2],ir);
// -------------------------------------------------------------------------------------	
//
function 3dtri_centerOfIn_circle (Acord,Bcord,Ccord,r) = 
		[(Bcord[0]+sqrt(pow(Ccord[0]-Bcord[0],2)+pow(Ccord[1],2))+
	sqrt(pow(Ccord[0],2)+pow(Ccord[1],2)))/2-sqrt(pow(Ccord[0]-Bcord[0],2)+pow(Ccord[1],2)),r,0];
//
//
// ============================================
//
//	MODULE: 		3dtri_draw
//	DESCRIPTION:
//		 Draw a standard solid triangle with A,B,C - vertices specified by its 
// 		co-ordinates and height "h", as given by the  input parameters. 
//	PARAMETER:
//		Acord	: [x,y,z] 	Coordinates of vertex A
//		Bcord	: [x,y,z] 	Coordinates of vertex B
//		Ccord	: [x,y,z] 	Coordinates of vertex C
//		h	: real		Hight of the triangle
//	RETURNS:
//		none
//
//	COMMENT:
//		You might use the result from function 3dtri_sides2coord
//		to call module 3dtri_draw ( vertices[0],vertices[1],vertices[2], h) 
// -------------------------------------------------------------------------------------	
//
module 3dtri_draw ( Acord, Bcord, Ccord, h) {
polyhedron (points=[Acord,Bcord,Ccord,
					Acord+[0,0,h],Bcord+[0,0,h],Ccord+[0,0,h]],
			triangles=[	[0,1,2],[0,2,3],[3,2,5],
					[3,5,4],[1,5,2],[4,5,1],
					[4,1,0],[0,3,4]]);

};
//
//
// ==============================================
//
//	MODULE: 		3dtri_rnd_draw
//	DESCRIPTION:
//		Draw a round corner triangle with A,B,C - vertices specified by its 
// 		co-ordinates, height h and round vertices having radius "r".
//		As specified by the input parameters.
//		Please note, the tringles side lenght gets extended by "2 * r",
//		and the vertices coordinates define the centers of the 
//		circles with radius "r". 
//	PARAMETER:
//		Acord	: [x,y,z] 	Coordinates of vertex A
//		Bcord	: [x,y,z] 	Coordinates of vertex B
//		Ccord	: [x,y,z] 	Coordinates of vertex C
//		h	: real		Hight of the triangle
//		r	: real		Radius from vertices coordinates
//	RETURNS:
//		none
//
//	COMMENT:
//		You might use the result from function 3dtri_sides2coord
//		to call module 3dtri_rnd_draw ( vertices[0],vertices[1],vertices[2], h, r) 
// -------------------------------------------------------------------------------------	
//
module 3dtri_rnd_draw ( Acord, Bcord, Ccord, h, r) {
Avect=Ccord-Bcord;	// vector pointing from vertex B to vertex C
p0=Acord + [0,-r,0];
p1=Bcord + [0,-r,0];
p2=Bcord + [r*Avect[1]/sqrt(pow(Avect[0],2)+pow(Avect[1],2)), 
                    -r*Avect[0]/sqrt(pow(Avect[0],2)+pow(Avect[1],2)) ,0];
p3=Ccord + [r*Avect[1]/sqrt(pow(Avect[0],2)+pow(Avect[1],2)), 
                     -r*Avect[0]/sqrt(pow(Avect[0],2)+pow(Avect[1],2)) ,0];
p4=Ccord +[- r*Ccord[1]/sqrt(pow(Ccord[0],2)+pow(Ccord[1],2)), 
                     r*Ccord[0]/sqrt(pow(Ccord[0],2)+pow(Ccord[1],2)) ,0];
p5=Acord + [- r*Ccord[1]/sqrt(pow(Ccord[0],2)+pow(Ccord[1],2)), 
                     r*Ccord[0]/sqrt(pow(Ccord[0],2)+pow(Ccord[1],2)) ,0];
bottom_triangles = [[0,1,2],[0,2,3],[0,3,4],[0,4,5]];
c_side_triangles = [[7,1,0],[0,6,7]];
a_side_triangles = [[2,8,3],[8,9,3]];
b_side_triangles = [[4,10,5],[10,11,5]];
A_edge_triangles = [[0,5,11],[0,11,6]];
B_edge_triangles = [[1,7,2],[2,7,8]];
C_edge_triangles = [[3,9,4],[9,10,4]];
top_triangles = [[11,7,6],[11,8,7],[11,10,8],[8,10,9]];
union () { 
	polyhedron (points=[p0,p1,p2,p3,p4,p5,
					p0+[0,0,h],p1+[0,0,h],p2+[0,0,h],p3+[0,0,h],p4+[0,0,h],p5+[0,0,h]],
			triangles=[	bottom_triangles[0],bottom_triangles[1],bottom_triangles[2],bottom_triangles[3],
						A_edge_triangles[0],A_edge_triangles[1],
						c_side_triangles[0],c_side_triangles[1],
						B_edge_triangles[0],B_edge_triangles[1],
						a_side_triangles[0],a_side_triangles[1],
						C_edge_triangles[0],C_edge_triangles[1],
						b_side_triangles[0],b_side_triangles[1],
						top_triangles[0],top_triangles[1],top_triangles[2],top_triangles[3]]);
	translate(Acord) cylinder(r1=r,r2=r,h=h,center=false);
	translate(Bcord) cylinder(r1=r,r2=r,h=h,center=false);
	translate(Ccord) cylinder(r1=r,r2=r,h=h,center=false);
};
}
//
// ==============================================
//
// Demo Application - copy into new file and uncomment or uncomment here but
// without uncommenting the use <...> statement, then press F6 - Key 
//
// use <MCAD/3d_triangle.scad>;
//$fn=50;
//  h =4;
//  r=2;
//  echo ("Draws a right angle triangle with its circumcircle and in-circle");
//  echo ("The calculated co-ordinates and radius are show in this console window");
//  echo ("Geometry rules for a right angle triangle say, that the circumcircle is the");
//  echo ("Thales Circle which center must be in the middle of the triangle's c - side");
//  echo ("===========================================");
//  vertices  =  3dtri_sides2coord (30,40,50);
//  echo("A = ",vertices[0],"  B = ",vertices[1],"  C = ",vertices[2]);
//  cg = 3dtri_centerOfGravityCoord (vertices[0],vertices[1],vertices[2]);
//  echo (" Center of gravity = ",cg);
//  cc = 3dtri_centerOfcircumcircle (vertices[0],vertices[1],vertices[2]);
//  echo (" Center of circumcircle = ",cc);
//  cr = 3dtri_radiusOfcircumcircle (vertices[0],cc,r); 
//  echo(" Radius of circumcircle ",cr);
//  ir = 3dtri_radiusOfIn_circle (vertices[0],vertices[1],vertices[2]);
//  echo (" Radius of in-circle = ",ir);
//  ic = 3dtri_centerOfIn_circle (vertices[0],vertices[1],vertices[2],ir);
//  echo (" Center of in-circle = ",ic);
//   translate(cc+[0,0,5*h/2]) difference () {
//  		cylinder (h=5*h,r1=cr+4,r2=cr+4,center=true);
//  		cylinder (h=6*h,r1=cr,r2=cr,center=true);}
//  difference () {
//  	union () {
//  	     difference () {
//   		3dtri_rnd_draw (vertices[0], vertices[1], vertices[2],5*h,r);
//  		scale([0.8,0.8,1]) translate([6,2,4*h])  3dtri_rnd_draw (vertices[0], vertices[1], vertices[2],5*h,r);
//  	     }
//  	     translate (ic+[0,0,5*h]) cylinder(h=10*h,r1=ir+r,r2=ir+r,center=true);
//  	}
//  	translate (ic+[0,0,5*h]) cylinder(h=12*h,r1=0.5*ir,r2=0.5*ir,center=true);
//  }

