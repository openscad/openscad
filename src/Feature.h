#pragma once

#include <map>
#include <string>
#include <vector>

#include "utils/exceptions.h"

class Feature
{
public:
  using list_t = std::vector<Feature *>;
  using iterator = list_t::iterator;

  static const Feature ExperimentalRoof;
  static const Feature ExperimentalInputDriverDBus;
  static const Feature ExperimentalLazyUnion;
  static const Feature ExperimentalVxORenderersIndexing;
  static const Feature ExperimentalTextMetricsFunctions;
  static const Feature ExperimentalImportFunction;
  static const Feature ExperimentalObjectFunction;
  static const Feature ExperimentalPredictibleOutput;
  static const Feature ExperimentalVectorSwizzle;
  static const Feature ExperimentalDiscretizationByError;
  static const Feature ExperimentalAiFeatures;
  static const Feature ExperimentalProcessIsolation;
  static const Feature ExperimentalStreamingPreview;
  static const Feature ExperimentalStructuredDiagnostics;
#ifdef ENABLE_PYTHON
  static const Feature ExperimentalPythonEngine;
#endif

#ifdef ENABLE_GUI_TESTS
  static constexpr bool HasGuiTesting{true};
#else
  static constexpr bool HasGuiTesting{false};
#endif

  [[nodiscard]] const std::string& get_name() const;
  [[nodiscard]] const std::string& get_description() const;

  [[nodiscard]] bool is_enabled() const;
  void enable(bool status);

  // The feature this one is meaningless without, or nullptr. A dependent feature reports itself
  // disabled while its dependency is off, so no caller has to check the pair.
  [[nodiscard]] const Feature *get_dependency() const { return dependency; }

  static iterator begin();
  static iterator end();

  static std::string features();
  static void enable_feature(const std::string& feature_name, bool status = true);
  static void enable_all(bool status = true);

private:
  bool enabled{false};

  const Feature *dependency{nullptr};

  const std::string name;
  const std::string description;

  using map_t = std::map<std::string, Feature *>;
  static map_t feature_map;
  static list_t feature_list;

  Feature(const std::string& name, std::string description, bool hidden = false);
  Feature(const std::string& name, std::string description, const Feature *dependency);
  virtual ~Feature() = default;
};

class ExperimentalFeatureException : public EvaluationException
{
public:
  static void check(const Feature& feature);

private:
  ExperimentalFeatureException(const std::string& what_arg);
};
