#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "Feature.h"
#include "printutils.h"

#ifdef ENABLE_CGAL
#include "cgal.h" // for FAST_CSG_KERNEL_IS_LAZY
#endif

/**
 * Feature registration map/list for later lookup. This must be initialized
 * before the static feature instances as those register with this map.
 * NOTE: All features double-register pointers to themselves in these two
 * structures, and can be accessed from either.
 */
Feature::map_t Feature::feature_map;  // Double-listed values. ----v
Feature::list_t Feature::feature_list;  // Double-listed values. --^

/*
 * List of features, the names given here are used in both command line
 * argument to enable the option and for saving the option value in GUI
 * context.
 * NOTE: The order of features listed in the gui is determined by the
 * (well-defined) order of object construction, matching the order of the
 * const Features listed below.
 */
const Feature Feature::ExperimentalFastCsg("fast-csg", "Enable much faster CSG operations with corefinement instead of nef when possible.");
const Feature Feature::ExperimentalFastCsgTrustCorefinement("fast-csg-trust-corefinement", "Speed up fast-csg by trusting corefinement functions to tell us the cases they don't support, rather than proactively avoiding these with costly checks.");
const Feature Feature::ExperimentalFastCsgDebug("fast-csg-debug", "Debug mode for fast-csg: adds logs with extra costly checks and dumps .off files with the last corefinement operands.");
#if FAST_CSG_KERNEL_IS_LAZY
const Feature Feature::ExperimentalFastCsgExact("fast-csg-exact", "Force lazy numbers to exact after each CSG operation.");
const Feature Feature::ExperimentalFastCsgExactCorefinementCallback("fast-csg-exact-callbacks", "Same as fast-csg-exact but even forces exact numbers inside corefinement callbacks rather than at the end of each operation.");
#endif // FAST_CSG_KERNEL_IS_LAZY
const Feature Feature::ExperimentalFastCsgRemesh("fast-csg-remesh", "Simplify results of fast-csg to avoid explosively slow renders (adds a bit of overhead as uses corefinement callbacks)");
const Feature Feature::ExperimentalFastCsgRemeshPredictibly("fast-csg-remesh-predictibly", "Same as fast-csg-remesh but ensuring it remeshes faces starting from a predictible vertex. Slower but good for tests.");
const Feature Feature::ExperimentalRoof("roof", "Enable <code>roof</code>");
const Feature Feature::ExperimentalInputDriverDBus("input-driver-dbus", "Enable DBus input drivers (requires restart)");
const Feature Feature::ExperimentalLazyUnion("lazy-union", "Enable lazy unions.");
const Feature Feature::ExperimentalVxORenderers("vertex-object-renderers", "Enable vertex object renderers");
const Feature Feature::ExperimentalVxORenderersIndexing("vertex-object-renderers-indexing", "Enable indexing in vertex object renderers");
const Feature Feature::ExperimentalVxORenderersDirect("vertex-object-renderers-direct", "Enable direct buffer writes in vertex object renderers");
const Feature Feature::ExperimentalVxORenderersPrealloc("vertex-object-renderers-prealloc", "Enable preallocating buffers in vertex object renderers");
const Feature Feature::ExperimentalTextMetricsFunctions("textmetrics", "Enable the <code>textmetrics()</code> and <code>fontmetrics()</code> functions.");
const Feature Feature::ExperimentalImportFunction("import-function", "Enable import function returning data instead of geometry.");
const Feature Feature::ExperimentalObjectFunction("object-function", "Enable object function to allow user creation of objects.");
const Feature Feature::ExperimentalSortStl("sort-stl", "Sort the STL output for predictable, diffable results.");

Feature::Feature(const std::string& name, const std::string& description)
  : enabled(false), name(name), description(description)
{
  feature_map[name] = this;
  feature_list.push_back(this);
}

Feature::~Feature()
{
}

const std::string& Feature::get_name() const
{
  return name;
}

const std::string& Feature::get_description() const
{
  return description;
}

bool Feature::is_enabled() const
{
#ifdef ENABLE_EXPERIMENTAL
  return enabled;
#else
  return false;
#endif
}

void Feature::enable(bool status)
{
  enabled = status;
}

// Note, features are also accessed by iterator with begin/end.
void Feature::enable_feature(const std::string& feature_name, bool status)
{
  auto it = feature_map.find(feature_name);
  if (it != feature_map.end()) {
    it->second->enable(status);
  } else {
    LOG(message_group::Warning, Location::NONE, "", "Ignoring request to enable unknown feature '%1$s'.", feature_name);
  }
}

void Feature::enable_all(bool status)
{
  for (const auto& f : boost::make_iterator_range(Feature::begin(), Feature::end())) {
    f->enable(status);
  }
}

Feature::iterator Feature::begin()
{
  return feature_list.begin();
}

Feature::iterator Feature::end()
{
  return feature_list.end();
}

std::string Feature::features()
{
  const auto seq = boost::make_iterator_range(Feature::begin(), Feature::end());
  const auto str = [](const Feature *const f) {
      return (boost::format("%s%s") % f->get_name() % (f->is_enabled() ? "*" : "")).str();
    };
  return boost::algorithm::join(boost::adaptors::transform(seq, str), ", ");
}

ExperimentalFeatureException::ExperimentalFeatureException(const std::string& what_arg)
  : EvaluationException(what_arg)
{

}

ExperimentalFeatureException::~ExperimentalFeatureException() throw()
{

}

void ExperimentalFeatureException::check(const Feature& feature)
{
  if (!feature.is_enabled()) {
    throw ExperimentalFeatureException(STR("ERROR: Experimental feature not enabled: '" << feature.get_name() << "'. Please check preferences."));
  }
}
