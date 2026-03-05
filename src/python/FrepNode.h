#pragma once

#include "core/node.h"
#include "Value.h"
#include <linalg.h>
#include "PolySet.h"

#ifdef ENABLE_PYTHON
#include <Python.h>
#endif
#include <libfive.h>
#include "libfive/oracle/oracle_clause.hpp"
#include "libfive/oracle/oracle_storage.hpp"

#ifdef ENABLE_PYTHON
PyObject *ifrep(const std::shared_ptr<const PolySet>& ps);
#endif

typedef std::vector<int> intList;

struct CutFace {
  double a, b, c, d;
};

struct CutProgram {
  double a, b, c, d;
  int posbranch;
  int negbranch;
};

class OpenSCADOracle : public libfive::OracleStorage<LIBFIVE_EVAL_ARRAY_SIZE>
{
public:
  OpenSCADOracle(const std::vector<CutProgram>& program, const std::vector<CutFace>& normFaces);
  void evalInterval(libfive::Interval& out) override;
  void evalPoint(float& out, size_t index = 0) override;
  void checkAmbiguous(
    Eigen::Block<Eigen::Array<bool, 1, LIBFIVE_EVAL_ARRAY_SIZE>, 1, Eigen::Dynamic> /* out */) override;

  // Find one derivative with partial differences
  void evalFeatures(boost::container::small_vector<libfive::Feature, 4>& out) override;

protected:
  std::vector<CutProgram> program;
  std::vector<CutFace> normFaces;
};

class OpenSCADOracleClause : public libfive::OracleClause
{
public:
  OpenSCADOracleClause(const std::vector<CutProgram>& program, const std::vector<CutFace>& normFaces)
    : program(program), normFaces(normFaces)
  {
    // Nothing to do here
  }

  std::unique_ptr<libfive::Oracle> getOracle() const override
  {
    return std::unique_ptr<libfive::Oracle>(new OpenSCADOracle(program, normFaces));
  }

  std::string name() const override { return "OpenSCADOracleClause"; }

protected:
  std::vector<CutProgram> program;
  std::vector<CutFace> normFaces;
};

class FrepNode : public LeafNode
{
public:
  explicit FrepNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "sdf"; }
  std::unique_ptr<const Geometry> createGeometry() const override;
#ifdef ENABLE_PYTHON
  PyObject *expression = nullptr;
#endif
  double x1 = 0, y1 = 0, z1 = 0;
  double x2 = 0, y2 = 0, z2 = 0;
  double res = 0;
};
