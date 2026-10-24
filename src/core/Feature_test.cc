#include "Feature.h"

#include <catch2/catch_all.hpp>
#include <utility>
#include <vector>

namespace {
// Feature state is global. Restore it even if a REQUIRE aborts a test case.
class RestoreFeatureStates
{
public:
  RestoreFeatureStates()
  {
    for (auto it = Feature::begin(); it != Feature::end(); ++it) {
      states.emplace_back(*it, (*it)->is_enabled());
    }
  }

  ~RestoreFeatureStates()
  {
    for (const auto& [feature, enabled] : states) {
      feature->enable(enabled);
    }
  }

private:
  std::vector<std::pair<Feature *, bool>> states;
};
}  // namespace

TEST_CASE("Unicode identifiers are stable and enabled by default", "[core][Feature]")
{
  CHECK(Feature::UnicodeIdentifiers.get_default_enabled());
  CHECK(Feature::UnicodeIdentifiers.is_available());
  CHECK(Feature::UnicodeIdentifiers.is_enabled());
  CHECK_FALSE(Feature::ExperimentalRoof.get_default_enabled());
}

TEST_CASE("Unicode identifiers support an explicit opt-out and re-enabling", "[core][Feature]")
{
  const RestoreFeatureStates restore;

  Feature::enable_feature("unicode-identifiers", false);
  CHECK_FALSE(Feature::UnicodeIdentifiers.is_enabled());
  CHECK(Feature::UnicodeIdentifiers.get_default_enabled());

  Feature::enable_feature("unicode-identifiers");
  CHECK(Feature::UnicodeIdentifiers.is_enabled());
}

TEST_CASE("An explicit Unicode opt-out overrides enabling all features", "[core][Feature]")
{
  const RestoreFeatureStates restore;

  Feature::enable_all();
  CHECK(Feature::UnicodeIdentifiers.is_enabled());
  Feature::enable_feature("unicode-identifiers", false);
  CHECK_FALSE(Feature::UnicodeIdentifiers.is_enabled());
}

TEST_CASE("Experimental feature availability follows the build configuration", "[core][Feature]")
{
  const RestoreFeatureStates restore;

  Feature::enable_feature("roof");
#ifdef ENABLE_EXPERIMENTAL
  CHECK(Feature::ExperimentalRoof.is_available());
  CHECK(Feature::ExperimentalRoof.is_enabled());
#else
  CHECK_FALSE(Feature::ExperimentalRoof.is_available());
  CHECK_FALSE(Feature::ExperimentalRoof.is_enabled());
#endif
  CHECK(Feature::UnicodeIdentifiers.is_available());
  CHECK(Feature::UnicodeIdentifiers.is_enabled());
}
