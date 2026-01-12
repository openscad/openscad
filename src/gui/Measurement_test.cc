#include <catch2/catch_all.hpp>
#include "Measurement.h"

enum class MeasureMode { DISTANCE, ANGLE };

TEST_CASE("Measurement Values Plumbed to GUI", "[measurement]") {
  // Reference the template inside the namespace
  using MTemplate = Measurement::Template<Measurement::FakeGLView>;
    
  struct MathTestCase {
    std::string name;
    MeasureMode mode;
    std::vector<SelectedObject> objects;
    // Substrings to find in result.messages, starting from the LAST message added
    std::vector<QString> expectedSubstrings; 
  };

  auto p = [](double x, double y, double z) {
    SelectedObject obj;
    obj.type = SelectionType::SELECTION_POINT;
    obj.p1 = {x, y, z};
    return obj;
  };

  auto l = [](double x1, double y1, double z1, double x2, double y2, double z2) {
    SelectedObject obj;
    obj.type = SelectionType::SELECTION_LINE;
    obj.p1 = {x1, y1, z1};
    obj.p2 = {x2, y2, z2};
    return obj;
  };

  std::vector<MathTestCase> cases = {
    {
      "Point to Point",
      MeasureMode::DISTANCE,
      { p(0,0,0), p(3,4,0) },
      { "5", "3, 4, 0" } 
    },
    {
      "90 Degree Angle",
      MeasureMode::ANGLE,
      { p(1,0,0), p(0,0,0), p(0,1,0) },
      { "90" }
    },
    {
      "Skew Lines",
      MeasureMode::DISTANCE,
      { 
        l(0, 0, 0, 10, 0, 0),
        l(0, 0, 5, 0, 10, 5)
      },
      { "5", "0, 0, 5" } 
    },
  };

  for (const auto& tc : cases) {
    SECTION(tc.name) {
      MTemplate meas;
      Measurement::FakeGLView fake;
      meas.setView(&fake);

      // 1. Initialize Mode using namespace enums
      if (tc.mode == MeasureMode::ANGLE) {
        meas.startMeasureAngle();
      } else {
        meas.startMeasureDist();
      }

      // Load the queue for the fake selectPoint calls
      fake.selection_queue = tc.objects;

      // 2. Drive State Machine
      Measurement::Result result;
      int safety_break = 0;
      do {
        result = meas.statemachine(QPoint(0,0));
        // Safety break to prevent infinite loops if state doesn't advance
      } while (result.status == Measurement::Result::Status::NoChange && ++safety_break < 10);

      // 3. Verification
      REQUIRE(result.status == Measurement::Result::Status::Success);
      REQUIRE(result.messages.size() >= tc.expectedSubstrings.size());

      for (size_t i = 0; i < tc.expectedSubstrings.size(); ++i) {
        // messages is a vector, so the last addText call is at size() - 1
        // We iterate backwards through messages using i
        const auto& actualMsg = result.messages[result.messages.size() - 1 - i].display_text;
        const auto& expected = tc.expectedSubstrings[i];

        UNSCOPED_INFO("Expected substring: " << expected.toStdString());
        UNSCOPED_INFO("Actual message: " << actualMsg.toStdString());
                
        CHECK(actualMsg.contains(expected));
      }
    }
  }
}
