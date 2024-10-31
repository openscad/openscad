#pragma once

#include <limits>
#include "utils/printutils.h"

#include <boost/cast.hpp>
#include <sstream>

#include <boost/logic/tribool.hpp>
BOOST_TRIBOOL_THIRD_STATE(unknown)

/* Convert number types but print WARNING for failures during
   conversion. This is useful for situations where it is important to not
   fail silently during casting or conversion. (For example, accidentally
   converting 64 bit types to 32 bit types, float to int, etc).
   For positive overflow, return max of Tout template type
   For negative overflow, return min of Tout template type
   On other conversion failures, return 0. */
template <class Tout, class Tin> Tout boost_numeric_cast(Tin input)
{
  Tout result = 0;
  std::ostringstream status;
  status.str("ok");
  try {
    result = boost::numeric_cast<Tout>(input);
  } catch (boost::numeric::negative_overflow& e) {
    status << e.what();
    result = std::numeric_limits<Tout>::min();
  } catch (boost::numeric::positive_overflow& e) {
    status << e.what();
    result = std::numeric_limits<Tout>::max();
  } catch (boost::numeric::bad_numeric_cast& e) {
    status << e.what();
    result = 0;
  }
  if (status.str() != "ok") {
    LOG(message_group::Warning, "Problem converting this number: %1$s", std::to_string(input));
    LOG(message_group::Warning, "%1$s", status.str());
    LOG(message_group::Warning, "setting result to %1$u", result);
  }
  return result;
}


