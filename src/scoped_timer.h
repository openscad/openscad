// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

class ScopedTimer {
public:
  ScopedTimer(const std::string& name) : name(name) {
    t.start();
  }
  ~ScopedTimer() {
    t.stop();
    printf("[%s] time %f s\n", name.c_str(), t.time());
  }
private:
  std::string name;
  CGAL::Timer t;
};

#ifdef PERFORMANCE_TIMINGS
#define SCOPED_PERFORMANCE_TIMER(name) ScopedTimer timer(name)
#else
#define SCOPED_PERFORMANCE_TIMER(name)
#endif
