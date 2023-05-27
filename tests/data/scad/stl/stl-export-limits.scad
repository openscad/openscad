doubleValueOfFloatEpsilon = 1.19208998e-07;

differentDoubleWithSameFloatCast1 = 1.23456788;
differentDoubleWithSameFloatCast2 = 1.23456789;

cube([
  doubleValueOfFloatEpsilon,
  // Check that we're only outputting floats, not doubles! (there should be no 1.23456789 in the output)
  differentDoubleWithSameFloatCast1,
  differentDoubleWithSameFloatCast2
]);
