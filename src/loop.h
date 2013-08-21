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


template <class T>  class Circular
{ private :
    std::vector<T> vec;
  public :
    Circular();
    void reserve(const unsigned size);
    void add(const T & val);
    void del(const unsigned index);
    unsigned size() const;
    bool isEmpty() const;
    void clear();
    T const& first() const;
    T const& last() const;
    T & operator[](const int index);
    T const& operator[](const int index) const;
};

class Lib
{ public :
    template <class T> static bool has(const std::vector<T> vec, const T val);
    template <class T> static bool msz(const std::vector< std::vector<T> > vecs, const int minsize);
    static bool isEven(unsigned i);
    static bool isOdd(unsigned i);
    static double rad(const double deg);
    static double deg(const double rad); };


class Log
{ public:
    enum ResultType {SUCCES,WARN,FAIL};
  public:
    ResultType result;
    std::vector<std::string> messages;
  public:
    Log();
    void addLog(const ResultType newResult, const std::string entry); };


class Vertex
{ public:
    enum VertexType {DEF,LIN,ARC,BEZ};
  private:
    Log * log;
    VertexType vtype;
    double pars[4];
    Eigen::Vector3d loc;
  public:
    Eigen::Vector3d gLoc() const;
    double gPar(const unsigned i) const;
    VertexType gVtype() const;
    Vertex(Log * Plog, const Eigen::Vector3d Ploc);
    void specify(const VertexType Pvtype, const std::vector<double> Ppars);
    void reset();
  public:
    static VertexType stringsHasVertexType(const std::vector<std::string> strs, const VertexType deftype = Vertex::DEF); };


class Edge
{ public:
    enum EdgeType {DEF,LIN,BEZ,SIN};
    enum TransType {IDN,ROT,RTX,RTY,RXY};
  private:
    Log * log;
    EdgeType etype;
    TransType ttype;
    double pars[6];
    Vector3Dvector bzps;
    Eigen::Matrix3d base;
  public:
    Eigen::Matrix3d baseRotation(const double scale) const;
    Eigen::Vector3d baseTranslation(const Eigen::Vector3d t) const;
    Vector3Dvector gBzps() const;
    Eigen::Matrix3d gBase() const;
    void sBase(const Eigen::Vector3d & x, const Eigen::Vector3d & y, const Eigen::Vector3d & z);
    double gPar(const unsigned i) const;
    EdgeType gEtype() const;
    TransType gTtype() const;
    Edge(Log * Plog);
    void specify(const EdgeType Petype, const TransType Pttype, const std::vector<double> Ppars, const Vector3Dvector Pbzps);
    void reset();
  public:
    static EdgeType stringsHasEdgeType(const std::vector<std::string> strs, const EdgeType deftype = Edge::DEF);
    static TransType stringsHasTransType(const std::vector<std::string> strs, const TransType deftype = Edge::IDN); };


class Segment
{ public:
    enum SegmentType {SPEC,DEF,LIN,BEZ,SIN};
    enum TransType {IDN,ROT,RTL,RLR};
  private:
    Log * log;
    bool show;
    bool outer;
    bool abscrv;
    SegmentType stype;
    TransType ttype;
    double pars[6];
    double length;
    double outscale;
    Vector3Dvector bzps;
  public:
    Vector3Dvector gBzps() const;
    double gPar(const unsigned i) const;
    double gLength() const;
    void addToLength(const double len);
    SegmentType gStype() const;
    TransType gTtype() const;
    bool gOuter() const;
    bool gShow() const;
    double gOutscale(const double maxr) const;
    Segment(Log * Plog);
    void specify(const SegmentType Pstype, const TransType Pttype, const std::vector<double> Ppars, const std::vector<std::string> strs, const Vector3Dvector Pbzps);
    void reset();
  public:
    static SegmentType stringsHasSegmentType(const std::vector<std::string> strs, const SegmentType deftype = Segment::SPEC);
    static TransType stringsHasTransType(const std::vector<std::string> strs, const TransType deftype = Segment::IDN); };

class Step
{ private :
    bool valid;
    Eigen::Vector3d loc;
    unsigned partnr;
  public :
    Eigen::Vector3d gLoc() const;
    unsigned gPartnr() const;
    void invalidate();
    bool isValid() const;
    Step(const Eigen::Vector3d loc, const unsigned partnr); };


class Plane
{ public :
    enum PlaneType {DEF,OUT,AUX};
    enum CoverType {NONE,TOP,BOTTOM,RING};
  private :
    Log * log;
    PlaneType ptype;
    CoverType ctype;
    unsigned part;
    Eigen::Vector3d cent;
    Eigen::Vector3d tang;
    Eigen::Vector3d norm;
    Eigen::Vector3d sdir;
    double loc, length, scale, stretch, angle;
  public :
    Eigen::Vector3d gCent() const;
    Eigen::Vector3d gTang() const;
    Eigen::Vector3d gNorm() const;
    Eigen::Vector3d gBnrm() const;
    Eigen::Vector3d gSdir() const;
    double gScale() const;
    double gLoc() const;
    double gLength() const;
    PlaneType gPtype() const;
    CoverType gCtype() const;
    void defCover(const bool leftShow, const bool rightShow);
    void sLengths(const double Plength, const double Ploc);
    void sNorm(const Eigen::Vector3d Pcent);
    void sModul(const double Pstretch, const double Pangle);
    unsigned gPart();
    Eigen::Vector3d displace(const Eigen::Vector2d p);
    Plane(Log * Plog, const unsigned Ppart, const Eigen::Vector3d Pcent, const Eigen::Vector3d Ptang, const PlaneType Ptype = DEF);
    Plane(Log * Plog, const unsigned Ppart, const Eigen::Vector3d Pcent, const Eigen::Vector3d Ptang, const double Pscale, const Eigen::Vector3d Psdir); };

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
    Circular<Step> steps;
    Circular<Vertex> vertices;
    Circular<Edge> edges;
    Circular<Plane> planes;
    Circular<Segment> segments;
    Log logger;
    Log * log;
    Eigen::Vector3d pv(const unsigned i) const;
    Eigen::Vector3d cv(const unsigned i) const;
    Eigen::Vector3d nv(const unsigned i) const;
    Eigen::Vector3d pf(const unsigned i) const;
    Eigen::Vector3d cf(const unsigned i) const;
    Eigen::Vector3d nf(const unsigned i) const;
    long double polyVal(const unsigned prsCnt, const long double x, const Circular<double> & prs) const;
    long double polyDif(const unsigned prsCnt, const long double x, const Circular<double> & prs) const;
    void polyInner(const unsigned vcnt, const Circular<double> prs, Circular<double> & radii, Circular<double> & angles);
    void polyOuter(const unsigned vcnt, const Circular<double> prs, Circular<double> & radii, Circular<double> & angles);
    void polyCyclic(const unsigned vcnt, const Circular<double> prs, Circular<double> & radii, Circular<double> & angles);
    double pp(const unsigned i, const unsigned j) const;
    double cp(const unsigned i, const unsigned j) const;
    double np(const unsigned i, const unsigned j) const;
    void baseSet(const unsigned i, Eigen::Vector3d & p,  Eigen::Vector3d & q,  Eigen::Vector3d & s,  Eigen::Vector3d & b,  Eigen::Vector3d & t, Eigen::Matrix3d & A) const;
    Eigen::Vector3d rotate(const double cosa, const Eigen::Vector3d axis, const Eigen::Vector3d vec) const;
    Eigen::Vector3d shift(const double amp, const Eigen::Vector3d base, const Eigen::Vector3d target) const;
    Eigen::Vector3d linearSearch(const double t, const Vector3Dvector vs) const;
    void linear(const unsigned n, const Eigen::Vector3d vs, const Eigen::Vector3d ve);
    Eigen::Vector3d bezierFunc(const double t, const Vector3Dvector vs);
    void bezier(const bool edge, const Vector3Dvector vs);
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
    void calcSegmentLinBez(const unsigned i, const bool isBez);
    void calcSegmentSin(const unsigned i);
    void calcExtrusionPlane(double maxr, const unsigned i);
    void calcFrame();
  public:
    enum ResultType {SUCCES,WARN,FAIL};
    enum CoorType {CARTESIAN,CYLINDER,SPHERE};
    enum PolyType {NONE,RIN,ROUT,SIDE};
    enum OptionType {UNKNOWN,POINTS,POLY,RECT,VERTICES,EDGES,SEGMENTS};
    Loop(const unsigned faces);
    unsigned gPlaneCount() const;
    void addPoint(const Eigen::Vector3d pnt);
    void addPoints(const CoorType ctype, const Vector3Dvector pnts);
    void addRect(const std::vector<std::string> strs, const Vector3Dvector pnts);
    void addPoly(std::vector<int> ints, const PolyType ptype, const std::vector<double> pars, const std::vector<std::string> strs);
    void addVertex(const Vertex::VertexType vtype, const std::vector<int> pnts, const std::vector<double> pars);
    void addEdge(const Edge::EdgeType etype, const Edge::TransType ttype, const std::vector<int> lnps, const std::vector<double> dbls, const Vector3Dvector bzps);
    void addSegment(const Segment::SegmentType stype, const Segment::TransType ttype, const std::vector<int> snps, const std::vector<double> dbls, const std::vector<std::string> strs, const Vector3Dvector bzps);
    void addGeneric(const OptionType Otype, const std::vector<int> ints, const std::vector<double> dbls, const std::vector<std::string> strs, const Vector3Dvector vcts);
    void construct(const bool Ppure2D);
    void extrude(const double maxr);
    Eigen::Vector2d gStep(unsigned i);
    unsigned gStepSize() const;
    void extrudeTransform(const int plane, const Eigen::Vector2d & mask, double result[]);
    void planeTransform(const int plane, const Eigen::Vector3d & mask, double result[]);
    bool planeIsBottom(const int plane) const;
    bool planeIsTop(const int plane) const;
    bool hullIsVisible(const int plane) const;
    bool hasCovers() const;
    ResultType gResult() const;
    std::vector<std::string> gMessages() const;
  public:
    static void print(const std::string s, const Eigen::Vector3d a, const int index = -1);
    static void print(const std::string s, const Eigen::Vector2d a, const int index = -1);
    static double epsilon();
    static double minr();
    static double saved();
    static PolyType stringsHasPolyType(const std::vector<std::string> strs, const PolyType deftype = Loop::NONE);
    static CoorType stringsHasCoorType(const std::vector<std::string> strs, const CoorType deftype = Loop::CARTESIAN);
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
