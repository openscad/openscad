// Streaming preview and structured diagnostics are both meaningless without process isolation:
// they change how the isolated compute worker returns results and diagnostics, and there is no
// worker to change when the feature is off. Rather than have every caller remember that, a
// dependent feature reports itself disabled whenever the feature it depends on is.
#include "Feature.h"

#include <catch2/catch_all.hpp>

TEST_CASE("a dependent feature stays off while its dependency is off", "[feature]")
{
  Feature::enable_all(false);

  Feature::enable_feature("streaming-preview");
  Feature::enable_feature("structured-diagnostics");
  CHECK_FALSE(Feature::ExperimentalStreamingPreview.is_enabled());
  CHECK_FALSE(Feature::ExperimentalStructuredDiagnostics.is_enabled());

  // Turning the dependency on reveals the choices the user already made; it does not discard them.
  Feature::enable_feature("process-isolation");
  CHECK(Feature::ExperimentalStreamingPreview.is_enabled());
  CHECK(Feature::ExperimentalStructuredDiagnostics.is_enabled());

  // And turning it back off hides them again rather than clearing them.
  Feature::enable_feature("process-isolation", false);
  CHECK_FALSE(Feature::ExperimentalStreamingPreview.is_enabled());
  CHECK_FALSE(Feature::ExperimentalStructuredDiagnostics.is_enabled());

  Feature::enable_all(false);
}

TEST_CASE("a feature with no dependency is unaffected", "[feature]")
{
  Feature::enable_all(false);
  Feature::enable_feature("lazy-union");
  CHECK(Feature::ExperimentalLazyUnion.is_enabled());
  Feature::enable_all(false);
}
