#pragma once

#include <cstdio>
#include <string>
#include <map>
#include <vector>

#include "utils/exceptions.h"

class Feature
{
public:
  using list_t = std::vector<Feature *>;
  using iterator = list_t::iterator;

  static const Feature ExperimentalFastCsg;
  static const Feature ExperimentalFastCsgSafer;
  static const Feature ExperimentalFastCsgDebug;
  static const Feature ExperimentalRoof;
  static const Feature ExperimentalInputDriverDBus;
  static const Feature ExperimentalLazyUnion;
  static const Feature ExperimentalVxORenderersIndexing;
  static const Feature ExperimentalTextMetricsFunctions;
  static const Feature ExperimentalImportFunction;
  static const Feature ExperimentalPredictibleOutput;
#ifdef ENABLE_PYTHON
  static const Feature ExperimentalPythonEngine;
#endif

  [[nodiscard]] const std::string& get_name() const;
  [[nodiscard]] const std::string& get_description() const;

  [[nodiscard]] bool is_enabled() const;
  void enable(bool status);

  static iterator begin();
  static iterator end();

  static std::string features();
  static void enable_feature(const std::string& feature_name, bool status = true);
  static void enable_all(bool status = true);

private:
  bool enabled{false};

  const std::string name;
  const std::string description;

  using map_t = std::map<std::string, Feature *>;
  static map_t feature_map;
  static list_t feature_list;

  Feature(const std::string& name, std::string description, bool hidden = false);
  virtual ~Feature() = default;
};

class ExperimentalFeatureException : public EvaluationException
{
public:
  static void check(const Feature& feature);

private:
  ExperimentalFeatureException(const std::string& what_arg);
};
