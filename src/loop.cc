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

#include "loop.h"

/* Class : Lib ================================================================================================================= */

template <class T> bool Lib::has(const std::vector<T> vec, const T val)
{ return std::find(vec.begin(), vec.end(), val)!=vec.end();  };

template <class T> bool Lib::msz(const std::vector< std::vector<T> > vecs, const int minsize)
{ for (unsigned i=0; i<vecs.size(); i++) { if (vecs[i].size()>minsize) { return false; }  }
  return true; }

bool Lib::isEven(unsigned i) { return i%2==0; }
bool Lib::isOdd(unsigned i)  { return i%2==1; }


/* Class : Strip =============================================================================================================== */

unsigned Strip::gCount() { return count; }

Strip::Strip(const Value & Pvvalue, const bool Pdeep) : vvalue(Pvvalue), deep(Pdeep), pntr(0) , count(0) {};

bool Strip::peel()
{ if (vvalue.type() != Value::VECTOR) { return false; }
  ints.clear();
  dbls.clear();
  strs.clear();
  vcts.clear();
  double x,y,z;
  bool abort = false;
  const Value::VectorType pvalue = vvalue.toVector();
  for (count++; pntr<pvalue.size() && !abort; pntr++)
  { if ((pvalue)[pntr].type() == Value::NUMBER) { ints.push_back(round((pvalue)[pntr].toDouble())); }
    if ((pvalue)[pntr].type() == Value::STRING) { strs.push_back((pvalue)[pntr].toString()); }
    if ((pvalue)[pntr].type() == Value::VECTOR)
    { if (deep)
      { const Value::VectorType qvalue =  (pvalue)[pntr].toVector() ;
      for (unsigned j=0; j<qvalue.size(); j++)
        { if ((qvalue)[j].type() == Value::NUMBER) { dbls.push_back((qvalue)[j].toDouble()); }
          if ((qvalue)[j].type() == Value::STRING) { strs.push_back((qvalue)[j].toString()); }
          if ((qvalue)[j].type() == Value::VECTOR) { if ((qvalue)[j].getVec3(x,y,z)) { vcts.push_back(Eigen::Vector3d(x,y,z)); } } }
      abort = true; }
      else
      { if ((pvalue)[pntr].getVec3(x,y,z)) { vcts.push_back(Eigen::Vector3d(x,y,z)); } } } }
  return !ints.empty() || !dbls.empty() || !strs.empty() || !vcts.empty() ; }

void Strip::print()
{ for (unsigned i=0; i<ints.size(); i++) { std::cerr << "ints[" <<  i << "] = " << ints[i] << "\n"; }
  for (unsigned i=0; i<dbls.size(); i++) { std::cerr << "dbls[" <<  i << "] = " << dbls[i] << "\n"; }
  for (unsigned i=0; i<strs.size(); i++) { std::cerr << "strs[" <<  i << "] = " << strs[i] << "\n"; }
  for (unsigned i=0; i<vcts.size(); i++) { std::cerr << "vcts[" <<  i << "] = (" << vcts[i][0] << "," << vcts[i][1] << ")\n"; } }

void Strip::process(Loop & loop, const Loop::OptionType Otype)
{ while (peel())
    { //print();
      loop.addGeneric(Otype,ints,dbls,strs,vcts); } }


/* Class : Vertex ============================================================================================================== */

Vertex::Vertex(Eigen::Vector3d Ploc) : vtype(DEF),  pars(), loc(Ploc) { };

void Vertex::specify(const VertexType Pvtype, const std::vector<double> Ppars)
{ vtype = Pvtype;
  unsigned count = Ppars.size();
  for (unsigned i=0; i<4; i++)
  { if (i<count) { pars[i]=Ppars[i]; } else { pars[i] = 0; } }
  switch (Pvtype)
  { case DEF :
     break;
    case LIN :
      if (count<2) { pars[1] = pars[0]; }
      if (count<1) { std::cerr << "Loop: to few parameters for linear construction. \n"; }
      break;
    case ARC :
      if (count<3) { pars[2] = 0; }
      if (count<2) { pars[1] = pars[0]; }
      if (count<1) { std::cerr << "Loop: to few parameters for arc construction. \n"; }
      break;
    case BEZ :
      if (count<4) { pars[3] = pars[2]; }
      if (count<3) { pars[3] = 1; pars[2] = 1; }
      if (count<2) { pars[3] = 1; pars[2] = 1; pars[1] = pars[0]; }
      if (count<1) { std::cerr << "Loop: to few parameters for bezier construction. \n"; }
      break; } }

double Vertex::gPar(const unsigned i) { return ( (i<4) ? pars[i] : 0 ); }

Vertex::VertexType Vertex::gVtype() { return vtype; }

void Vertex::reset()
{ vtype = DEF;
 for (unsigned i=0; i<6; i++) { pars[i]=0; } }

Vertex::VertexType Vertex::stringsHasVertexType(const std::vector<std::string> strs, const VertexType deftype)
{ if (Lib::has<std::string>(strs,"def")) { return Vertex::DEF; }
  if (Lib::has<std::string>(strs,"lin")) { return Vertex::LIN; }
  if (Lib::has<std::string>(strs,"arc")) { return Vertex::ARC; }
  if (Lib::has<std::string>(strs,"bez")) { return Vertex::BEZ; }
  return deftype; }


/* Class : Edge ================================================================================================================ */

Edge::Edge() : etype(DEF),  ttype(IDN), pars(), bzps(), base() { };

void Edge::specify(const EdgeType Petype, const TransType Pttype, const std::vector<double> Ppars, const Vector3Dvector Pbzps)
{ etype = Petype;
  ttype = Pttype;
  unsigned count = Ppars.size();
  for (unsigned i=0; i<6; i++)
  { if (i<count) { pars[i]=Ppars[i]; } else { pars[i] = 0; } }
  switch (Petype)
  { case DEF :
      bzps.clear();
      break;
    case LIN :
    case BEZ :
      bzps = Pbzps;
      break;
    case SIN :
      if (ttype == IDN) { ttype = ROT; }
      if (count<3) { pars[2]=1; }
      if (count<2) { std::cerr << "Loop: to few parameters for wave construction. \n"; }
      break; } }

double Edge::gPar(const unsigned i) { return ( (i<6) ? pars[i] : 0 ); }

Edge::EdgeType Edge::gEtype()  { return etype; }
Edge::TransType Edge::gTtype() { return ttype; }

void Edge::reset()
{ etype = DEF;
  bzps.clear();
  for (unsigned i=0; i<6; i++) { pars[i]=0; } }

Eigen::Matrix3d Edge::baseRotation(const double scale)
{ Eigen::Matrix3d R;
  switch (ttype)
  { case IDN : R = Eigen::Matrix3d::Identity();                               break;
    case ROT : R = base;                                                      break;
    case RTX : R << scale*base.col(0), base.col(1), base.col(2);              break;
    case RTY : R << base.col(0), scale*base.col(1), scale*base.col(2);        break;
    case RXY : R << scale*base.col(0), scale*base.col(1), scale*base.col(2);  break; }
  return R; }

Eigen::Vector3d Edge::baseTranslation(const Eigen::Vector3d t)
{ Eigen::Vector3d r;
  switch (ttype)
  { case IDN : r << 0,0,0;  break;
    case ROT : r = t;       break;
    case RTX : r = t;       break;
    case RTY : r = t;       break;
    case RXY : r = t;       break; }
  return r; }

Edge::EdgeType Edge::stringsHasEdgeType(const std::vector<std::string> strs, const EdgeType deftype)
{ if (Lib::has<std::string>(strs,"def")) { return Edge::DEF; }
  if (Lib::has<std::string>(strs,"lin")) { return Edge::LIN; }
  if (Lib::has<std::string>(strs,"bez")) { return Edge::BEZ; }
  if (Lib::has<std::string>(strs,"wav")) { return Edge::SIN; }
  return deftype; }

Edge::TransType Edge::stringsHasTransType(const std::vector<std::string> strs, const TransType deftype)
{ if (Lib::has<std::string>(strs,"iden")) { return Edge::IDN; }
  if (Lib::has<std::string>(strs,"cen"))  { return Edge::ROT; }
  if (Lib::has<std::string>(strs,"csx"))  { return Edge::RTX; }
  if (Lib::has<std::string>(strs,"csy"))  { return Edge::RTY; }
  if (Lib::has<std::string>(strs,"csxy")) { return Edge::RXY; }
  return deftype; }

/* Class : Segment ============================================================================================================= */

double Segment::gPar(const unsigned i) { return ( (i<6) ? pars[i] : 0 ); }

Segment::SegmentType Segment::gStype()  { return stype; }
Segment::TransType Segment::gTtype()    { return ttype; }

bool Segment::gOuter() { return outer; }
bool Segment::gShow()  { return show; }

double  Segment::gLength() { return length; }
void  Segment::addToLength(const double len)  { length += len; }

Segment::Segment() : show(true), outer(false), stype(DEF),  ttype(IDN),  pars(), length(0), bzps() { pars[0]=1; };

void Segment::specify(const SegmentType Pstype, const TransType Pttype, const std::vector<double> Ppars, const Vector3Dvector Pbzps)
{ stype = Pstype;
  ttype = Pttype;
  unsigned count = Ppars.size();
  for (unsigned i=0; i<6; i++)
  { if (i<count) { pars[i]=Ppars[i]; } else { pars[i] = 0; } }
  switch (Pstype)
  { case DEF :
      bzps.clear();
      break;
    case LIN :
    case BEZ :
      bzps = Pbzps;
      break;
    case SIN :
      if (ttype == IDN) { ttype = ROT; }
      if (count<5) { pars[5]=1; }
      if (count<3) { pars[2]=1; }
      if (count<2) { std::cerr << "Loop: to few parameters for wave construction. \n"; }
      break;
  } }

void Segment::specify(const bool show, const bool hide, const bool out, const bool in, const std::vector<double> Ppars)
{ if (show) { this->show = true; }
  if (hide) { this->show = false; }
  if (out)  { this->outer = true; }
  if (in)   { this->outer = false; }
  if (out || in) { if (Ppars.size()==0) { pars[0]=1; } else { pars[0]=Ppars[0]; } } }

void Segment::reset()
{ stype = DEF;
  bzps.clear();
  for (unsigned i=0; i<6; i++) { pars[i]=0; }
  pars[0]=1; }

Segment::SegmentType Segment::stringsHasSegmentType(const std::vector<std::string> strs, const SegmentType deftype)
{ if (Lib::has<std::string>(strs,"def")) { return Segment::DEF; }
  if (Lib::has<std::string>(strs,"lin")) { return Segment::LIN; }
  if (Lib::has<std::string>(strs,"bez")) { return Segment::BEZ; }
  if (Lib::has<std::string>(strs,"wav")) { return Segment::SIN; }
  return deftype; }

Segment::TransType Segment::stringsHasTransType(const std::vector<std::string> strs, const TransType deftype)
{ if (Lib::has<std::string>(strs,"iden")) { return Segment::IDN; }
  if (Lib::has<std::string>(strs,"cen"))  { return Segment::ROT; }
  if (Lib::has<std::string>(strs,"csl"))  { return Segment::RTL; }
  if (Lib::has<std::string>(strs,"cslr")) { return Segment::RLR; }
  return deftype; }


/* Class : Plane =============================================================================================================== */

Plane::Plane(const int Ppart, const Eigen::Vector3d Pcent, const Eigen::Vector3d Ptang, const PlaneType Ptype) :
  ptype(Ptype), ctype(NONE), part(Ppart), cent(Pcent), tang(Ptang), norm(), sdir(), loc(0), length(0), scale(1), stretch(1), angle(0) {};

Plane::Plane(const int Ppart, const Eigen::Vector3d Pcent, const Eigen::Vector3d Ptang, const double Pscale, const Eigen::Vector3d Psdir) :
  ptype(OUT), ctype(NONE), part(Ppart), cent(Pcent), tang(Ptang), norm(), sdir(Psdir), loc(0), length(0), scale(Pscale), stretch(1), angle(0) {};

Eigen::Vector3d Plane::gCent() { return cent; }
Eigen::Vector3d Plane::gTang() { return tang; }
Eigen::Vector3d Plane::gNorm() { return norm; }
Eigen::Vector3d Plane::gBnrm() { return tang.cross(norm); }
Eigen::Vector3d Plane::gSdir() { return sdir; }
double Plane::gLength() { return length; }
double Plane::gLoc() { return loc; }

Plane::CoverType Plane::gCtype() { return ctype; }

void Plane::defCover(const bool leftShow, const bool midShow, const bool rightShow)
{ if ( Lib::isOdd(part) || ((leftShow==midShow) && (midShow==rightShow)) )
  { if (midShow) { ctype=RING; } else { ctype=NONE; }  }
  else
  { if ((leftShow!=midShow) && (midShow==rightShow))
    { ctype=TOP; }
    else if ((leftShow==midShow) && (midShow!=rightShow))
    { ctype=BOTTOM; }
    else
    { /* This situation should not appear. */
      ctype=NONE; } } }

void Plane::defCover(const bool leftShow, const bool rightShow)
{ if ((leftShow) && (rightShow))       { ctype=RING;   }
  else if ((leftShow) && (!rightShow)) { ctype=TOP;    }
  else if ((!leftShow) && (rightShow)) { ctype=BOTTOM; }
  else                                 { ctype=NONE;   } }

double Plane::gScale() { return scale; }
Plane::PlaneType Plane::gPtype() { return ptype; }


void Plane::sLengths(const double Plength, const double Ploc)
{ length = Plength;
  loc = Ploc; }

void Plane::sNorm(const Eigen::Vector3d Pnorm) { norm = Pnorm; }
void Plane::sModul(const double Pstretch, const double Pangle) { stretch = Pstretch; angle=Pangle; }
unsigned Plane::gPart() { return part; }


Eigen::Vector3d Plane::displace(const Eigen::Vector2d p)
{ Eigen::Matrix3d A;
  Eigen::Matrix2d T = Eigen::Matrix2d::Identity();
  Eigen::Matrix2d R ;
  R << cos(angle),sin(angle),-sin(angle),cos(angle);
  if (ptype == OUT)
  { double c = norm.dot(sdir);
    double s = gBnrm().dot(sdir);
    T << (c*c*scale+s*s),(c*s*(1-scale)),(c*s*(1-scale)),(s*s*scale+c*c) ;  }
  Eigen::Vector2d q = stretch * R * T * p;
  Eigen::Vector3d w(q[0], q[1], 0);
  A << norm , gBnrm() , tang;
  return cent + A * w; }


/* Class : Step ================================================================================================================ */


Step::Step(const Eigen::Vector3d Ploc, const unsigned Ppartnr) : loc(Ploc), partnr(Ppartnr), valid(true) {};


/* Class : Loop ================================================================================================================ */

Loop::Loop(const unsigned faces) : coverPresent(false), pure2D(true) ,faces(faces), pointCnt(0), partCnt(0), vertexCnt(0), edgeCnt(0), segmentCnt(0), steps(), vertices(), edges(), planes(), segments() {};

Eigen::Vector3d Loop::pv(const unsigned i) { return vertices[mod((int)i-1)].loc; }
Eigen::Vector3d Loop::cv(const unsigned i) { return vertices[mod((int)i)].loc; }
Eigen::Vector3d Loop::nv(const unsigned i) { return vertices[mod((int)i+1)].loc; }

double Loop::pp(const unsigned i, const unsigned j) { return vertices[mod((int)i-1)].gPar(j); }
double Loop::cp(const unsigned i, const unsigned j) { return vertices[mod((int)i)].gPar(j); }
double Loop::np(const unsigned i, const unsigned j) { return vertices[mod((int)i+1)].gPar(j); }

unsigned Loop::mod(const int i) { return (i >= 0) ? (i % pointCnt) : (((i + pointCnt) % pointCnt));  }

bool Loop::hasCovers() { return coverPresent; }

void Loop::addStep(const Eigen::Vector3d step)
{ steps.push_back(Step(step,partCnt)); }

void Loop::verify()
{ for (unsigned i=0; i<pointCnt; i++)
  { if (vertices[i].gVtype() != Vertex::DEF)
    { /* If the resulting curvature is too small ignore the Vertex. */
      if (cp(i,0)<minr())
      { vertices[i].reset(); }
      /* The length of the side must be enough to accommodate the reduction due to milling, otherwise ignore. */
      if ( (vertices[mod(i+1)].gPar(1)+cp(i,0)) + minr() > (nv(i)-cv(i)).norm() )
      { vertices[i].reset();
        vertices[mod(i+1)].reset(); }
    /* Should we check for simple polygons using Shamos-Hoey? */ }
  { if (edges[i].gEtype() == Edge::SIN)
    { if (edges[i].gPar(0)<minr()) { edges[i].reset(); }
      if (edges[i].gPar(1)<minr()) { edges[i].reset(); }
      if (edges[i].gPar(2)<minr()) { edges[i].reset(); } } } } }

Eigen::Vector3d Loop::shift(const double amp, const Eigen::Vector3d base, const Eigen::Vector3d target)
{ return base - amp*(base-target).normalized(); }

void Loop::calcVertexDef(const unsigned i)
{ addStep(cv(i)); }

void Loop::calcVertexLin(const unsigned i)
{ addStep(shift(cp(i,0),cv(i),pv(i)));
  addStep(shift(cp(i,1),cv(i),nv(i))); }

void Loop::calcVertexArc(const unsigned i)
{ Eigen::Vector3d s,p,q,r;
  Eigen::Matrix3d A;
  double qr = cp(i,2);
  double pr = qr + sqrt(qr*qr+1);
  p = shift(cp(i,0),cv(i),pv(i));
  q = shift(cp(i,1),cv(i),nv(i));
  r = p.cross(q);
  s = (p + q) / 2;
  A << (q-s),(cv(i)-s),r;
  for (unsigned j=0; j<=faces; j++)
  { double x = 2*((double) j / faces) - 1;
    double y = sqrt( pr * (pr+1-x*x) ) - pr;
    Eigen::Vector3d w(x,y,0);
    addStep(s+A*w); } }

void Loop::calcVertexBez(const unsigned i)
{ Vector3Dvector vs;
  vs.push_back(shift(cp(i,0),cv(i),pv(i)));
  vs.push_back(shift(cp(i,0)*(1-cp(i,2)),cv(i),pv(i)));
  vs.push_back(shift(cp(i,1)*(1-cp(i,3)),cv(i),nv(i)));
  vs.push_back(shift(cp(i,1),cv(i),nv(i)));
  bezier(true,vs); }

Eigen::Vector3d Loop::bezierFunc(const double t, const Vector3Dvector vs)
{ Eigen::Vector3d s(0,0,0);
  unsigned n = vs.size() - 1;
  double ts = (t<0.5) ? (1 - t) : t;
  double cf = pow(ts,n);
  for (unsigned j=0; j<=n; j++)
  { s = s + cf * ( (t<0.5) ? vs[j] : vs[n-j]) ;
    cf = cf * (n-j) / (j+1) * (1-ts) / ts; }
  return s; }

void Loop::bezier(const bool edge, const Vector3Dvector vs)
{ unsigned e = edge ? 0 : 1;
  for (unsigned i=e; i<=(faces-e); i++) { addStep(bezierFunc( ((double) i / faces) ,vs)); } }

void Loop::linear(const unsigned n, const Eigen::Vector3d vs, const Eigen::Vector3d ve)
{ unsigned subfaces = faces / n;
  for (unsigned i=1; i<subfaces; i++)
  { double t = (double) i / subfaces;
    addStep((1-t)*vs + t*ve); } }

void Loop::calcBase()
{ Eigen::Vector3d t,n,b;
  for (unsigned i=0; i<pointCnt; i++)
  { t = (nv(i)-cv(i)).normalized();
    if (pure2D)
    { n << -t[1],t[0],0;
      b << 0,0,1; }
    else
    { n = (nv(i)-cv(i)).cross(cv(i)-pv(i)).normalized();
      b = t.cross(n); }
    edges[i].base << t,n,b; } }

Eigen::Vector3d Loop::rotate(const double cosa, const Eigen::Vector3d axis, const Eigen::Vector3d vec)
{ Eigen::Vector3d result,bn;
  double sina = sqrt(1-cosa*cosa);
  if (axis.norm()>epsilon())
  { Eigen::Matrix3d R;
    bn = axis.normalized();
    R <<       (cosa+bn[0]*bn[0]*(1-cosa)) , (bn[0]*bn[1]*(1-cosa)-sina*bn[2]) , (bn[2]*bn[0]*(1-cosa)+sina*bn[1]) ,
         (bn[0]*bn[1]*(1-cosa)+sina*bn[2]) ,       (cosa+bn[1]*bn[1]*(1-cosa)) , (bn[2]*bn[1]*(1-cosa)-sina*bn[0]) ,
         (bn[0]*bn[2]*(1-cosa)-sina*bn[1]) , (bn[1]*bn[2]*(1-cosa)+sina*bn[0]) ,       (cosa+bn[2]*bn[2]*(1-cosa)) ;
    result = R * vec; }
  else
  { result = vec; }
  return result; }

void Loop::calcBase2D()
{ Eigen::Vector3d t,n,b;
  for (unsigned i=0; i<pointCnt; i++)
  { t = (nv(i)-cv(i)).normalized();
    n << -t[1],t[0],0;
    b << 0,0,1;
    edges[i].base << t,n,b; } }

void Loop::calcBase3D()
{ Eigen::Vector3d tl,t,n;
  for (unsigned i=0; i<pointCnt; i++)
  { t = (nv(i)-cv(i)).normalized();
    if (i==0) { n = t.unitOrthogonal(); } else { n = rotate(tl.dot(t),tl.cross(t),n); }
    edges[i].base << t, n, t.cross(n);
    tl = t; } }

void Loop::calcEdgeDef(const unsigned i)
{ Eigen::Vector3d p,q;
  if (segments[2*i+1].gStype() != Segment::DEF)
  { p = shift(cp(i,1),cv(i),nv(i));
    q = shift(np(i,0),nv(i),cv(i));
    linear(1,p,q); } }

void Loop::calcEdgeLin(const unsigned i)
{ Eigen::Vector3d s,p,q,b,c,t;
  Eigen::Matrix3d A;
  Vector3Dvector vs;
  bool subdiv = (segments[2*i+1].gStype() != Segment::DEF);
  p = shift(cp(i,1),cv(i),nv(i));
  q = shift(np(i,0),nv(i),cv(i));
  s = (q + p) / 2;
  t = edges[i].baseTranslation(s);
  b = (q - p) / 2;
  A = edges[i].baseRotation(b.norm());
  c = p;
  for (unsigned j=0; j<edges[i].bzps.size(); j++)
  { Eigen::Vector3d b = t+A*edges[i].bzps[j];
    if (subdiv) { linear(edges[i].bzps.size()+1,c,b); c=b; }
    addStep(b); }
  if (subdiv) { linear(edges[i].bzps.size()+1,c,q); } }

void Loop::calcEdgeBez(const unsigned i)
{ Eigen::Vector3d s,p,q,b,t;
  Eigen::Matrix3d A;
  Vector3Dvector vs;
  p = shift(cp(i,1),cv(i),nv(i));
  q = shift(np(i,0),nv(i),cv(i));
  s = (q + p) / 2;
  b = (q - p) / 2;
  t = edges[i].baseTranslation(s);
  A = edges[i].baseRotation(b.norm());
  vs.push_back(p);
  for (unsigned j=0; j<edges[i].bzps.size(); j++)
  { Eigen::Vector3d b = edges[i].bzps[j];
    vs.push_back(t+A*b); }
  vs.push_back(q);
  bezier(false,vs); }

void Loop::calcEdgeSin(const unsigned i)
{ if (!pure2D) { return; }
  Eigen::Vector3d s,p,q,b,t;
  Eigen::Matrix3d A;
  double width = edges[i].gPar(0)/2;
  double amplitude = edges[i].gPar(1);
  double tops = edges[i].gPar(2);
  double slope = edges[i].gPar(3);
  double phase = edges[i].gPar(4);
  double height = edges[i].gPar(5);
  p = shift(cp(i,1),cv(i),nv(i));
  q = shift(np(i,0),nv(i),cv(i));
  s = (q + p) / 2;
  b = (q - p) / 2;
  t = edges[i].baseTranslation(s);
  A = edges[i].baseRotation(b.norm());
  bool useEnv = (slope>minr());
  bool xScaled = (edges[i].gTtype() == Edge::RTX) || (edges[i].gTtype() == Edge::RXY);
  double length = xScaled ? 1 : (p-q).norm()/2;
  double stretch = useEnv ? length : std::min(length,width);
  double period = 4 * width / tops;
  for (unsigned j=0; j<=faces; j++)
  { double x = (2*((double) j / faces) - 1) * stretch;
    double env = (useEnv) ? (1+exp(-slope*(width+x)))*(1+exp(-slope*(width-x))) : 1;
    double y = (amplitude * cos(2*M_PI*(x/period+phase)) + height);
    Eigen::Vector3d w(x,y/env,0);
    addStep(t+A*w); } }

void Loop::construct(const bool Ppure2D)
{ pure2D = Ppure2D;
  verify();
  if (pure2D) { calcBase2D(); } else { calcBase3D(); }
  steps.reserve(2*pointCnt*(faces+1));
  for (unsigned i=0; i<pointCnt; i++)
  { switch (vertices[i].gVtype())
    { case Vertex::DEF : calcVertexDef(i);  break;
      case Vertex::LIN : calcVertexLin(i);  break;
      case Vertex::ARC : calcVertexArc(i);  break;
      case Vertex::BEZ : calcVertexBez(i);  break; }
    partCnt++;
    switch (edges[i].gEtype())
    { case Edge::DEF : calcEdgeDef(i);      break;
      case Edge::LIN : calcEdgeLin(i);      break;
      case Edge::BEZ : calcEdgeBez(i);      break;
      case Edge::SIN : calcEdgeSin(i);      break; }
    partCnt++; } }

Eigen::Vector2d Loop::gStep(unsigned i) { return Eigen::Vector2d(steps[i].loc[0],steps[i].loc[1]);  }

unsigned Loop::gStepSize() { return steps.size(); }

void Loop::addPoint(const Eigen::Vector3d pnt)
{ if ( (vertices.size()==0) || (((pnt - vertices.front().loc).norm() > minr()) && ((pnt - vertices.back().loc).norm() > minr())) )
  { vertices.push_back(Vertex(pnt));
    edges.push_back(Edge());
    segments.push_back(Segment());
    segments.push_back(Segment());
    pointCnt++; } }

void Loop::addPoints(const Vector3Dvector pnts)
{ for (unsigned i=0; i<pnts.size(); i++) { addPoint(pnts[i]); } }

void Loop::addRpoly(const unsigned vcnt, const PolyType ptype, const double size, const bool flat, const bool clock)
{ if ((vcnt<3) || (size<minr()) || ptype == NONE) { return; }
  vertices.clear();
  vertices.reserve(vcnt);
  double gamma = M_PI/vcnt;
  double radius = 0;
  { switch (ptype)
    { case NONE : radius = size;                 break;
      case RIN  : radius = size/cos(gamma);      break;
      case ROUT : radius = size;                 break;
      case SIDE : radius = size/(2*sin(gamma));  break; } }
  double alpha = clock ? -2*gamma : 2*gamma;
  double beta  = (flat  ? -alpha/2 : 0) + ( clock ?  M_PI/2  : 0 );
  for (unsigned i=0; i<vcnt; i++)
  { Eigen::Vector3d p( radius*cos(alpha*i + beta), radius*sin(alpha*i + beta) , 0 );
    addPoint(p); } };

void Loop::addVertex(const Vertex::VertexType vtype, const std::vector<int> pnts, const std::vector<double> pars)
{ if (pnts.size() == 0)
  { for (unsigned i=vertexCnt; i<pointCnt; i++)
    { vertices[i].specify(vtype,pars); } }
  else
  { for (unsigned i=0; i<pnts.size(); i++)
    { unsigned index = pnts[i];
      if ((index > 0) && (index<=pointCnt)) { vertices[index-1].specify(vtype,pars); } } }
  vertexCnt++; }

void Loop::addEdge(const Edge::EdgeType etype, const Edge::TransType ttype, const std::vector<int> lnps, const std::vector<double> dbls, const Vector3Dvector bzps)
{ if (lnps.size() == 0)
  { for (unsigned i=edgeCnt; i<pointCnt; i++)
    { edges[i].specify(etype,ttype,dbls,bzps); } }
  else
  { for (unsigned i=0; i<lnps.size(); i++)
    { unsigned index = lnps[i];
      if ((index > 0) && (index<=pointCnt)) { edges[index-1].specify(etype,ttype,dbls,bzps); } } }
  edgeCnt++; }

void  Loop::addSegment(const Segment::SegmentType stype, const Segment::TransType ttype, const bool show, const bool hide, const bool out, const bool in, const std::vector<int> snps, const std::vector<double> dbls, const Vector3Dvector bzps)
{ if (snps.size() == 0)
  { for (unsigned i=segmentCnt; i<2*pointCnt; i++)
    { if (show || hide || out || in)
      { segments[i].specify(show, hide, out, in, dbls); }
      else
      { segments[i].specify(stype,ttype,dbls,bzps); } } }
  else
  { for (unsigned i=0; i<snps.size(); i++)
    { unsigned index = snps[i];
      if ((index > 0) && (index<=2*pointCnt))
      { if (show || hide || out || in)
           { segments[index-1].specify(show, hide, out, in, dbls); }
           else
           { segments[index-1].specify(stype,ttype,dbls,bzps); } } } }
  segmentCnt++; }


void Loop::addGeneric(const OptionType Otype, const std::vector<int> ints, const std::vector<double> dbls, const std::vector<std::string> strs, const Vector3Dvector vcts)
{ switch (Otype)
  { case UNKNOWN  : break;
    case POINTS   : addPoints(vcts);  break;
    case POLY     : if ((ints.size()>0)&&(dbls.size()>0)) { addRpoly(ints[0],Loop::stringsHasPolyType(strs,Loop::SIDE),dbls[0],Lib::has<std::string>(strs,"flat"),Lib::has<std::string>(strs,"clock")); } ;  break;
    case VERTICES : addVertex(Vertex::stringsHasVertexType(strs),ints,dbls);  break;
    case EDGES    : addEdge(Edge::stringsHasEdgeType(strs),Edge::stringsHasTransType(strs),ints,dbls,vcts);  break;
    case SEGMENTS : addSegment(Segment::stringsHasSegmentType(strs),Segment::stringsHasTransType(strs),Lib::has<std::string>(strs,"show"),Lib::has<std::string>(strs,"hide"),Lib::has<std::string>(strs,"out"),Lib::has<std::string>(strs,"in"),ints,dbls,vcts);  break; } }

Loop::PolyType Loop::stringsHasPolyType(const std::vector<std::string> strs, const PolyType deftype)
{ if (Lib::has<std::string>(strs,"rin"))  { return Loop::RIN;  }
  if (Lib::has<std::string>(strs,"rout")) { return Loop::ROUT; }
  if (Lib::has<std::string>(strs,"side")) { return Loop::SIDE; }
  return deftype; }

Loop::OptionType Loop::stringsHasOptionType(const std::vector<std::string> strs, const OptionType deftype)
{ if (Lib::has<std::string>(strs,"points"))   { return Loop::POINTS;   }
  if (Lib::has<std::string>(strs,"poly"))     { return Loop::POLY;     }
  if (Lib::has<std::string>(strs,"vertices")) { return Loop::VERTICES; }
  if (Lib::has<std::string>(strs,"edges"))    { return Loop::EDGES;    }
  if (Lib::has<std::string>(strs,"segments")) { return Loop::SEGMENTS; }
  return deftype; }

double Loop::minr() { return 0.001; }
double Loop::epsilon() { return 1e-8; }
double Loop::saved() { return 0.01; }

Eigen::Vector3d Loop::pf(const unsigned i) { return steps[modf((int)i-1)].loc; }
Eigen::Vector3d Loop::cf(const unsigned i) { return steps[modf((int)i)].loc; }
Eigen::Vector3d Loop::nf(const unsigned i) { return steps[modf((int)i+1)].loc; }
unsigned Loop::modf(const int i) { return (i >= 0) ? (i % steps.size()) : (((i + steps.size()) % steps.size()));  }
unsigned Loop::modg(const int i) { return (i >= 0) ? (i % planes.size()) : (((i + planes.size()) % planes.size()));  }
unsigned Loop::modh(const int i) { return (i >= 0) ? (i % segments.size()) : (((i + segments.size()) % segments.size()));  }

unsigned Loop::gPlaneCount() { return  planes.size(); }

void Loop::fillvectset(const unsigned i, const double maxr, Vectset & vs)
{ vs.da = (cf(i) - pf(i)).norm();
  vs.dc = (cf(i) - nf(i)).norm();
  vs.ca = (cf(i) + pf(i))/2;
  vs.ta = (cf(i) - pf(i)).normalized();
  vs.tc = (cf(i) - nf(i)).normalized();
  vs.tb = (vs.ta-vs.tc).normalized();
  vs.nb = (vs.ta+vs.tc).normalized();
  vs.cosa = (vs.ta.dot(vs.tc));
  vs.iszero = (1 - vs.cosa) < epsilon();
  vs.ispi = (1 + vs.cosa) < epsilon();
  vs.cc = cf(i);
  if ((!vs.iszero) && (!vs.ispi))
  { double ctnha = sqrt((1+vs.cosa)/(1-vs.cosa));
    double sinha = sqrt((1-vs.cosa)/2);
    double localr = maxr * segments[steps[modf(i)].partnr].gPar(0);
    vs.distance = saved() + localr * ctnha;
    vs.outscale = 1 / sinha;
    vs.la = shift(vs.distance,cf(i),pf(i));
    vs.lc = shift(vs.distance,cf(i),nf(i)); }
  else
  { vs.distance = 0;
    vs.outscale = 1;
    vs.la = cf(i);
    vs.lc = cf(i);  } }

bool Loop::filterPlanes(double maxr, const unsigned i)
{ bool result = false;
  Vectset cur;
  fillvectset(i,maxr,cur);
  if (cur.iszero)
  { steps[i].valid = false;
    result = true; }
  else if (cur.ispi)
  { /* Can not invalidate any point*/  }
  else
  { if ( (cur.distance>cur.da) && (steps[modf(i-1)].valid) )
    { steps[modf(i-1)].valid = false;
      result = true; }
    if  ( (cur.distance>cur.dc) && (steps[modf(i+1)].valid) )
    { steps[modf(i+1)].valid = false;
      result = true; } }
  return result; }

void Loop::calcExtrusionPlane(double maxr, const unsigned i)
{ Vectset prv,cur,nxt;
  Eigen::Matrix3d A;
  unsigned partNr = steps[i].partnr;
  bool outer = segments[partNr].gOuter();
  fillvectset(i,maxr,cur);
  if (cur.iszero)
  { /* Issue some kind of warning! */ }
  else if (cur.ispi)
  { planes.push_back(Plane(partNr,cur.la,cur.ta,Plane::AUX)); }
  else
  { fillvectset(i-1,maxr,prv);
    fillvectset(i+1,maxr,nxt);
    if (prv.distance+cur.distance<cur.da)
    { planes.push_back(Plane(partNr,cur.la,cur.ta)); }
    else
    { planes.push_back(Plane(partNr,cur.ca,cur.ta)); }
    // TODO Fix the out of plane center scaling, this gives incorrect results.
    if (outer) { planes.push_back(Plane(partNr,cur.cc,cur.tb,cur.outscale,cur.nb)); }
    if (nxt.distance+cur.distance<cur.dc)
    { planes.push_back(Plane(partNr,cur.lc,-cur.tc)); }
    else
    { /* Doe niets want dat gebeurd in de volgende ronde */ } } }

void Loop::calcFrame()
{ if (gPlaneCount()<=2) { return; }
  Eigen::Vector3d b,bn,ns;
  ns = planes[0].gTang().unitOrthogonal();
  planes[0].sNorm(ns);
  for (unsigned i=1; i<gPlaneCount(); i++)
  { Eigen::Vector3d b = planes[modg(i-1)].gTang().cross(planes[i].gTang());
    double c = planes[modg(i-1)].gTang().dot(planes[i].gTang());
    ns = rotate(c,b,ns);
    planes[i].sNorm(ns); }
  //TODO Fix the begin-end plane angle mismatch.
  unsigned st = 0;
  while ( (st<gPlaneCount()) && (planes[modg(st)].gPart() == planes[modg(st+1)].gPart()) ) { st++; }
  for (unsigned i=st; i<(gPlaneCount()+st); i++)
  { double length = (planes[modg(i+1)].gCent() - planes[modg(i)].gCent()).norm();
    unsigned thisPartNr = planes[modg(i)].gPart();
    unsigned nextPartNr = planes[modg(i+1)].gPart();
    planes[modg(i)].sLengths(length,segments[thisPartNr].gLength());
    if (thisPartNr == nextPartNr)
    { segments[thisPartNr].addToLength(length); }
    else if ( Lib::isEven(thisPartNr) && Lib::isOdd(nextPartNr) )
    { segments[nextPartNr].addToLength(length); }
    else if ( Lib::isEven(thisPartNr) && Lib::isEven(nextPartNr) )
    { segments[thisPartNr+1].addToLength(length); }
    else
    { segments[thisPartNr].addToLength(length); } }

  for (unsigned i=0; i<gPlaneCount(); i++)
  { unsigned prevPart = planes[modg(i-1)].gPart();
    unsigned thisPart = planes[modg(i)].gPart();
    unsigned nextPart = planes[modg(i+1)].gPart();
    /* odd segements directs give there visible state to the plane*/
    if (Lib::isOdd(thisPart))
    { planes[i].defCover(segments[thisPart].gShow(),segments[thisPart].gShow()); }
    else
    { /* determine if this is border */
      if ((prevPart==thisPart) && (thisPart==nextPart))
      { planes[i].defCover(segments[thisPart].gShow(),segments[thisPart].gShow()); }
      else
      { /* Yes it is, since this is an even plane it cannot have different parts on both sides */
        if ((prevPart==thisPart) && (thisPart!=nextPart))
        { /* The border is right*/
          planes[i].defCover(segments[thisPart].gShow(),segments[modh(thisPart+1)].gShow()); }
        else if ((prevPart!=thisPart) && (thisPart==nextPart))
        { /* The border is left*/
          planes[i].defCover(segments[modh(thisPart-1)].gShow(),segments[thisPart].gShow()); }
        else
        { /* Hmm, this is a deserted plane after all, lets remove it to draw attention. Should log it as well */
          std::cerr << "INTERNAL ERROR: deserted vertex plane, not drawing! ";
          planes[i].defCover(false,false); } } }
    coverPresent = coverPresent || (planes[i].gCtype() != Plane::RING); } }

Eigen::Vector3d Loop::linearSearch(const double t, const Vector3Dvector vs)
{ Eigen::Vector3d result(0,0,0);
  if (vs.size() > 0)
  { if (vs.size() == 1 )   { result = vs[0]; }
    else if (t <= vs.front()[0]) { result = vs.front(); }
    else if (t >= vs.back()[0]) { result = vs.back(); }
    else
    { unsigned i=1;
      while ((i<vs.size()) && (t>vs[i][0])) { i++; }
      if (std::fabs(vs[i-1][0] - vs[i][0])>minr())
      { double r = (t -  vs[i-1][0])/(vs[i][0] -  vs[i-1][0]);
        result = (1-r)*vs[i-1] + (r)*vs[i]; }
      else  { result  = (vs[i-1]+vs[i])/2; } } }
  return result; }

void Loop::calcSegmentDef(const unsigned i)
{ /* Intentionally left empty */ }

void Loop::calcSegmentLin(const unsigned i)
{ Vector3Dvector vs;
  if (segments[i].bzps.empty()) { return; }
  if (segments[i].bzps.front()[0]>epsilon()-1) { vs.push_back(Eigen::Vector3d(-1,1,0)); }
  for (unsigned j=0; j<segments[i].bzps.size(); j++) { vs.push_back(segments[i].bzps[j]); }
  if (segments[i].bzps.back()[0]<1-epsilon()) { vs.push_back(Eigen::Vector3d(1,1,0)); }
  for (unsigned j=0; j<vs.size(); j++) { print ("calcSegmentLin: vs",vs[j],j); }
  double fulllen = segments[i].gLength();
  for (unsigned j=0; j<planes.size(); j++)
    if (planes[j].gPart() == i)
    { Eigen::Vector3d cyl;
      switch (segments[i].gTtype())
      { case Segment::IDN :
          /* As seperate case this makes no sense, assume rotation */
        case Segment::ROT :
          cyl = linearSearch(planes[j].gLoc(),vs);
          break;
        case Segment::RTL :
        case Segment::RLR :
          cyl = linearSearch(2*planes[j].gLoc()/fulllen-1,vs);
          break; }
       cyl[2] = cyl[2]/180*M_PI;
      planes[j].sModul(cyl[1],cyl[2]);
    }
}

void Loop::calcSegmentBez(const unsigned i)
{ Vector3Dvector vs,rs;
  if (segments[i].bzps.empty()) { return; }

  double fulllen = segments[i].gLength();
  bool lScaled = (segments[i].gTtype() == Segment::RTL) || (segments[i].gTtype() == Segment::RLR);
  double lower = lScaled ? -1 : 0;
  double upper = lScaled ?  1 : fulllen;

  if (segments[i].bzps.front()[0]>lower + epsilon()) { vs.push_back(Eigen::Vector3d(lower,1,0)); }
  for (unsigned j=0; j<segments[i].bzps.size(); j++) { vs.push_back(segments[i].bzps[j]); }
  if (segments[i].bzps.back()[0]<upper-epsilon()) { vs.push_back(Eigen::Vector3d(upper,1,0)); }

  for (unsigned j=0; j<=5*faces; j++) { rs.push_back( bezierFunc((double)j/(5*faces), vs)); }
  for (unsigned j=0; j<planes.size(); j++)
    if (planes[j].gPart() == i)
    { Eigen::Vector3d cyl;
      if (lScaled)
      { cyl = linearSearch(2*planes[j].gLoc()/fulllen-1,rs);}
      else
      { cyl = linearSearch(planes[j].gLoc(),rs); }
      cyl[2] = cyl[2]/180*M_PI;
      planes[j].sModul(cyl[1],cyl[2]); } }


void Loop::calcSegmentSin(const unsigned i)
{ double width = segments[i].gPar(0)/2;
  double amplitude = segments[i].gPar(1);
  double tops = segments[i].gPar(2);
  double slope = segments[i].gPar(3);
  double phase = segments[i].gPar(4);
  double height = segments[i].gPar(5);
  double fulllen = segments[i].gLength();
  bool useEnv = (slope>minr());
  bool lScaled = (segments[i].gTtype() == Segment::RTL) || (segments[i].gTtype() == Segment::RLR);
  double border = lScaled ?  width * fulllen/2 : width;
  double period = 4 * border / tops;
  for (unsigned j=0; j<planes.size(); j++)
  { double y;
    if (planes[j].gPart() == i)
    { double x = planes[j].gLoc() - fulllen/2;
      if ( (!useEnv) && ((x<-border) || (x>border)) )
      { y = 1; }
      else
      { double env = (useEnv) ? (1+exp(-slope*(width+x)))*(1+exp(-slope*(width-x))) : 1;
        y = (amplitude * cos(2*M_PI*(x/period+phase))/env + height);
      planes[j].sModul(y,0); } } } }



void Loop::extrude(const double Pmaxr)
{ planes.reserve(faces*steps.size());
  bool reduct = true;
  unsigned fr = 0;
  while (reduct && (fr++<100))
  { reduct = false;
    for (unsigned i=0; i<steps.size(); i++) { reduct = filterPlanes(Pmaxr,i) || reduct; };
    for (int i=steps.size()-1; i>=0; i--)
    { if (!steps[i].valid) { steps.erase(steps.begin()+i); } } }
  for (unsigned i=0; i<steps.size(); i++) { calcExtrusionPlane(Pmaxr,i); }
  calcFrame();
  for (unsigned i=0; i<partCnt; i++)
  { switch (segments[i].gStype())
    { case Segment::DEF : calcSegmentDef(i);  break;
      case Segment::LIN : calcSegmentLin(i);  break;
      case Segment::BEZ : calcSegmentBez(i);  break;
      case Segment::SIN : calcSegmentSin(i);  break;  } } }

bool Loop::planeIsBottom(const int plane)  { return planes[plane].gCtype() == Plane::BOTTOM; }
bool Loop::planeIsTop(const int plane)     { return planes[plane].gCtype() == Plane::TOP; }
bool Loop::hullIsVisible(const int plane)  { return (planes[plane].gCtype() == Plane::BOTTOM) || (planes[plane].gCtype() == Plane::RING); }

void Loop::extrudeTransform(const int plane, const Eigen::Vector2d & mask, double result[])
{ Eigen::Vector3d  r = planes[plane].displace(mask);
  result[0] = r[0];
  result[1] = r[1];
  result[2] = r[2]; }

void Loop::planeTransform(const int plane, const Eigen::Vector3d & mask, double result[])
{ Eigen::Vector2d p(mask[0],mask[1]);
  Eigen::Vector3d  r;
  r = planes[plane].displace(p);
  result[0] = r[0];
  result[1] = r[1];
  result[2] = r[2]; }

void Loop::print(const std::string s, const Eigen::Vector3d a, const int index)
{ if (index>=0)
  { std::cerr << s << "[ " << index << " ] = (" << a[0] << "," << a[1] << ","  << a[2] << ")\n"; }
  else
  { std::cerr << s << " = (" << a[0] << "," << a[1] << ","  << a[2] << ")\n"; } }

void Loop::print(const std::string s, const Eigen::Vector2d a, const int index)
{ if (index>=0)
  { std::cerr << s << "[ " << index << " ] = (" << a[0] << "," << a[1] << ")\n"; }
  else
  { std::cerr << s << " = (" << a[0] << "," << a[1] << ")\n"; } }

