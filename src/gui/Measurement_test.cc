#include <catch2/catch_all.hpp>
#include "Measurement.h"

class MeasurementTest {
public:
  MeasurementTest(Measurement & m) : m(m) {}
  Measurement& m;
  Measurement::Distance distMeasurement(SelectedObject& obj1, SelectedObject& obj2)
  {
    return m.distMeasurement(obj1, obj2);
  }
};
