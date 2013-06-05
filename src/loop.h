/**
 *  Addition to OpenSCAD (www.openscad.org)
 *  Copyright (C) 2013 Ruud Vlaming <ruud@betaresearch.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef LOOP_H_
#define LOOP_H_

#include "value.h"
#include <iostream>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/StdVector>
#include <math.h>
#include <vector>
#include <algorithm>

/* Needed for most situations (also 64 bit!) see http://eigen.tuxfamily.org/dox/TopicStlContainers.html */
#define Vector3Dvector std::vector< Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> >
#define Matrix3Dvector std::vector< Eigen::Matrix3d, Eigen::aligned_allocator<Eigen::Matrix3d> >


class Lib
{ public :
    template <class T> static bool has(const std::vector<T> vec, const T val);
    template <class T> static bool msz(const std::vector< std::vector<T> > vecs, const int minsize);
    static bool isEven(unsigned i);
    static bool isOdd(unsigned i); };

class Vertex
{ public:
    enum VertexType {DEF,LIN,ARC,BEZ};
  private:
    VertexType vtype;
    double pars[4];
  public:
    // const ervoor?
    Eigen::Vector3d loc;
    double gPar(const unsigned i);
    VertexType gVtype();
    Vertex(Eigen::Vector3d Ploc);
    void specify(const VertexType Pvtype, const std::vector<double> Ppars);
    void reset();
  public:
    static VertexType stringsHasVertexType(const std::vector<std::string> strs, const VertexType deftype = Vertex::DEF); };


class Edge
{ public:
    enum EdgeType {DEF,LIN,BEZ,SIN};
    enum TransType {IDN,ROT,RTX,RTY,RXY};
  private:
    EdgeType etype;
    TransType ttype;
    double pars[6];
  public:
    Eigen::Matrix3d baseRotation(const double scale);
    Eigen::Vector3d baseTranslation(const Eigen::Vector3d t);
    Vector3Dvector bzps;
    Eigen::Matrix3d base;
    double gPar(const unsigned i);
    EdgeType gEtype();
    TransType gTtype();
    Edge();
    void specify(const EdgeType Petype, const TransType Pttype, const std::vector<double> Ppars, const Vector3Dvector Pbzps);
    void reset();
  public:
    static EdgeType stringsHasEdgeType(const std::vector<std::string> strs, const EdgeType deftype = Edge::DEF);
    static TransType stringsHasTransType(const std::vector<std::string> strs, const TransType deftype = Edge::IDN); };


class Segment
{ public:
    enum SegmentType {DEF,LIN,BEZ,SIN};
    enum TransType {IDN,ROT,RTL,RLR};
  private:
    bool show;
    bool outer;
    SegmentType stype;
    TransType ttype;
    double pars[6];
    double length;
  public:
    Vector3Dvector bzps;
    double gPar(const unsigned i);
    double gLength();
    void addToLength(const double len);
    SegmentType gStype();
    TransType gTtype();
    bool gOuter();
    bool gShow();
    Segment();
    void specify(const SegmentType Pstype, const TransType Pttype, const std::vector<double> Ppars, const Vector3Dvector Pbzps);
    void specify(const bool show, const bool hide, const bool out, const bool in, const std::vector<double> Ppars);
    void reset();
  public:
    static SegmentType stringsHasSegmentType(const std::vector<std::string> strs, const SegmentType deftype = Segment::DEF);
    static TransType stringsHasTransType(const std::vector<std::string> strs, const TransType deftype = Segment::IDN); };

class Step
{ public :
    Eigen::Vector3d loc;
    unsigned partnr;
    bool valid;
    Step(const Eigen::Vector3d loc, const unsigned partnr);
};


class Plane
{ public :
    enum PlaneType {DEF,OUT,AUX};
    enum CoverType {NONE,TOP,BOTTOM,RING};
  private :
    PlaneType ptype;
    CoverType ctype;
    unsigned part;
    Eigen::Vector3d cent;
    Eigen::Vector3d tang;
    Eigen::Vector3d norm;
    Eigen::Vector3d sdir;
    double loc, length, scale, stretch, angle;
    //bool draw;
  public :
    Eigen::Vector3d gCent();
    Eigen::Vector3d gTang();
    Eigen::Vector3d gNorm();
    Eigen::Vector3d gBnrm();
    Eigen::Vector3d gSdir();
    double gScale();
    double gLoc();
    double gLength();
    PlaneType gPtype();
    CoverType gCtype();
    void defCover(const bool leftShow, const bool midShow, const bool rightShow);
    void defCover(const bool leftShow, const bool rightShow);
    void sLengths(const double Plength, const double Ploc);
    void sNorm(const Eigen::Vector3d Pcent);
    void sModul(const double Pstretch, const double Pangle);
    unsigned gPart();
    Eigen::Vector3d displace(const Eigen::Vector2d p);
    Plane(const int Ppart, const Eigen::Vector3d Pcent, const Eigen::Vector3d Ptang, const PlaneType Ptype = DEF);
    Plane(const int Ppart, const Eigen::Vector3d Pcent, const Eigen::Vector3d Ptang, const double Pscale, const Eigen::Vector3d Psdir);
};

class Loop
{ private:
    struct Vectset
    { Eigen::Vector3d ta,tb,tc,nb,la,lc,ca,cc;
      double da,dc,cosa, distance, outscale;
      bool iszero, ispi, isouter; };
    bool coverPresent;
    bool pure2D;
    const unsigned faces;
    unsigned pointCnt;
    unsigned partCnt;
    unsigned vertexCnt;
    unsigned edgeCnt;
    unsigned segmentCnt;
    std::vector<Step> steps;
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    std::vector<Plane> planes;
    std::vector<Segment> segments;
    Eigen::Vector3d pv(const unsigned i);
    Eigen::Vector3d cv(const unsigned i);
    Eigen::Vector3d nv(const unsigned i);
    Eigen::Vector3d pf(const unsigned i);
    Eigen::Vector3d cf(const unsigned i);
    Eigen::Vector3d nf(const unsigned i);
    double pp(const unsigned i, const unsigned j);
    double cp(const unsigned i, const unsigned j);
    double np(const unsigned i, const unsigned j);
    Eigen::Vector3d rotate(const double cosa, const Eigen::Vector3d axis, const Eigen::Vector3d vec);
    Eigen::Vector3d shift(const double amp, const Eigen::Vector3d base, const Eigen::Vector3d target);
    Eigen::Vector3d linearSearch(const double t, const Vector3Dvector vs);
    void linear(const unsigned n, const Eigen::Vector3d vs, const Eigen::Vector3d ve);
    Eigen::Vector3d bezierFunc(const double t, const Vector3Dvector vs);
    void bezier(const bool edge, const Vector3Dvector vs);
    unsigned mod(const int i);
    unsigned modf(const int i);
    unsigned modg(const int i);
    unsigned modh(const int i);
    void addStep(const Eigen::Vector3d step);
    void verify();
    bool filterPlanes(double maxr, const unsigned i);
    void fillvectset(const unsigned i, const double maxr, Vectset & vs);
  protected:
    void calcBase();
    void calcBase2D();
    void calcBase3D();
    void calcVertexDef(const unsigned i);
    void calcVertexLin(const unsigned i);
    void calcVertexArc(const unsigned i);
    void calcVertexBez(const unsigned i);
    void calcEdgeDef(const unsigned i);
    void calcEdgeLin(const unsigned i);
    void calcEdgeBez(const unsigned i);
    void calcEdgeSin(const unsigned i);
    void calcSegmentDef(const unsigned i);
    void calcSegmentLin(const unsigned i);
    void calcSegmentBez(const unsigned i);
    void calcSegmentSin(const unsigned i);
    void calcExtrusionPlane(double maxr, const unsigned i);
    void calcFrame();
  public:
    enum PolyType {NONE,RIN,ROUT,SIDE};
    enum OptionType {UNKNOWN,POINTS,POLY,VERTICES,EDGES,SEGMENTS};
    Loop(const unsigned faces);
    unsigned gPlaneCount();
    void addPoint(const Eigen::Vector3d pnt);
    void addPoints(const Vector3Dvector pnts);
    void addRpoly(const unsigned vcnt, const PolyType ptype, const double size, const bool flat, const bool clock);
    void addVertex(const Vertex::VertexType vtype, const std::vector<int> pnts, const std::vector<double> pars);
    void addEdge(const Edge::EdgeType etype, const Edge::TransType ttype, const std::vector<int> lnps, const std::vector<double> dbls, const Vector3Dvector bzps);
    void addSegment(const Segment::SegmentType stype, const Segment::TransType ttype, const bool show, const bool hide, const bool out, const bool in, const std::vector<int> snps, const std::vector<double> dbls, const Vector3Dvector bzps);
    void addGeneric(const OptionType Otype, const std::vector<int> ints, const std::vector<double> dbls, const std::vector<std::string> strs, const Vector3Dvector vcts);
    void construct(const bool Ppure2D);
    void extrude(const double maxr);
    Eigen::Vector2d gStep(unsigned i);
    unsigned gStepSize();
    void extrudeTransform(const int plane, const Eigen::Vector2d & mask, double result[]);
    void planeTransform(const int plane, const Eigen::Vector3d & mask, double result[]);
    bool planeIsBottom(const int plane);
    bool planeIsTop(const int plane);
    bool hullIsVisible(const int plane);
    bool hasCovers();
  public:
    static void print(const std::string s, const Eigen::Vector3d a, const int index = -1);
    static void print(const std::string s, const Eigen::Vector2d a, const int index = -1);
    static double epsilon();
    static double minr();
    static double saved();
    static PolyType stringsHasPolyType(const std::vector<std::string> strs, const PolyType deftype = Loop::NONE);
    static OptionType stringsHasOptionType(const std::vector<std::string> strs, const OptionType deftype = Loop::UNKNOWN); };


class Strip
{ private :
    const Value vvalue;
    const bool deep;
  unsigned pntr;
  unsigned count;
  public :
    unsigned gCount();
    std::vector<int> ints;
    std::vector<double> dbls;
    std::vector<std::string> strs;
    Vector3Dvector vcts;
    Strip(const Value & Pvvalue, const bool Pdeep) ;
    bool peel();
    void print();
    void process(Loop & loop, const Loop::OptionType Otype); };


#endif /* LOOP_H_ */
